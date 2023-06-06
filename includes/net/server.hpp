
#pragma once

#ifdef ENABLE_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include <string>
#include <iostream>
#include <map>
#include <stack>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "packet.hpp"
#include "dto_base.hpp"

#include "Tintin_reporter.hpp"

// predefinition for ServerClient
template<typename H>
class Server;



/*  ServerClient class 
*
*   TODO DESCRIPTION
*   ...
*
*
*
*
*
*/
template<typename H, typename D>
class ServerClient : public D, protected PacketManager
{
#ifdef ENABLE_TLS
    private:
        ServerClient(int socket, SSL* ssl = nullptr)
            : D(), _socket(socket), _ssl(ssl), _is_ssl(ssl != nullptr), _accept_done(false)
            {}
    
    public:
        // ipv4 SSL client
        ServerClient(int socket, const sockaddr_in& address, SSL* ssl = nullptr)
            : ServerClient(socket, ssl)
            {
                this->_address = std::string(inet_ntoa(address.sin_addr));
                this->_port = ntohs(address.sin_port);
            }

        // ipv6 SSL client
        ServerClient(int socket, const sockaddr_in6& address, const socklen_t address_len, SSL* ssl = nullptr)
            : ServerClient(socket, ssl)
            {
                char addr_buffer[address_len];
                if (inet_ntop(AF_INET6, &address.sin6_addr, addr_buffer, address_len) == NULL)
                {
                    LOG_ERROR(LOG_CATEGORY_NETOWRK, "Invalid address for initializing a new client");
                    // TODO
                }
                
                this->_address = std::string(addr_buffer);
                this->_port = ntohs(address.sin6_port);
            }
#else
    private:
        ServerClient(int socket)
            : D(), _socket(socket)
            {}
    
    public:
        // ipv4 client
        ServerClient(int socket, const sockaddr_in& address)
            : ServerClient(socket)
            {
                this->_address = std::string(inet_ntoa(address.sin_addr));
                this->_port = ntohs(address.sin_port);
            }
        
        // ipv6 client
        ServerClient(int socket, const sockaddr_in6& address, const socklen_t address_len)
            : ServerClient(socket)
            {
                char addr_buffer[address_len];
                if (inet_ntop(AF_INET6, &address.sin6_addr, addr_buffer, address_len) == NULL)
                {
                    LOG_ERROR(LOG_CATEGORY_NETOWRK, "Invalid address for initializing a new client");
                    // TODO
                }
                
                this->_address = std::string(addr_buffer);
                this->_port = ntohs(address.sin6_port);
            }
#endif

#ifdef ENABLE_TLS
        SSL    *getSSL() const { return this->_ssl; }
        bool    isSSL() const { return this->_is_ssl; }

        // todo 
        std::string getCertificate()
        {
            if (!this->_is_ssl)
                return "";

            X509 *cert;
            cert = SSL_get_peer_certificate(this->_ssl);
            if ( cert == NULL )
                return "No certificate";
            
            // char *line;
            // printf("Server certificates:\n");
            // line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
            // printf("Subject: %s\n", line);
            // free(line);
            // line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
            // printf("Issuer: %s\n", line);
            // free(line);
            // X509_free(cert);
            char *line;
            line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
            std::string certificate = std::string(line, std::strlen(line));
            free(line);
            X509_free(cert);
            return (certificate);
        }

#endif

        int     getSocket() const { return this->_socket; }

        std::string getFullAddress() const
        {
            return (this->_address + ":" + std::to_string(this->_port));
        }

        std::string getAddress() const
        {
            return (this->_address);
        }

        int         getPort() const
        {
            return (this->_port);
        }

    private:
        int         _socket;
        std::string _address;
        int         _port;

#ifdef ENABLE_TLS
        SSL    *_ssl;
        bool    _is_ssl;
        bool    _accept_done;
#endif

    friend class Server<H>;
};



/*      ServerClientHandler class 
*
*   TODO DESCRIPTION
*   ...
*
*
*
*
*
*/
template<typename T, typename D>
class ServerClientHandler
{
    public:
        typedef D                                   client_data_type;
        typedef ServerClient<T, client_data_type>   client_type;
        typedef Server<T>                           server_type;
        typedef ServerClientHandler<T, D>           handler_type;
    
    public:
        ServerClientHandler(server_type& server)
            : server(server)
            {}

