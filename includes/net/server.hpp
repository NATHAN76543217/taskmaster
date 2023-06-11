
#pragma once

#ifdef ENABLE_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <map>
#include <stack>
#include <list>

/* SocketsHandler implementation is specified for each handler type */
#if   defined(USE_SELECT)
# ifndef DISABLE_WARNINGS
#  warning "select() Socket Handler is deprecated: use poll/kqueue on BSD plateforms, and epoll for Linux"
# endif
# include "common/socket_handlers/SelectSocketHandler.hpp"
#elif defined(USE_KQUEUE)
class SocketsHandler {};
# error "Socket Handler not implemented for kqueue()"
#elif defined(USE_EPOLL)
class SocketsHandler {};
# error "Socket Handler not implemented for epoll()"
#else // use POLL by default
# ifndef  USE_POLL
#  define USE_POLL
# endif
# include "common/socket_handlers/PollSocketHandler.hpp"
#endif

#include "Tintin_reporter.hpp"

#include "common/PacketManager.hpp"
#include "common/dto_base.hpp"
#include "server/ServerClient.hpp"
#include "server/ServerClientsHandler.hpp"
#include "server/ServerEndpoint.hpp"

/* Example client Data */
class DefaultClientData
{
    // data per client
    // std::string name;
    // ...
};

/* Example clients handler */
class DefaultClientHandler : ServerClientsHandler<DefaultClientHandler, DefaultClientData>
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

        /* Server constructor for multiple endpoints                               */
        /* this call can throw if one of the endpoints contains an invalid address */
        /* and thus needs to be catched.                                           */
        /* If TLS is enabled, it can also throw a SSLInitException.                */
        Server(const std::list<ServerEndpoint>& endpoints) throw(std::logic_error)
        : endpoints(endpoints),
          running(false),
          n_clients_connected(0),
          max_connections(-1),
          _client_handler(*this)
#ifdef ENABLE_TLS
          ,_ssl_method(TLS_server_method())
#endif
        {            

#ifdef ENABLE_TLS
            SSL_library_init();

            this->_init_ssl_ctx();
#endif
            this->_client_handler.declareMessages();

            // FD_ZERO(&this->_read_fds);
            // FD_ZERO(&this->_write_fds);
        }



        /* Server constructor for single endpoint                                  */
        /* this call can throw if the endpoint contains an invalid address.        */
        /* If TLS is enabled, it can also throw a SSLInitException.                */
#ifdef ENABLE_TLS
        Server(const std::string& ip_address, const int port, const bool useTLS = false, const sa_family_t family = AF_INET)
        : Server(std::list<ServerEndpoint>(1, ServerEndpoint(ip_address, port, useTLS, family)))
#else
        Server(const std::string& ip_address, const int port, const sa_family_t family = AF_INET)
        : Server(std::list<ServerEndpoint>(1, ServerEndpoint(ip_address, port, family)))
