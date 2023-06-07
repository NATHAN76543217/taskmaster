
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
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

// system select implementation may not implement FD_COPY
#ifndef FD_COPY
#define FD_COPY(src, dst) std::memcpy(dst, src, sizeof(*(src)))
#endif

#include "Tintin_reporter.hpp"

#include "common/packet.hpp"
#include "common/dto_base.hpp"
#include "server/ServerClient.hpp"
#include "server/ServerClientsHandler.hpp"
#include "server/ServerEndpoint.hpp"

class DefaultClientData
{
    // data per client
    // std::string name;
    // ...
};

/* Example client handler */
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


        Server(const std::list<ServerEndpoint>& endpoints)
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

            FD_ZERO(&this->_read_fds);
            FD_ZERO(&this->_write_fds);
        }


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
            for (typename client_list_type::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
                ::close(it->second.getSocket());

            for (const ServerEndpoint& ep : this->endpoints)
                ::close(ep.getSocket());
            
#ifdef ENABLE_TLS
            SSL_CTX_free(this->_ssl_ctx);
#endif
        }


        /* ================================================ */
        /* Public Members                                   */
        /* ================================================ */

        /* Makes the server starts listening,                                       */
        /* after that, wait_update should be called to gather informations.         */
        void    start_listening(int max_pending_connections = 10)
        {
#ifdef ENABLE_TLS
            bool enablesTLS = false;
#endif
            for (const ServerEndpoint& ep : this->endpoints)
            {
#ifdef ENABLE_TLS
                if (ep.useTLS())
                    enablesTLS = true;
#endif
                ep.start_listening(max_pending_connections);
                // set endpoint to be selected 
                FD_SET(ep.getSocket(), &this->_read_fds);
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
                if (errno == EINTR || errno == ENOMEM)
                    return (true);
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

            for (const ServerEndpoint& ep : this->endpoints)
            {
                if (FD_ISSET(ep.getSocket(), &selected_read_fds))
                {
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
                }
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
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted to client from " << client.getHostname())
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
            } catch (SendException& e)
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
            } catch (SendException& e)
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
            std::string address = client.getHostname();

#ifdef ENABLE_TLS
            if (client._is_ssl && client._ssl_connection != nullptr)
            {
                SSL_free(client._ssl_connection);
                client._ssl_connection = nullptr;
            }
#endif

            if (this->_clients.erase(socket) == 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "trying to disconnect unkown client with socket " << socket << " from address " << address)
                return false;
            }
            FD_CLR(socket, &this->_read_fds);
            ::close(socket);
            this->n_clients_connected--;
            LOG_INFO(LOG_CATEGORY_NETWORK, "client from " << address << " disconnected");
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
            socklen_t       len;
            int client_socket = ::accept(endpoint.getSocket(), (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed on endpoint " << endpoint.getHostname() << " with error: " << std::strerror(errno));
                throw AcceptException();
            }
            if (this->max_connections > 0 && this->n_clients_connected >= this->max_connections)
            {
                LOG_INFO(LOG_CATEGORY_NETWORK, "Cannot accept more than " << this->max_connections << " connections, refusing new client from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
                // cannot accept more clients
                ::close(client_socket);
                return (-1);
            }
            
            try {

                std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, client_addr)));
                if (insertion.second == false)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed on endpoint " << endpoint.getHostname() << " for client just arrived from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
                    ::close(client_socket);
                    return (-1);
                }

                this->n_clients_connected++;
                FD_SET(client_socket, &this->_read_fds);
                LOG_INFO(LOG_CATEGORY_NETWORK, "New client connected on endpoint " << endpoint.getHostname() << " from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
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
            socklen_t       len;
            int client_socket = ::accept(endpoint.getSocket(), (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed on IPv6 on endpoint " << endpoint.getHostname() << " with error: " << std::strerror(errno));
                throw AcceptException();
            }

            char address6[INET6_ADDRSTRLEN];
            if (inet_ntop(AF_INET6, &client_addr.sin6_addr, address6, sizeof(client_addr.sin6_addr)) == NULL)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Cannot parse sockaddr_in6 received on IPv6 endpoint " << endpoint.getHostname() << ": " << std::strerror(errno));
                ::close(client_socket);
                return (true);
            }
            std::string client_ip = std::string(address6, std::strlen(address6));

            if (this->max_connections > 0 && this->n_clients_connected >= this->max_connections)
            {
                LOG_INFO(LOG_CATEGORY_NETWORK, "Cannot accept more than " << this->max_connections << " connections, refusing new client from "  << client_ip << ":" << ntohs(client_addr.sin6_port));
                // cannot accept more clients
                close (client_socket);
                return (-1);
            }

            try {
                std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, client_addr)));
                if (insertion.second == false)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed on endpoint " << endpoint.getHostname() << " for client just arrived from "  << client_ip << ":" << ntohs(client_addr.sin6_port));
                    ::close(client_socket);
                    return (-1);
                }

                this->n_clients_connected++;
                FD_SET(client_socket, &this->_read_fds);
                LOG_INFO(LOG_CATEGORY_NETWORK, "New client connected on IPv6 on endpoint " << endpoint.getHostname() << " from "  << client_ip << ":" << ntohs(client_addr.sin6_port));
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
                LOG_WARN(LOG_CATEGORY_NETWORK, "SSL accept handshake failed client trying to connect from " << client.getHostname());
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
            socklen_t       len;
            int client_socket = ::accept(endpoint.getSocket(), (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Accept failed on endpoint " << endpoint.getHostname() << " with error: " << std::strerror(errno));
                throw AcceptException();
            }
            if (this->max_connections > 0 && this->n_clients_connected >= this->max_connections)
            {
                LOG_INFO(LOG_CATEGORY_NETWORK, "Cannot accept more than " << this->max_connections << " connections, refusing new client from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
                // cannot accept more clients
                close (client_socket);
                return (-1);
            }

            SSL*    ssl = SSL_new(this->_ssl_ctx);
            SSL_set_fd(ssl, client_socket);
            SSL_set_accept_state(ssl);

            try {
                std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket, client_addr, ssl)));
                if (insertion.second == false)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Client insertion in std::map failed on endpoint " << endpoint.getHostname() << " for client just arrived from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
                    ::close(client_socket);
                    return (-1);
                }

                this->n_clients_connected++;
                FD_SET(client_socket, &this->_read_fds);
                LOG_INFO(LOG_CATEGORY_NETWORK, "New client was accepted on TLS on endpoint " << endpoint.getHostname() << " from "  << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << ", waiting for SSL handshake...");
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
            if (from._is_ssl)
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
                LOG_ERROR(LOG_CATEGORY_NETWORK, "fd_set was set for sending on client from " << client.getHostname() << " however no data is provider to send.")
                FD_CLR(client.getSocket(), &this->_write_fds);
                return false;
            }
            LOG_INFO(LOG_CATEGORY_NETWORK, "emitting to client on client from " << client.getHostname());