        virtual void    onConnected(client_type& client) { (void)client; };
        virtual void    onDisconnected(client_type& client) { (void)client; };

        // called when an user tried to send data on message but data type mismatched.
        virtual void    onMessageMismatch(client_type& client, const std::string& message_name) { (void)client; (void)message_name; };


        virtual void    declareMessages() = 0;
    
    protected:
        server_type&        server;
};


class DefaultClientData
{
    // data per client
    // std::string name;
    // ...
};

/* Example client handler */
class DefaultClientHandler : ServerClientHandler<DefaultClientHandler, DefaultClientData>
{
    // struct ExampleDto : DTO
    // {
    //     int     data1;
    //     long    data2;
    //     char    str_data[128];
    // };

    // void declareMessages()
    // {
    //     this->server.onMessage("example", server_type::make_handler<ExampleDto>(
    //         [](server_type& server, ServerClient& from, DTO* obj)
    //         {
    //             // retrieve dto type 
    //             ExampleDto  *dto = reinterpret_cast<ExampleDto*>(obj);

    //             std::cout << "in example handler" << std::endl;
    //             // ...
    //         }
    //     ));
    // }
};





/*  Server class 
*
*   TODO DESCRIPTION
*   ...
*
*
*
*
*
*/
template<typename H = DefaultClientHandler>
class Server
{
    public:
        /* ================================================ */
        /* Constructors                                     */
        /* ================================================ */

        Server(const std::string& ip_address, const int port, const std::string& ip_address6 = "")
        : ip_address(ip_address),
          ip_address6(ip_address6),
          port(port),
#ifdef ENABLE_TLS
          ssl_port(443),
#endif
          running(false),
          n_clients_connected(0),
          max_connections(-1),
          enable_IPv6(!ip_address6.empty()),
          _client_handler(*this),
          _socket(-1),
          _socket6(-1)
#ifdef ENABLE_TLS
          ,_ssl_socket(-1),
          _ssl_only(false),
          _ssl_method(TLS_server_method())
#endif
        {            
            this->_init_addr4();

            if (this->enable_IPv6)
            {
                this->_init_addr6();
            }

#ifdef ENABLE_TLS
            SSL_library_init();

            this->_init_ssl_ctx();
            this->_init_ssl_addr();
#endif

            FD_ZERO(&this->_read_fds);
            FD_ZERO(&this->_write_fds);
        }

        Server(const Server& copy) = delete;

        Server& operator=(const Server& other) = delete;


        ~Server()
        {
            for (client_type& client : this->_clients)
                this->disconnect(client);

#ifdef ENABLE_TLS
            SSL_CTX_free(this->_ssl_ctx);
            close(this->_ssl_socket);
#endif
            close(this->_socket);
            if (enable_IPv6)
                close(this->_socket6);
        }


        /* ================================================ */
        /* Public Members                                   */
        /* ================================================ */

        /* Makes the server starts listening,                                       */
        /* after that, wait_update should be called to gather informations.         */
        void    start_listening(int max_pending_connections = 10)
        {
#ifdef ENABLE_TLS
            this->_load_ssl_certs();
#endif
            this->_init_socket4(max_pending_connections);

            if (enable_IPv6)
            {
                this->_init_socket6(max_pending_connections);
            }

#ifdef ENABLE_TLS
            _init_ssl_socket(max_pending_connections);
#endif

            this->_client_handler.declareMessages();
            this->running = true;
        }

        /* Waits for an update on the read fds, needs to be called every time        
        */
        /* It will gather informations received on sockets and run clients handlers  */
        /* This function should be called after doing some operations on the clients */
        /* externally so that the data processed has less latency                    */
        /* It will also perform all the emits requested before its call              */
        /* It returns true if the server is still active after completion            */
        /* If specified, a timeout argument can be set to define the                 */
        /* timeout in ms for select                                                  */
        bool    wait_update(const int timeout_ms = -1)
        {
            if (!running)
                return (false);
            // preserve main fd_set's
            fd_set selected_read_fds = {0};
            fd_set selected_write_fds = {0};
            FD_COPY(&this->_read_fds, &selected_read_fds);
            FD_COPY(&this->_write_fds, &selected_write_fds);
            
            int nfds = this->_get_nfds();
            struct timeval *timeout_ptr = nullptr;
            struct timeval timeout = {.tv_sec=timeout_ms / 1000,.tv_usec=(timeout_ms % 1000) * 1000};
            if (timeout_ms != -1)
            {
                timeout_ptr = &timeout;
            }
            if (select(nfds, &selected_read_fds, &selected_write_fds, NULL, timeout_ptr) == -1)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Select failed: " << strerror(errno));
                throw SelectException();
            }