#endif
        {}

        Server(const Server& copy) = delete;

        Server& operator=(const Server& other) = delete;


        ~Server()
        {
            this->shutdown();
            
#ifdef ENABLE_TLS
            SSL_CTX_free(this->_ssl_ctx);
#endif
        }


        /* ================================================ */
        /* Public Members                                   */
        /* ================================================ */

        /* Makes the server starts listening,                                          */
        /* after that, wait_update should be called to gather informations.            */
        /* This call can throw an exception if any of the specified endpoints cannot   */
        /* bind the specified address of that listen fails, when TLS is used, it loads */
        /* the certificates and can thus throw a SSLCertException                      */
        void    start_listening(int max_pending_connections = 10) throw(std::logic_error)
        {
#ifdef ENABLE_TLS
            bool enablesTLS = false;
#endif
            for (ServerEndpoint& ep : this->endpoints)
            {
#ifdef ENABLE_TLS
                if (ep.useTLS())
                    enablesTLS = true;
#endif
                ep.start_listening(max_pending_connections);
                // set endpoint to be selected 
                this->_sockets_handler.addSocket(ep);
                // struct pollfd endpoint_pollfd;
                // endpoint_pollfd.fd = ep.getSocket();
                // endpoint_pollfd.events = POLL_IN;
                // this->_pollfds.push_back(endpoint_pollfd);
                // FD_SET(ep.getSocket(), &this->_read_fds);
            }

#ifdef ENABLE_TLS
            if (enablesTLS)
                this->_load_ssl_certs();
#endif
            this->running = true;
        }

        /* Waits for an update on the read fds, needs to be called every time        */
        /* It will gather informations received on sockets and run clients handlers  */
        /* This function should be called after doing some operations on the clients */
        /* externally so that the data processed has less latency                    */
        /* It will also perform all the emits requested before its call              */
        /* It returns true if the server is still active after completion            */
        /* If specified, a timeout argument can be set to define the                 */
        /* timeout in ms for select                                                  */
        bool    wait_update(const int timeout_ms = -1) noexcept
        {
            if (!running)
                return (false);

            int received = 0;
            
            if (this->_sockets_handler.processPoll(timeout_ms))
            {
                if (errno == EINTR || errno == ENOMEM)
                    return (true);
                LOG_ERROR(LOG_CATEGORY_NETWORK, "sockets handler failed: " << strerror(errno));
                return (false);
            }

            // looking for each handeled events
            SocketsHandler::socket_event ev = this->_sockets_handler.nextSocketEvent();
            while (!EV_IS_END(ev))
            {
                // looking for new connections on endpoints
                bool is_endpoint = false;
                for (ServerEndpoint& ep : this->endpoints)
                {
                    if (!EV_IS_SOCKET(ev, ep.getSocket()))
                        continue ;
                    if (EV_IS_ERROR(ev))
                    {
                        LOG_ERROR(LOG_CATEGORY_NETWORK, "An error occurred on endpoint " << ep.getHostname());
                        this->_sockets_handler.delSocket(ep);
                        return true;
                    }
                    if (!EV_IS_READABLE(ev))
                        continue ;
                    received++;
    #ifdef ENABLE_TLS
                    if (ep.useTLS())
                        this->_accept_ssl(ep);
                    else 
    #endif
                    if (ep.getAddressFamily() == AF_INET)
                    {
                        this->_accept(ep);
                    }
                    else if (ep.getAddressFamily() == AF_INET6)
                    {
                        this->_accept6(ep);
                    }
                    is_endpoint = true;
                    break ;
                }

                if (is_endpoint)
                {
                    ev = this->_sockets_handler.nextSocketEvent();
                    continue;
                }

                // looking for clients received data
                try {
                    client_type& client = this->_clients.at(ev.socket);
                
                    if (EV_IS_ERROR(ev))
                    {
                        this->disconnect(client);
                        break ;
                    }
                    if (EV_IS_READABLE(ev))
                    {
                        if (this->_receive(client) == false)
                            break ; // sent disconnect
                        if (!running)
                            return (false); 
                    }
                    if (EV_IS_WRITABLE(ev))
                    {
                        this->_send_data(client);
                    }
                } catch (std::out_of_range& e)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "SocketsHandler returned an event with no recipient !!!");
                    return (true);
                }

                ev = this->_sockets_handler.nextSocketEvent();
            }

            return (running);
        }

        /* ================================================ */
        /* Server types                                     */
        /* ================================================ */
        typedef H                           handler_type;
        typedef typename H::client_type     client_type;
        typedef Server<H>                   server_type;
        typedef ServerEndpoint              endpoint_type;
        typedef std::list<ServerEndpoint>   endpoint_list_type;

        struct functor_handler
        {
            size_t  sizeof_type;
            void(*handle)(server_type&, client_type&, DTO*);
        };

        /* handler maker, returns a message_handler_type to be added in the _message_handlers,  */
        /* it auto calculates the size for the expected T value                                 */
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

        /* Adds a new message handler to the server, the message handler must be created          */
        /* with server_type::make_handler<T>(message_name, handler_lambda) where T is the         */
        /* expected data type for that message, message_name is the name of the message           */
        /* that should contain a valid data for T, and handler_lambda is a c++ lambda function    */
        /* taking in a reference to server_type, a reference to client_type which is a reference  */
        /* to the client which sent the message, and a DTO* which is a pointer to the begining of */
        /* the data received which needs to be reinterpreted in T, this can be made safely with   */
        /* reinterpret_cast<T*>(dto) since data size is checked before, however validity of the   */
        /* data fields must be checked in handler.                                                */
        void    onMessage(const std::string& name, message_handler_type handler) noexcept
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
        void    emit(const std::string& message, const T& to_emit) throw(InvalidPacketMessageNameException)
        { 
            for (client_type& client : this->_clients)
                this->_emit_base(message, to_emit);
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted for each clients");
        }
    
        /* emits T to the client specified as `client` */
        template<typename T>
        void    emit(const std::string& message, const T& to_emit, client_type& client) throw(InvalidPacketMessageNameException)
        { 
            this->_emit_base(message, to_emit, client);
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted to client from " << client.getHostname())
        }
        
        /* emit_now emits the same way emit does, however it doesn't wait for wait_update() to be sent   */
        /* this is usefull for errors or for emitting something before calling disconnect()              */
        /* however the client's socket might not be able to receive the packet at the moment it is sent  */
        /* in that case, the package is queued and emit_now returns false indicating a failure           */
        /* same thing if send wasn't able to send the packet entierely, emit_now returns false           */
        /* for that reason, it should be used to send small data structures to ensure data packages are  */
        /* completely sent.                                                                              */
        template<typename T>
        bool    emit_now(const std::string& message, const T& to_emit, client_type& client) throw(InvalidPacketMessageNameException)
        { 
            this->emit(message, to_emit, client);
            return this->_send_data(client);
        }

        /* emit_now to every client on the server */
        template<typename T>
        bool    emit_now(const std::string& message, const T& to_emit) throw(InvalidPacketMessageNameException)
        { 
            this->emit(message, to_emit);
            int emitted_all = 0;
            for (client_type& client : this->_clients)
                emitted_all += this->_send_data(client);
            return (emitted_all == this->_clients.size());
        }


        /* ================================================ */
        /* Other Utils                                      */
        /* ================================================ */

        /* disconnects the specified client from the server*/
        bool    disconnect(client_type& client)
        {
            int socket = client.getSocket();
            std::string address = client.getHostname();

#ifdef ENABLE_TLS
            if (client._useTLS && client._ssl_connection != nullptr)
            {
                SSL_free(client._ssl_connection);
                client._ssl_connection = nullptr;
            }
#endif

            this->_sockets_handler.delSocket(client);
            if (this->_clients.erase(socket) == 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "trying to disconnect unkown client with socket " << socket << " from address " << address)
                return false;
            }
            ::close(socket);
            this->n_clients_connected--;
            LOG_INFO(LOG_CATEGORY_NETWORK, "client from " << address << " disconnected");
            return (true);
        }

        /* shutdowns the server an closes all connections */
        void    shutdown()
        {
            for (typename client_list_type::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
                ::close(it->second.getSocket());

            for (const ServerEndpoint& ep : this->endpoints)
                ::close(ep.getSocket());
            this->running = false;
        }

        /* gets a specific client by socket id              */
        client_type&    getClient(const int socket)
        {
            return this->_clients.at(socket);
        }

        /* gets a const reference to the whole clients list */
        const client_list_type& getClients() const
        {
            return this->_clients;
        }







        /* ======================================================================================================== */
        /* PRIVATE MEMBERS                                                                                          */
        /* ======================================================================================================== */

    private:

#ifdef ENABLE_TLS
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
            if (this->ssl_private_key_file.empty() && ++no_cert_or_key)
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

        int     _accept(const ServerEndpoint& endpoint)
        {
            sockaddr_in     client_addr;
            socklen_t       len = sizeof(client_addr);
            std::memset(&client_addr, 0, len);
            int client_socket = ::accept(endpoint.getSocket(), (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed on endpoint " << endpoint.getHostname() << " with error: " << std::strerror(errno));
                return (-1);
            }
            
            try {
                InetAddress addr_info = InetAddress(client_addr);
                if (this->max_connections > 0 && this->n_clients_connected >= this->max_connections)
                {
                    LOG_INFO(LOG_CATEGORY_NETWORK, "Cannot accept more than " << this->max_connections << " connections, refusing new client from "  << addr_info.getHostname());
                    // cannot accept more clients
                    ::close(client_socket);
                    return (-1);
                }

                std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, addr_info)));
                if (insertion.second == false)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed on endpoint " << endpoint.getHostname() << " for client just arrived from "  << addr_info.getHostname());
                    ::close(client_socket);
                    return (-1);
                }

                this->n_clients_connected++;
                this->_sockets_handler.addSocket(insertion.first->second);
                LOG_INFO(LOG_CATEGORY_NETWORK, "New client connected on endpoint " << endpoint.getHostname() << " from "  << addr_info.getHostname());
                this->_client_handler.onConnected((*insertion.first).second);
                return (client_socket);

            } catch (std::exception& e)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Client creation failed for connection on IPv4: " << e.what());
                ::close(client_socket);
                return (-1);
            };
        }

        int     _accept6(const ServerEndpoint& endpoint)
        {
            sockaddr_in6    client_addr;
            socklen_t       len = sizeof(client_addr);
            std::memset(&client_addr, 0, len);
            int client_socket = ::accept(endpoint.getSocket(), (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed on IPv6 on endpoint " << endpoint.getHostname() << " with error: " << std::strerror(errno));
                return (-1);
            }
            
            try {
                InetAddress addr_info = InetAddress(client_addr);
                if (this->max_connections > 0 && this->n_clients_connected >= this->max_connections)
                {
                    LOG_INFO(LOG_CATEGORY_NETWORK, "Cannot accept more than " << this->max_connections << " connections, refusing new client from "  << addr_info.getHostname());
                    // cannot accept more clients
                    close (client_socket);
                    return (-1);
                }
                
                std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, addr_info)));
                if (insertion.second == false)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed on endpoint " << endpoint.getHostname() << " for client just arrived from "  << addr_info.getHostname());
                    ::close(client_socket);
                    return (-1);
                }

                this->n_clients_connected++;
                this->_sockets_handler.addSocket(insertion.first->second);
                LOG_INFO(LOG_CATEGORY_NETWORK, "New client connected on IPv6 on endpoint " << endpoint.getHostname() << " from "  << addr_info.getHostname());
                this->_client_handler.onConnected((*insertion.first).second);
                return (client_socket);
            }
            catch (std::exception &e)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Client creation failed for connection on IPv6: " << e.what());
                ::close(client_socket);
                return (-1);
            }
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
                    LOG_INFO(LOG_CATEGORY_NETWORK, "SSL accept incomplete, waiting for more data for handshake...");
                    return (0);
                }
                LOG_WARN(LOG_CATEGORY_NETWORK, "SSL accept handshake failed for client trying to connect from " << client.getHostname());
                ERR_print_errors_fp(stderr);
                return (-1);
            }
            client._accept_done = true;
            LOG_INFO(LOG_CATEGORY_NETWORK, "Upgraded connection to TLS for client connected from " << client.getHostname());
            this->_client_handler.onConnected(client);
            return (1);
        }

        int     _accept_ssl(const ServerEndpoint& endpoint)
        {
            sockaddr_in     client_addr;
            socklen_t       len = sizeof(client_addr);
            std::memset(&client_addr, 0, len);
            int client_socket = ::accept(endpoint.getSocket(), (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed on endpoint " << endpoint.getHostname() << " with error: " << std::strerror(errno));
                return (-1);
            }
           
            try {
                InetAddress addr_info = InetAddress(client_addr);
                if (this->max_connections > 0 && this->n_clients_connected >= this->max_connections)
                {
                    LOG_INFO(LOG_CATEGORY_NETWORK, "Cannot accept more than " << this->max_connections << " connections, refusing new client from "  << addr_info.getHostname());
                    // cannot accept more clients
                    ::close (client_socket);
                    return (-1);
                }

                SSL*    ssl = SSL_new(this->_ssl_ctx);
                SSL_set_fd(ssl, client_socket);
                SSL_set_accept_state(ssl);
                
                std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, addr_info, ssl)));
                if (insertion.second == false)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed on endpoint " << endpoint.getHostname() << " for client just arrived from " << addr_info.getHostname());
                    ::close(client_socket);
                    return (-1);
                }

                this->n_clients_connected++;
                this->_sockets_handler.addSocket(insertion.first->second);
                LOG_INFO(LOG_CATEGORY_NETWORK, "New client was accepted on TLS on endpoint " << endpoint.getHostname() << " from "  << addr_info.getHostname() << ", waiting for SSL handshake...");
                return (client_socket);
            }
            catch (std::exception &e)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Client creation failed for connection on TLS IPv4: " << e.what());
                ::close(client_socket);
                return (-1);
            }
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
            if (from._useTLS)
            {
                if (!from._accept_done)
                {
                    if (this->_ssl_do_accept(from) == -1)
                    {
                        this->disconnect(from);
                        return false;
                    }
                    return (true);
                }
                size = SSL_read(from._ssl_connection, buffer, RECV_BLK_SIZE);
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
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Recv failed for client from " << from.getHostname() << ": " << std::strerror(errno) << ", disconnecting client for safety.");
                this->disconnect(from);
                return (false);
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
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Cannot get data handler in parsed packet for message `" << from._received_packet.message_name << "` received on client from " << from.getHostname() << ", this should have been resolved before parsing the packet.");
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
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Invalid new packet received on client from " << from.getHostname() << " packet size is smaller than data_header size: cannot parse packet");
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
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Unknown data handler for message `" << message_name << "` received on client from " << from.getHostname());
                    return (true);
                }
                if (handler->second.sizeof_type != data_header.data_size)
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Packet size for message `" << message_name << "` received on client from " << from.getHostname() << " doesnt correspond with current message packet type definition: expected a data containing " 
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
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Parsed packet received on client from " << from.getHostname() << " size doesn't correspond to size in handler !!!");
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
        void    _emit_base(const std::string& message, const T& to_emit, client_type& client) throw(InvalidPacketMessageNameException)
        {
            packed_data<S> pack = pack_data<T>(message, to_emit);
            char    data_buffer[sizeof(packed_data<S>)] = {0};

            std::memcpy(data_buffer, &pack, sizeof(data_buffer));
            client._data_to_send.push(std::string(data_buffer, sizeof(data_buffer)));
            this->_sockets_handler.socketWantsWrite(client, true);
        }


        /* ================================================ */
        /* Send data                                        */
        /* ================================================ */

        // Sends the data queued for client, returns true if data was flushed entierly.
        bool    _send_data(client_type& client)
        {
            if (client._data_to_send.empty())
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "fd_set was set for sending for client from " << client.getHostname() << " however no data is provider to send.")
                this->_sockets_handler.socketWantsWrite(client, false);
                return false;
            }
            LOG_INFO(LOG_CATEGORY_NETWORK, "emitting to client from " << client.getHostname());