#ifdef ENABLE_TLS
            ssize_t sent_bytes;
            if (client._is_ssl)
            {
                if (!client._accept_done)
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Attempting to emit on TLS client from " << client.getHostname() << " which is not yet accepted, setting emit for later...");
                    return true;
                }
                sent_bytes = SSL_write(client._ssl_connection, client._data_to_send.top().c_str(), client._data_to_send.top().size());
            }
            else
                sent_bytes = send(client._socket, client._data_to_send.top().c_str(), client._data_to_send.top().size(), 0);
#else
            ssize_t sent_bytes = send(client._socket, client._data_to_send.top().c_str(), client._data_to_send.top().size(), 0);
#endif
            if (sent_bytes < 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Send from client from " << client.getHostname() << " failed with error: " << std::strerror(errno))
                throw server_type::SendException();
            }
            else if (sent_bytes == 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "sent 0 bytes of data from " << client.getHostname());
                return false;
            }
            else if ((size_t)sent_bytes != client._data_to_send.top().size())
            {
                // cropped data todo
                std::string left = client._data_to_send.top().substr(sent_bytes, client._data_to_send.top().length());
                client._data_to_send.pop();
                client._data_to_send.push(left);   
                LOG_INFO(LOG_CATEGORY_NETWORK, "Data sent from client from " << client.getHostname()<< " was cropped: " << client._data_to_send.top().length() << " bytes left to send");
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
            static int nfds = 0;
            static int n_clients = -1;

            // recalculate nfds only when a new client is connected
            if (this->n_clients_connected != n_clients)
            {
                n_clients = this->n_clients_connected;
                for (const ServerEndpoint &ep : this->endpoints)
                {
                    nfds = _MAX(nfds, ep.getSocket());
                }
                for (typename Server::client_list_type::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
                {
                    nfds = _MAX(nfds, it->second.getSocket());
                }
            }
            return (nfds + 1); 
        }


    /* ================================================ */
    /* Server Exceptions                                */
    /* ================================================ */

    public:
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
        const endpoint_list_type    endpoints;

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

        handler_type    _client_handler;

        fd_set          _read_fds;
        fd_set          _write_fds;

#ifdef ENABLE_TLS
        SSL_CTX*            _ssl_ctx;
        const SSL_METHOD*   _ssl_method;
#endif

        client_list_type    _clients;

        msg_list_type       _messages_handlers;
};