            // look for clients receive
            for (typename Server::client_list_type::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
            {
                // received from client
                if (FD_ISSET(it->second.getSocket(), &selected_read_fds))
                {
                    if (this->_receive(it->second) == false)
                        break ; // sent disconnect
                    if (!running)
                        return (false); 
                }
                // writing enabled for client, can send data
                if (FD_ISSET(it->second.getSocket(), &selected_write_fds))
                {
                    this->_send_data(it->second);
                }
            }

            // look for connections on ipv4 
            if (FD_ISSET(this->_socket, &selected_read_fds))
            {
                // client tries to connect
                this->_accept();
            }

            if (this->enable_IPv6)
            {
                // look for connections on ipv6 
                if (FD_ISSET(this->_socket6, &selected_read_fds))
                {
                    // client tries to connect
                    this->_accept6();
                }
            }

#ifdef  ENABLE_TLS
            // look for connections on TLS
            if (FD_ISSET(this->_ssl_socket, &selected_read_fds))
            {
                this->_accept_ssl();
            }
#endif

            return (running);
        }

        /* ================================================ */
        /* Server types                                     */
        /* ================================================ */
        typedef H                           handler_type;
        typedef typename H::client_type     client_type;
        typedef Server<H>                   server_type;

        struct functor_handler
        {
            size_t  sizeof_type;
            void(*handle)(server_type&, client_type&, DTO*);
        };

        template<typename T>
        static functor_handler    make_handler(void(*handler_function)(server_type&, client_type&, DTO*))
        {
            functor_handler handler;
            handler.sizeof_type = sizeof(T);
            handler.handle = handler_function;
            return (handler);
        }

        typedef functor_handler message_handler_type;

        typedef std::map<std::string, message_handler_type> msg_list_type;
        typedef std::map<int, client_type>                  client_list_type;

        friend client_type;

        /* ================================================ */
        /* Messages management                              */
        /* ================================================ */

        void    onMessage(const std::string& name, message_handler_type handler)
        {
            if (name.length() > sizeof(packed_data_header<0>::message_name))
            {
                LOG_ERROR(LOG_CATEGORY_INIT, "cannot add handler for `" << name << "` message: name is too long (" 
                    << sizeof(packed_data_header<0>::message_name) << " max)"); 
                return ;           
            }
            if (!is_valid_message_name(name.c_str()))
            {
                LOG_ERROR(LOG_CATEGORY_INIT, "cannot add handler for `" << name << "` message: name is not valid (allowed characters are [a-z][A-Z][0-9][_,-,/,.,[,],<,>])");  
                return ;
            }
            this->_messages_handlers.insert(std::make_pair(name, handler));
            LOG_INFO(LOG_CATEGORY_INIT, "added new handler for `" << name << "` message");
        }


        /* ================================================ */
        /* Emits                                            */
        /* ================================================ */

        /* emits T to all clients connected as ServerClient */
        template<typename T>
        void    emit(const std::string& message, const T& to_emit)
        { 
            for (client_type& client : this->_clients)
                this->_emit_base(message, to_emit);
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted for each clients");
        }
    
        /* emits T to the client specified as `client` */
        template<typename T>
        void    emit(const std::string& message, const T& to_emit, client_type& client)
        { 
            this->_emit_base(message, to_emit, client);
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted from " << client.getFullAddress())
        }
        
        /* emit_now emits the same way emit does, however it doesn't wait for wait_update() to be sent   */
        /* this is usefull for errors or for emitting something before calling disconnect()              */
        /* however the client's socket might not be able to receive the packet at the moment it is sent  */
        /* in that case, the package is queued and emit_now returns false indicating a failure           */
        /* same thing if send wasn't able to send the packet entierely, emit_now returns false           */
        /* ! IT WILL NOT SEND THE LEFTOVER PACKET IF IT WASN'T ABLE TO SENT IT ENTIERLY !                */
        /* for that reason, it should be used to send small data structures to ensure data packages are  */
        /* completely sent.                                                                              */
        template<typename T>
        bool    emit_now(const std::string& message, const T& to_emit, client_type& client)
        { 
            this->emit(message, to_emit, client);
            try {
                return this->_send_data(client);
            } catch (SendException e)
            {
                return (false);
            }
            return (true);
        }