#ifdef ENABLE_TLS
            ssize_t sent_bytes;
            if (client._useTLS)
            {
                if (!client._accept_done)
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Attempting to emit to TLS client from " << client.getHostname() << " which is not yet accepted, setting emit for later...");
                    return false;
                }
                sent_bytes = SSL_write(client._ssl_connection, client._data_to_send.top().c_str(), client._data_to_send.top().size());
            }
            else
                sent_bytes = send(client.getSocket(), client._data_to_send.top().c_str(), client._data_to_send.top().size(), 0);
#else
            ssize_t sent_bytes = send(client.getSocket(), client._data_to_send.top().c_str(), client._data_to_send.top().size(), 0);
#endif
            if (sent_bytes < 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Send to client from " << client.getHostname() << " failed with error: " << std::strerror(errno))
                client._data_to_send.pop();
                if (client._data_to_send.empty())
                    this->_sockets_handler.socketWantsWrite(client, false);
                return false;
            }
            else if (sent_bytes == 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "sent 0 bytes of data to client from " << client.getHostname());
                return false;
            }
            else if ((size_t)sent_bytes != client._data_to_send.top().size())
            {
                // cropped data todo
                std::string left = client._data_to_send.top().substr(sent_bytes, client._data_to_send.top().length());
                client._data_to_send.pop();
                client._data_to_send.push(left);   
                LOG_INFO(LOG_CATEGORY_NETWORK, "Data sent to client from " << client.getHostname()<< " was cropped: " << client._data_to_send.top().length() << " bytes left to send");
                return false;
            }
            // sent full packet.
            client._data_to_send.pop();
            if (client._data_to_send.empty())
                this->_sockets_handler.socketWantsWrite(client, false);
            return (true);
        }


    /* ================================================ */
    /* Server Exceptions                                */
    /* ================================================ */

    public:
        
        class SSLInitException : std::logic_error
        {
            public:
                SSLInitException() : std::logic_error("SSL cannot initialize: abort") {}
        };

        class SSLCertException : std::logic_error
        {
            public:
                SSLCertException() : std::logic_error("SSL cannot load cert or key file: abort") {}
        };
        

    /* ================================================ */
    /* Public attributes                                */
    /* ================================================ */

    public:
        endpoint_list_type  endpoints;

#ifdef ENABLE_TLS
        std::string ssl_cert_file;
        std::string ssl_private_key_file;
#endif

        bool        running;
        int         n_clients_connected;
        int         max_connections;


    /* ================================================ */
    /* Private attributes                               */
    /* ================================================ */

    private:

        handler_type        _client_handler;

        SocketsHandler      _sockets_handler;

#ifdef ENABLE_TLS
        SSL_CTX*            _ssl_ctx;
        const SSL_METHOD*   _ssl_method;
#endif

        client_list_type    _clients;

        msg_list_type       _messages_handlers;
};
