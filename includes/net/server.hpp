
#pragma once

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
    public:
        ServerClient(int socket)
            : D(), _socket(socket)
            {}

        int     getSocket() const { return this->_socket; }
    
    private:
        int _socket;

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

        virtual void    onConnected(client_type& client, const struct sockaddr_in& client_address) { (void)client; (void)client_address; };
        virtual void    onConnectedIPv6(client_type& client, const struct sockaddr_in6& client_address) { (void)client; (void)client_address; };
        // super.onDisconnected() must be called if overrided
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
    std::string name;
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
        : ip_address(ip_address), ip_address6(ip_address6), port(port), running(false), enable_IPv6(!ip_address6.empty()), _client_handler(*this)
        {
            this->_address = (sockaddr_in){
                .sin_family = AF_INET,
                .sin_port = htons(static_cast<in_port_t>(this->port)),
            };
            if (inet_aton(this->ip_address.c_str(), &this->_address.sin_addr) == -1)
                throw InetException();

            if (this->enable_IPv6)
            {
                this->_address6 = (sockaddr_in6){
                    .sin6_family = AF_INET6,
                    .sin6_port = htons(static_cast<in_port_t>(this->port + 1)),
                };
                if (inet_pton(AF_INET6, this->ip_address6.c_str(), &this->_address6) == -1)
                    throw InetException();
            }

            FD_ZERO(&this->_read_fds);
            FD_ZERO(&this->_write_fds);
        }

        Server(const Server& copy) = delete;

        Server& operator=(const Server& other) = delete;


        /* ================================================ */
        /* Public Members                                   */
        /* ================================================ */

        /* Makes the server starts listening,                                       */
        /* after that, wait_update should be called to gather informations.         */
        void    start_listening(int max_pending_connections = 10)
        {
            this->_init_socket4(max_pending_connections);

            if (enable_IPv6)
            {
                this->_init_socket6(max_pending_connections);
            }

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
        bool    wait_update()
        {
            if (!running)
                return (false);
            // preserve main fd_set's
            fd_set selected_read_fds = {0};
            fd_set selected_write_fds = {0};
            FD_COPY(&this->_read_fds, &selected_read_fds);
            FD_COPY(&this->_write_fds, &selected_write_fds);
            
            int nfds = this->_get_nfds();
            //struct timeval timeout = {.tv_sec=10,.tv_usec=0};
            if (select(nfds, &selected_read_fds, &selected_write_fds, NULL, (struct timeval*)NULL) == -1)
                throw SelectException();

            // look for clients receive
            for (typename Server::client_list_type::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
            {
                // received from client
                if (FD_ISSET(it->second.getSocket(), &selected_read_fds))
                {
                    if (_receive(it->second) != 0)
                        break ; // sent disconnect
                    if (!running)
                        return (false); 
                }
                // writing enabled for client, can send data
                if (FD_ISSET(it->second.getSocket(), &selected_write_fds))
                {
                    _send(it->second);
                }
            }

            // look for connections on ipv4 
            if (FD_ISSET(this->_socket, &selected_read_fds))
            {
                // client tries to connect
                this->_accept();
            }

            // look for connections on ipv6 
            if (FD_ISSET(this->_socket6, &selected_read_fds))
            {
                // client tries to connect
                this->_accept6();
            }

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
            void(*handler)(server_type&, client_type&, DTO*);
        };

        template<typename T>
        static functor_handler    make_handler(void(*handler_function)(server_type&, client_type&, DTO*))
        {
            functor_handler handler;
            handler.sizeof_type = sizeof(T);
            handler.handler = handler_function;
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
                std::cout << "cannot add handler for `" << name << "` message: name is too long (" 
                    << sizeof(packed_data_header<0>::message_name) << " max)" << std::endl; 
                return ;           
            }
            this->_messages_handlers.insert(std::make_pair(name, handler));
            std::cout << "added new handler for `" << name << "` message" << std::endl;
        }


        /* ================================================ */
        /* Emits                                   */
        /* ================================================ */

        // TODO FOR ALL EMIT : T should be serializable with the << operator and deserializable using a specified copy constructor (i.e. see serialization in c++)
        //                     it should also be checked with a std::enable_if<std::is_serializable<T>::value, ___>::type

        /* emits T to all clients connected as ServerClient */
        template<typename T, typename std::size_t S = sizeof(T)>
        void    emit(const std::string& message, const T& to_emit)
        { /* todo */ (void)message; (void)to_emit; }
    
        /* emits T to the client specified as `client` */
        template<typename T, typename std::size_t S = sizeof(T)>
        void    emit(const std::string& message, const T& to_emit, client_type& client)
        { 
            packed_data<S> pack = pack_data<T>(message, to_emit);
            char    data_buffer[sizeof(packed_data<S>)] = {0};

            serialize(pack, (uint8_t*)data_buffer);
            client._data_to_send.push(std::string(data_buffer, sizeof(data_buffer)));
            FD_SET(client.getSocket(), &this->_write_fds);            
            std::cout << "set `" << message << "` to be emitted to socket " << client.getSocket() << std::endl;
        }



        /* ================================================ */
        /* Other Utils                                      */
        /* ================================================ */

        /* disconnects the specified client from the server*/
        bool    disconnect(client_type& client)
        {
            int socket = client.getSocket();
            if (this->_clients.erase(socket) == 0)
            {
                std::cout << "trying to disconnect unkown client from socket " << socket << std::endl;
                return false;
            }
            FD_CLR(socket, &this->_read_fds);
            ::close(socket);
            this->n_clients_connected--;
            std::cout << "client disconnected from socket " << socket << std::endl;
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

        void    _init_socket4(const int max_pending_connections)
        {
            this->_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (this->_socket <= 0)
                throw SocketException();
            
            if (bind(this->_socket, (sockaddr*)(&this->_address), sizeof(this->_address)) != 0)
                throw BindException();
            
            if (listen(this->_socket, max_pending_connections) != 0)
                throw ListenException();
            
            FD_SET(this->_socket, &this->_read_fds);
            std::cout << "Server started listening on address " << this->ip_address << " on port " << this->port << std::endl;
        }

        void    _init_socket6(const int max_pending_connections)
        {
            this->_socket6 = socket(AF_INET6, SOCK_STREAM, 0);
            if (this->_socket6 <= 0)
                throw SocketException();
            
            if (bind(this->_socket6, (sockaddr*)(&this->_address6), sizeof(this->_address6)) != 0)
                throw BindException();
            
            if (listen(this->_socket6, max_pending_connections) != 0)
                throw ListenException();
            
            FD_SET(this->_socket6, &this->_read_fds);
            std::cout << "Server started listening on IPv6 on address " << this->ip_address6 << " on port " << this->port << std::endl;
        }


        /* ================================================ */
        /* Accept Handlers                                  */
        /* ================================================ */

        int     _accept()
        {
            sockaddr_in     client_addr;
            socklen_t       len;
            int client_socket = ::accept(this->_socket, (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
                throw AcceptException();
            this->n_clients_connected++;
            
            std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket)));
            if (insertion.second == false)
            {
                std::cerr << "client insertion failed" << std::endl;
                throw AcceptException();
            }

            FD_SET(client_socket, &this->_read_fds);
            std::cout << "new client connected on socket " << client_socket << std::endl;
            this->_client_handler.onConnected((*insertion.first).second, client_addr);
            return (client_socket);
        }

        int     _accept6()
        {
            sockaddr_in6    client_addr;
            socklen_t       len;
            int client_socket = ::accept(this->_socket6, (sockaddr*)&client_addr, &len);
            if (client_socket <= 0)
                throw AcceptException();
            this->n_clients_connected++;
            std::pair<typename Server::client_list_type::iterator, bool> insertion = this->_clients.insert(std::make_pair(client_socket, client_type(client_socket)));
            if (insertion.second == false)
            {
                std::cerr << "client insertion failed" << std::endl;
                throw AcceptException();
            }

            FD_SET(client_socket, &this->_read_fds);
            std::cout << "new client connected on IPv6 on socket " << client_socket << std::endl;
            this->_client_handler.onConnectedIPv6((*insertion.first).second, client_addr);
            return (client_socket);
        }


        /* ================================================ */
        /* Recv handler & unpacker                          */
        /* ================================================ */

// must at least be sizeof(packet_data_header) (or sizeof(size_t) + 32)
#define RECV_BLK_SIZE   sizeof(packed_data_header<0>) + 3
        struct ExampleType
        {
            int a;
            int b;
            char name[23];
        };

        // todo proper error handeling
        int     _receive(client_type& from)
        {
            uint8_t buffer[RECV_BLK_SIZE] = {0};
            ssize_t size = recv(from.getSocket(), buffer, RECV_BLK_SIZE, MSG_DONTWAIT);
            if (size == 0)
            {
                this->disconnect(from);
                return (1);
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
                    return (0); // packet is incomplete, or error, needs more data chunks
                // packet complete
                handler = this->_messages_handlers.find(from._received_packet.message_name);
                if (handler == this->_messages_handlers.end())
                {
                    this->_client_handler.onMessageMismatch(from, from._received_packet.message_name);
                    std::cerr << "Unknown data handler for message: `" << from._received_packet.message_name << "`" << std::endl;
                    from._cancelPacket();
                    return 0;
                }
            }
            // no packet defined in client, expecting a header for inserting a new one
            else
            {
                if ((size_t)size < sizeof(packed_data_header<0>))
                {
                    std::cerr << "invalid new packet size is smaller than data_header size: cannot parse packet" << std::endl;
                    return 0;
                }

                // copy header from buffer
                packed_data_header<0>  data_header;
                std::memcpy(&data_header, buffer, sizeof(packed_data_header<0>));
                data_header.message_name[sizeof(data_header.message_name) - 1] = 0;

                // check message validity
                std::string message_name(data_header.message_name, std::strlen(data_header.message_name));
                handler = this->_messages_handlers.find(message_name);
                if (handler == this->_messages_handlers.end())
                {
                    this->_client_handler.onMessageMismatch(from, message_name);
                    std::cerr << "Unknown data handler for message: `" << message_name << "`" << std::endl;
                    return 0;
                }
                if (handler->second.sizeof_type != data_header.data_size)
                {
                    std::cerr << "Packet size for message `" << message_name << "` doesnt correspond with current message packet type definition: expected a data containing " 
                    << handler->second.sizeof_type << " bytes but header specifies " << data_header.data_size << " data bytes" << std::endl;
                    return (0);
                }

                // create new packet in client _received_packet in case of incomplete packet
                int new_packet_result = from._newPacket(data_header, buffer, size);
                if (new_packet_result < 0)
                    return (0); // error on packet.
                else if (new_packet_result == 1) // packet is incomplete, needs more data chunks
                    return (0);
                // packet complete
            }

            // packet handling
        
            // this should never happen but must be here for security
            if (handler->second.sizeof_type != from._received_packet.data_size)
            {
                std::cerr << "Parsed packet size doesn't correspond to size in handler !!!" << std::endl;
                from._cancelPacket();
                return (0);
            }

            // get previous client count and socket of handeled client
            int clients_count = this->n_clients_connected;
            int client_socket = from.getSocket();

            uint8_t data_buffer[handler->second.sizeof_type];
            std::memcpy(data_buffer, from._received_packet.data.c_str(), handler->second.sizeof_type);
            handler->second.handler(*this, from, reinterpret_cast<DTO*>(data_buffer));

            // client list has changed in handler, in that case, from may have become invalid
            if (clients_count != this->n_clients_connected)
            {
                // we can try to see if the client that was disconnected was from by checking its socket in the list
                if (this->_clients.find(client_socket) != this->_clients.end())
                {
                    // from isnt disconnected, we have to clear its packet 
                    from._cancelPacket();
                }
                
                // return 1 anyway because we have to break the loop to avoid an invalid iterator
                return (1);
            }

            // handeled packet needs to be cleared
            from._cancelPacket();

            return (0);
        }




        void    _send(client_type& to)
        {
            if (to._data_to_send.empty())
            {
                std::cerr << "fd_set was set for sending however no data is provider to send." << std::endl;
                return ;
            }
            std::cout << "emitting to client on socket " << to.getSocket() << std::endl;
            ssize_t sent_bytes = send(to._socket, to._data_to_send.top().c_str(), to._data_to_send.top().size(), 0);
            if (sent_bytes < 0)
                throw server_type::SendException();
            else if (sent_bytes == 0)
            {
                std::cerr << "fd_set was set for sending however no data is provider to send." << std::endl;
                return ;
            }
            else if ((size_t)sent_bytes != to._data_to_send.top().size())
            {
                // cropped data todo
                std::cerr << "sent data was cropped" << std::endl;
                std::string left = to._data_to_send.top().substr(sent_bytes, to._data_to_send.top().length());
                to._data_to_send.pop();
                to._data_to_send.push(left);   
                return ;
            }
            // sent full packet.
            to._data_to_send.pop();
            FD_CLR(to.getSocket(), &this->_write_fds);
        }


        /* ================================================ */
        /* Private utils                                    */
        /* ================================================ */

#define _MAX(a, b) ((a > b) ? a : b)
        int     _get_nfds()
        {
            static int nfds = _MAX(this->_socket, this->_socket6);
            static int n_clients = 0;

            // recalculate nfds only when a new client is connected
            if (this->n_clients_connected != n_clients)
            {
                n_clients = this->n_clients_connected;
                nfds = _MAX(this->_socket, this->_socket6);

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
        


    /* ================================================ */
    /* Public attributes                                */
    /* ================================================ */

    public:
        std::string ip_address;
        std::string ip_address6;
        int         port;

        bool        running;
        int         n_clients_connected;
        int         enable_IPv6;


    /* ================================================ */
    /* Private attributes                               */
    /* ================================================ */

    private:

        H               _client_handler;

        fd_set          _read_fds;
        fd_set          _write_fds;

        int             _socket;
        int             _socket6;
        sockaddr_in     _address;
        sockaddr_in6    _address6;

        client_list_type    _clients;

        msg_list_type       _messages_handlers;
};