        template<typename T>
        bool    emit_now(const std::string& message, const T& to_emit)
        { 
            this->emit(message, to_emit);
            int emitted_all = 0;
            try {
                for (client_type& client : this->_clients)
                    emitted_all += this->_send_data(client);
            } catch (SendException e)
            {
                return (false);
            }
            return (emitted_all == this->_clients.size());
        }


        /* ================================================ */
        /* Other Utils                                      */
        /* ================================================ */

        /* disconnects the specified client from the server*/
        bool    disconnect(client_type& client)
        {
            int socket = client.getSocket();

#ifdef ENABLE_TLS
            if (client._is_ssl && client._ssl != nullptr)
            {
                SSL_free(client._ssl);
            }
#endif

            if (this->_clients.erase(socket) == 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "trying to disconnect unkown client from socket " << socket)
                return false;
            }
            FD_CLR(socket, &this->_read_fds);
            ::close(socket);
            this->n_clients_connected--;
            LOG_INFO(LOG_CATEGORY_NETWORK, "client disconnected from socket " << socket);
            return (true);
        }


        void    shutdown()
        {
            ::close(this->_socket);
            ::close(this->_socket6);
            this->running = false;
        }


        client_type&    getClient(const int socket)
        {
            return this->_clients.at(socket);
        }
        
        const client_list_type& getClients() const
        {
            return this->_clients;
        }




        /* ================================================ */
        /* PRIVATE MEMBERS                                  */
        /* ================================================ */

    private:

        /* ================================================ */
        /* Socket init                                      */
        /* ================================================ */

        void _init_socket_base(int& sock, sockaddr_in& address, const int max_pending_connections)
        {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock <= 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Socket syscall failed for address initialization on port " << ntohs(address.sin_port) << ": " << strerror(errno));
                throw SocketException();
            }

            if (bind(sock, (sockaddr*)(&address), sizeof(address)) != 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Bind syscall failed for address initialization on port " << ntohs(address.sin_port) << ": " << strerror(errno));
                throw BindException();
            }

            if (listen(sock, max_pending_connections) != 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Listen syscall failed for address initialization on port " << ntohs(address.sin_port) << ": " << strerror(errno));
                throw ListenException();
            }

            FD_SET(sock, &this->_read_fds);
        }

        void    _init_socket4(const int max_pending_connections)
        {
            this->_init_socket_base(this->_socket, this->_address, max_pending_connections);
            LOG_INFO(LOG_CATEGORY_NETWORK, "Server started listening on address " << this->ip_address << " on port " << this->port);
        }

        void    _init_socket6(const int max_pending_connections)
        {
            this->_socket6 = socket(AF_INET, SOCK_STREAM, 0);
            if (this->_socket6 <= 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Socket syscall failed for address initialization on port " << ntohs(this->_address6.sin6_port) << ": " << strerror(errno));
                throw SocketException();
            }

            if (bind(this->_socket6, (sockaddr*)(&this->_address6), sizeof(this->_address6)) != 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Bind syscall failed for address initialization on port " << ntohs(this->_address6.sin6_port) << ": " << strerror(errno));
                throw BindException();
            }

            if (listen(this->_socket6, max_pending_connections) != 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Listen syscall failed for address initialization on port " << ntohs(this->_address6.sin6_port) << ": " << strerror(errno));
                throw ListenException();
            }

            FD_SET(this->_socket6, &this->_read_fds);
            LOG_INFO(LOG_CATEGORY_NETWORK, "Server started listening on IPv6 on address " << this->ip_address6 << " on port " << (this->port + 1));
        }

#ifdef ENABLE_TLS
        void    _init_ssl_socket(const int max_pending_connections)
        {
            this->_init_socket_base(this->_ssl_socket, this->_ssl_address, max_pending_connections);
            LOG_INFO(LOG_CATEGORY_NETWORK, "Server started listening on TLS on address " << this->ip_address << " on port " << (this->ssl_port));
        }
#endif

        /* ================================================ */
        /* Address init                                     */
        /* ================================================ */
        
        void    _init_addr4()
        {
            this->_address = (sockaddr_in){
                .sin_family = AF_INET,
                .sin_port = htons(static_cast<in_port_t>(this->port)),
            };
            if (inet_aton(this->ip_address.c_str(), &this->_address.sin_addr) == -1)
                throw InetException();
        }

        void    _init_addr6()
        {
            this->_address6 = (sockaddr_in6){
                .sin6_family = AF_INET6,
                .sin6_port = htons(static_cast<in_port_t>(this->port + 1)),
            };
            if (inet_pton(AF_INET6, this->ip_address6.c_str(), &this->_address6) == -1)
                throw InetException();
        }

#ifdef ENABLE_TLS
        void    _init_ssl_addr()
        {
            this->_ssl_address = (sockaddr_in){
                .sin_family = AF_INET,
                .sin_port = htons(static_cast<in_port_t>(this->ssl_port)),
            };
            if (inet_aton(this->ip_address.c_str(), &this->_ssl_address.sin_addr) == -1)
                throw InetException();
        }


        /* ================================================ */
        /* SSL context init                                 */
        /* ================================================ */
        
        void    _init_ssl_ctx()
        {
            OpenSSL_add_all_algorithms();                       /* load & register all cryptos, etc. */
            SSL_load_error_strings();                           /* load all error messages           */
            this->_ssl_ctx = SSL_CTX_new(this->_ssl_method);    /* create new context from method    */
            if ( this->_ssl_ctx == NULL )
            {
                ERR_print_errors_fp(stderr);
                throw SSLInitException();
            }
        }

        void    _load_ssl_certs()
        {
            int no_cert_or_key = 0;
            if (this->ssl_cert_file.empty() && ++no_cert_or_key)
                LOG_ERROR(LOG_CATEGORY_NETWORK, "No certificate provided for server that enables TLS.")
            else if (this->ssl_private_key_file.empty() && ++no_cert_or_key)
                LOG_ERROR(LOG_CATEGORY_NETWORK, "No private key provided for server that enables TLS.")
            if (no_cert_or_key != 0)
                throw SSLCertException();
            
            /* set the local certificate from CertFile */
            if ( SSL_CTX_use_certificate_file(this->_ssl_ctx, this->ssl_cert_file.c_str(), SSL_FILETYPE_PEM) <= 0 )
            {
                ERR_print_errors_fp(stderr);
                throw SSLCertException();
            }
            /* set the private key from KeyFile (may be the same as CertFile) */
            if ( SSL_CTX_use_PrivateKey_file(this->_ssl_ctx, this->ssl_private_key_file.c_str(), SSL_FILETYPE_PEM) <= 0 )
            {
                ERR_print_errors_fp(stderr);
                throw SSLCertException();
            }
            /* verify private key */
            if ( !SSL_CTX_check_private_key(this->_ssl_ctx) )
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Private key does not match the public certificate\n");
                throw SSLCertException();
            }
        }
#endif


        /* ================================================ */
        /* Accept Handlers                                  */
        /* ================================================ */

        int     _accept()
        {
            sockaddr_in     client_addr;
            socklen_t       len;
            int client_socket = ::accept(this->_socket, (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed with error: " << std::strerror(errno));
                throw AcceptException();
            }
            if (this->max_connections > 0 && this->n_clients_connected >= this->max_connections)
            {
                LOG_INFO(LOG_CATEGORY_NETWORK, "Cannot accept more than " << this->max_connections << " connections, refusing new client from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
                // cannot accept more clients
                close (client_socket);
                return (-1);
            }
            this->n_clients_connected++;
            
            std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, client_addr)));
            if (insertion.second == false)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed for client just arrived from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
                ::close(client_socket);
                throw AcceptException();
            }

            FD_SET(client_socket, &this->_read_fds);
            LOG_INFO(LOG_CATEGORY_NETWORK, "New client connected from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
            this->_client_handler.onConnected((*insertion.first).second);
            return (client_socket);
        }

        int     _accept6()
        {
            sockaddr_in6    client_addr;
            socklen_t       len;
            int client_socket = ::accept(this->_socket6, (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed on IPv6 with error: " << std::strerror(errno));
                throw AcceptException();
            }
            this->n_clients_connected++;
            std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, client_addr, len)));
            if (insertion.second == false)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed for client just arrived on socket6 " << client_socket);
                throw AcceptException();
            }

            FD_SET(client_socket, &this->_read_fds);
            LOG_INFO(LOG_CATEGORY_NETWORK, "New client connected on IPv6 from "  << client_socket);
            this->_client_handler.onConnected((*insertion.first).second);
            return (client_socket);
        }

#ifdef ENABLE_TLS

        // like in openssl:
        // returns 1  => done
        // returns 0  => expects more data
        // returns -1 => invalid
        int     _ssl_do_accept(client_type& client)
        {
            // ssl protocol accept (blocks process, maybe set a timeout or handle with select)
            int accept_error;
            if ( (accept_error = SSL_accept(client.getSSL())) != 1 )
            {
                int ssl_error = SSL_get_error(client.getSSL(), accept_error);
                if (ssl_error == SSL_ERROR_WANT_READ)
                {
                    LOG_INFO(LOG_CATEGORY_NETWORK, "SSL accept incomplete, waiting for more data...");
                    return (0);
                }
                LOG_WARN(LOG_CATEGORY_NETWORK, "SSL accept failed for client trying to connect on from " << client.getFullAddress());
                ERR_print_errors_fp(stderr);
                this->disconnect(client);
                return (-1);
            }
            client._accept_done = true;
            LOG_INFO(LOG_CATEGORY_NETWORK, "New client connected on TLS on from " << client.getFullAddress());
            this->_client_handler.onConnected(client);
            return (1);
        }

        int     _accept_ssl()
        {
            sockaddr_in     client_addr;
            socklen_t       len;
            int client_socket = ::accept(this->_ssl_socket, (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed with error: " << std::strerror(errno));
                throw AcceptException();
            }
            if (this->max_connections > 0 && this->n_clients_connected >= this->max_connections)
            {
                LOG_INFO(LOG_CATEGORY_NETWORK, "Cannot accept more than " << this->max_connections << " connections, refusing new client from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
                // cannot accept more clients
                close (client_socket);
                return (-1);
            }
            this->n_clients_connected++;

            SSL*    ssl = SSL_new(this->_ssl_ctx);
            SSL_set_fd(ssl, client_socket);
            SSL_set_accept_state(ssl);

            std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, client_addr, ssl)));
            if (insertion.second == false)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed for client just arrived from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
                ::close(client_socket);
                throw AcceptException();
            }

            FD_SET(client_socket, &this->_read_fds);
            LOG_INFO(LOG_CATEGORY_NETWORK, "New client was accepted on TLS port " << this->ssl_port << " from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << ", waiting for SSL accept...");
            return (client_socket);
        }
#endif


        /* ================================================ */
        /* Recv handler & unpacker                          */
        /* ================================================ */

// must at least be sizeof(packet_data_header) (or sizeof(size_t) + 32)
#define RECV_BLK_SIZE   1024
        bool     _receive(client_type& from)
        {
            uint8_t buffer[RECV_BLK_SIZE] = {0};
#ifdef ENABLE_TLS
            ssize_t size;
            if (from._is_ssl)
            {
                if (!from._accept_done)
                {
                    int err = this->_ssl_do_accept(from);
                    if (err == -1)
                        return false;
                    return (true);
                }
                size = SSL_read(from._ssl, buffer, RECV_BLK_SIZE);
            }
            else
                size = recv(from.getSocket(), buffer, RECV_BLK_SIZE, MSG_DONTWAIT);
#else
            ssize_t size = recv(from.getSocket(), buffer, RECV_BLK_SIZE, MSG_DONTWAIT);
#endif
            if (size == 0)
            {
                this->disconnect(from);
                return (false);
            }
            else if (size < 0)
            {
                // we dont know what made recv fail, but for safety disconnect client.
                this->disconnect(from);
                throw RecvException();
            }

            typename Server::msg_list_type::iterator handler = this->_messages_handlers.end();

            // already received a chunk of packet 
            if (!from._received_packet.empty())
            {
                int append_result = from._appendPacket(buffer, size);
                
                if (append_result != 0)
                    return (true); // packet is incomplete, or error, needs more data chunks
                // packet complete
                handler = this->_messages_handlers.find(from._received_packet.message_name);
                if (handler == this->_messages_handlers.end())
                {
                    this->_client_handler.onMessageMismatch(from, from._received_packet.message_name);
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Cannot get data handler in parsed packet for message `" << from._received_packet.message_name << "` sent on socket " << from.getSocket() << ", this should have been resolved before parsing the packet.");
                    from._cancelPacket();
                    return (true);
                }
            }
            // no packet defined in client, expecting a header for inserting a new one
            else
            {

                // copy header from buffer
                packed_data_header<0>  data_header;
                if (!unpack_data_header(data_header, (char*)buffer, size))
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Invalid new packet from socket " << from.getSocket() << " packet size is smaller than data_header size: cannot parse packet");
                    return (true);
                }

                // check message name character conformity
                if (!is_valid_message_name(data_header))
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Packet message_name format is invalid: packet ignored.");
                    return (true);
                }

                // check message validity
                std::string message_name(data_header.message_name, std::strlen(data_header.message_name));
                handler = this->_messages_handlers.find(message_name);
                if (handler == this->_messages_handlers.end())
                {
                    this->_client_handler.onMessageMismatch(from, message_name);
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Unknown data handler for message `" << message_name << "` received on socket " << from.getSocket());
                    return (true);
                }
                if (handler->second.sizeof_type != data_header.data_size)
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Packet size for message `" << message_name << "` received on socket " << from.getSocket() << " doesnt correspond with current message packet type definition: expected a data containing " 
                    << handler->second.sizeof_type << " bytes but header specifies " << data_header.data_size << " data bytes");
                    return (true);
                }

                // create new packet in client _received_packet in case of incomplete packet
                int new_packet_result = from._newPacket(data_header, buffer, size);
                if (new_packet_result < 0)
                    return (true); // error on packet.
                else if (new_packet_result == 1) // packet is incomplete, needs more data chunks
                    return (true);
                // packet complete
            }

            return (this->_handle_packet(from, handler->second));
        }


        /* ================================================ */
        /* Packet handler call                              */
        /* ================================================ */
        bool    _handle_packet(client_type& from, const message_handler_type handler)
        {
            // this should never happen but must be here for security
            if (handler.sizeof_type != from._received_packet.data_size)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Parsed packet received on socket " << from.getSocket() << " size doesn't correspond to size in handler !!!");
                from._cancelPacket();
                return (true);
            }

            // get previous client count and socket of handeled client
            int clients_count = this->n_clients_connected;
            int client_socket = from.getSocket();

            // handlex
            uint8_t *data_buffer = new uint8_t[handler.sizeof_type];
            std::memcpy(data_buffer, from._received_packet.data.c_str(), handler.sizeof_type);
            handler.handle(*this, from, reinterpret_cast<DTO*>(data_buffer));

            delete[] data_buffer;

            // client list has changed in handler, in that case, from may have become invalid
            if (clients_count != this->n_clients_connected)
            {
                // we can try to see if the client that was disconnected was from by checking its socket in the list
                if (this->_clients.find(client_socket) != this->_clients.end())
                {
                    // from isnt disconnected, we have to clear its packet 
                    from._cancelPacket();
                }
                
                // return false anyway because we have to break the client loop to avoid an invalid iterator
                // ( in Server<...>::wait_update() )
                return (false);
            }

            // handeled packet needs to be cleared
            from._cancelPacket();

            return (true);
        }



        /* ================================================ */
        /* Emit base                                        */
        /* ================================================ */

        template<typename T, typename std::size_t S = sizeof(T)>
        void    _emit_base(const std::string& message, const T& to_emit, client_type& client)
        {
            packed_data<S> pack = pack_data<T>(message, to_emit);
            char    data_buffer[sizeof(packed_data<S>)] = {0};

            std::memcpy(data_buffer, &pack, sizeof(data_buffer));
            client._data_to_send.push(std::string(data_buffer, sizeof(data_buffer)));
            FD_SET(client.getSocket(), &this->_write_fds);   
        }


        /* ================================================ */
        /* Send data                                        */
        /* ================================================ */

        bool    _send_data(client_type& client)
        {
            if (client._data_to_send.empty())
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "fd_set was set for sending on from " << client.getFullAddress() << " however no data is provider to send.")
                return false;
            }
            LOG_INFO(LOG_CATEGORY_NETWORK, "emitting to client on from " << client.getFullAddress());
#ifdef ENABLE_TLS
            ssize_t sent_bytes;
            if (client._is_ssl)
            {
                if (!client._accept_done)
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Attempting to emit on TLS client " << client._socket << " which is not yet accepted, setting emit for later...");
                    return true;
                }
                sent_bytes = SSL_write(client._ssl, client._data_to_send.top().c_str(), client._data_to_send.top().size());
            }
            else
                sent_bytes = send(client._socket, client._data_to_send.top().c_str(), client._data_to_send.top().size(), 0);
#else
            ssize_t sent_bytes = send(client._socket, client._data_to_send.top().c_str(), client._data_to_send.top().size(), 0);
#endif
            if (sent_bytes < 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Send from " << client.getFullAddress() << " failed with error: " << std::strerror(errno))
                throw server_type::SendException();
            }
            else if (sent_bytes == 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "sent 0 bytes of data from " << client.getFullAddress());
                return false;
            }
            else if ((size_t)sent_bytes != client._data_to_send.top().size())
            {
                // cropped data todo
                std::string left = client._data_to_send.top().substr(sent_bytes, client._data_to_send.top().length());
                client._data_to_send.pop();
                client._data_to_send.push(left);   
                LOG_INFO(LOG_CATEGORY_NETWORK, "Data sent from " << client.getFullAddress()<< " was cropped for socket " << client.getSocket() << ": " << client._data_to_send.top().length() << " bytes left to send");
                return false;
            }
            // sent full packet.
            client._data_to_send.pop();
            FD_CLR(client.getSocket(), &this->_write_fds);
            return (true);
        }


        /* ================================================ */
        /* Private utils                                    */
        /* ================================================ */

#define _MAX(a, b) ((a > b) ? a : b)
        int     _get_nfds()
        {
#ifdef ENABLE_TLS
            static int nfds = _MAX(_MAX(this->_socket, this->_socket6), _MAX(this->_socket, this->_ssl_socket));
#else
            static int nfds = _MAX(this->_socket, this->_socket6);
#endif
            static int n_clients = 0;

            // recalculate nfds only when a new client is connected
            if (this->n_clients_connected != n_clients)
            {
                n_clients = this->n_clients_connected;
#ifdef ENABLE_TLS
                nfds = _MAX(_MAX(this->_socket, this->_socket6), _MAX(this->_socket, this->_ssl_socket));
#else
                nfds = _MAX(this->_socket, this->_socket6);
#endif
                for (typename Server::client_list_type::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
                {
                    nfds = _MAX(nfds, it->second.getSocket());
                }
                // calculate nfds on this->_read_fds or this->_clients
            }
            return (nfds + 1);
        }


    /* ================================================ */
    /* Server Exceptions                                */
    /* ================================================ */

    public:
        class SocketException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("socket exception occured: abort");
                }
        };

        class BindException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("bind exception occured: abort");
                }
        };

        class ListenException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("listen exception occured: abort");
                }
        };

        class SelectException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("select exception occured: abort");
                }
        };

        class AcceptException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("accept exception occured: abort");
                }
        };

        class InetException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("inet exception occured: abort");
                }
        };

        class RecvException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("recv exception occured: abort");
                }
        };

        class SendException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("send exception occured: abort");
                }
        };
        
        class SSLInitException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("SSL cannot initialize: abort");
                }
        };

        class SSLCertException : std::exception
        {
            public:
                const char *what() const noexcept
                {
                    return ("SSL cannot load cert or key file: abort");
                }
        };
        


    /* ================================================ */
    /* Public attributes                                */
    /* ================================================ */

    public:
        std::string ip_address;
        std::string ip_address6;

        int         port;
#ifdef ENABLE_TLS
        int         ssl_port;

        std::string ssl_cert_file;
        std::string ssl_private_key_file;
#endif

        bool        running;
        int         n_clients_connected;
        int         max_connections;
        bool        enable_IPv6;




    /* ================================================ */
    /* Private attributes                               */
    /* ================================================ */

    private:

        handler_type    _client_handler;

        fd_set          _read_fds;
        fd_set          _write_fds;

        int             _socket;
        int             _socket6;
        sockaddr_in     _address;
        sockaddr_in6    _address6;

#ifdef ENABLE_TLS
        int             _ssl_socket;
        sockaddr_in     _ssl_address;
        bool            _ssl_only;

        SSL_CTX*            _ssl_ctx;
        const SSL_METHOD*   _ssl_method;
#endif

        client_list_type    _clients;

        msg_list_type       _messages_handlers;
};
