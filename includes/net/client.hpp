#pragma once

#ifdef ENABLE_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include <cstring>
#include <cstdlib>
#include <string>
#include <stack>
#include <map>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "Tintin_reporter.hpp"

// system select implementation may not implement FD_COPY
#ifndef FD_COPY
#define FD_COPY(src, dst) std::memcpy(dst, src, sizeof(*(src)))
#endif



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

#include "common/TcpSocket.hpp"
#include "common/PacketManager.hpp"
#include "common/dto_base.hpp"
#include "client/ClientHandler.hpp"

class DefaultClientData
{
    std::string server_name;
    // ...
};

/* Example client handler */
class DefaultClientHandler : ClientHandler<DefaultClientHandler, DefaultClientData>
{
    // struct ExampleDto : DTO
    // {
    //     int     data1;
    //     long    data2;
    //     char    str_data[128];
    // };

    // void declareMessages()
    // {
    //     this->client.onMessage("example", server_type::make_handler<ExampleDto>(
    //         [](client_type& client, DTO* obj)
    //         {
    //             // retrieve dto type 
    //             ExampleDto  *dto = reinterpret_cast<ExampleDto*>(obj);

    //             std::cout << "in example handler" << std::endl;
    //             // ...
    //         }
    //     ));
    // }
};



template<typename H>
class Client : public H::client_data_type, public PacketManager, public TcpSocket
{
    public:
        typedef Client                          client_type;
        typedef H                               handler_type;
        typedef typename H::client_data_type    data_type;

    public:
        /* ================================================ */
        /* Client constructor                               */
        /* ================================================ */

        /* Client constructor, can throw an exception if          */
        /* the specified informations cannot form a valid address */
        Client(const std::string& server_ip, const short server_port) throw (std::logic_error)
        : data_type(),
          PacketManager(server_ip, server_port, AF_INET),
          TcpSocket(this->getAddressFamily()),
          connected(false),
#ifdef ENABLE_TLS
          _useTLS(false),
          _ssl_method(TLS_client_method()),
          _handshake_done(false),
#endif
          _handler(*this)
        {            
            this->_handler.declareMessages();
        }

#ifdef ENABLE_TLS
        ~Client()
        {
            SSL_CTX_free(this->_ssl_ctx);
        }


        /* ================================================ */
        /* Public methods                                   */
        /* ================================================ */


        /* set client in TLS mode, and initializes an ssl context, in that mode, */
        /* client will try to do a handshake with the server upon connection.    */
        /* this call can throw a SSLInitException                                */
        void    enableTLS() throw(SSLInitException)
        {
            this->_useTLS = true;
            this->_init_ssl_ctx();
        }
#endif

        /* connects the client to the server                                  */
        /* this call can throw multiple exceptions, if the socket could not   */
        /* be created a SocketException is thrown, if the client is unable to */
        /* connect, a ConnectException is thrown.                             */
        void    connect()
        {
            if (::connect(this->getSocket(), reinterpret_cast<sockaddr*>(&this->_address_4), sizeof(this->_address_4)) != 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Can't connect to server: " << std::strerror(errno));
                throw ConnectException();
            }
            this->_socket_handler.addSocket(*this);
            this->connected = true;
#ifdef ENABLE_TLS
            if (this->_useTLS)
            {
                // set socket for writing to send ssl handshake
                this->_socket_handler.socketWantsWrite(*this, true);

                // create a new SSL connection instance
                this->_ssl_connection = SSL_new(this->_ssl_ctx);
                SSL_set_fd(this->_ssl_connection, this->getSocket());
                SSL_set_connect_state(this->_ssl_connection);
            }
            else
            {
                this->_handler.onConnected();
            }
#else // connection is set later for ssl handshake to take place
            
            this->_handler.onConnected();
#endif 
            LOG_INFO(LOG_CATEGORY_NETWORK, "Client connected on host " << this->getHostname() << " on port " << this->_port);
        }



        /* waits an update from the server, in the client, emits are sent on the wait_update call too */
        /* returns true if the connection is still active after handling                              */
        /* If specified, a timeout argument can be set to define the timeout in ms for select         */
        bool    wait_update(const int timeout_ms = -1)
        {
            if (this->_socket_handler.processPoll(timeout_ms))
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Sockets handler failed with error: " << std::strerror(errno));
                return (true);
            }

            SocketsHandler::socket_event ev = this->_socket_handler.nextSocketEvent();
            while (!EV_IS_END(ev))
            {
                if (EV_IS_SOCKET(ev, this->getSocket()))
                {
                    if (EV_IS_ERROR(ev))
                    {
                        this->disconnect();
                    }
                    if (EV_IS_READABLE(ev))
                    {
                        if (_receive() != true)
                            return (false); // disconnected from server
                    }
                    if (EV_IS_WRITABLE(ev))
                    {
                        this->_send_data();
                    }
                    break ;
                }
            }
            return (this->connected);
        }


        /* ================================================ */
        /* Messages management                              */
        /* ================================================ */

        /* Message list type definition */
        struct functor_handler
        {
            size_t  sizeof_type;
            void(*handle)(client_type&, DTO*);
        };

        /* handler maker, returns a message_handler_type to be added in the _message_handlers,  */
        /* it auto calculates the size for the expected T value                                 */
        template<typename T>
        static functor_handler    make_handler(void(*handler_function)(client_type&, DTO*))
        {
            functor_handler handler;
            handler.sizeof_type = sizeof(T);
            handler.handle = handler_function;
            return (handler);
        }

        typedef functor_handler                             message_handler_type;
        typedef std::map<std::string, message_handler_type> msg_list_type;

        /* Adds a new message handler to the client, the message handler must be created          */
        /* with client_type::make_handler<T>(message_name, handler_lambda) where T is the         */
        /* expected data type for that message, message_name is the name of the message           */
        /* that should contain a valid data for T, and handler_lambda is a c++ lambda function    */
        /* taking in a reference to client_type, and a DTO* which is a pointer to the begining of */
        /* the data received which needs to be reinterpreted in T, this can be made safely with   */
        /* reinterpret_cast<T*>(dto) since data size is checked before, however validity of the   */
        /* data fields must be checked in handler.                                                */
        void    onMessage(const std::string& name, const message_handler_type& handler)
        {
            // todo see properly why sizeof(packed_data_header<0>::message_name) is not accessible in client 
            // but accessible in server
            if (name.length() > (sizeof(packed_data_header<0>) - sizeof(size_t)))
            {
                LOG_ERROR(LOG_CATEGORY_INIT, "cannot add handler for `" << name << "` message: name is too long (" 
                    << (sizeof(packed_data_header<0>) - sizeof(size_t)) << " max)");  
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

        /* queues message to emit T to the server                                          */
        /* Can throw an InvalidPacketMessageNameException if `message` field is not valid. */
        template<typename T, typename std::size_t S = sizeof(T)>
        void    emit(const std::string& message, const T& data) throw (InvalidPacketMessageNameException)
        {
            this->_emit_base(message, data);
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted to the server");
        }

        /* emit_now emits the same way emit does, however it doesn't wait for wait_update() to be sent   */
        /* this is usefull for errors or for emitting something before calling disconnect()              */
        /* however the server's socket might not be able to receive the packet at the moment it is sent  */
        /* in that case, the package is queued and emit_now returns false indicating a failure           */
        /* same thing if send wasn't able to send the packet entierely, emit_now returns false           */
        /* for that reason, it should be used to send small data structures to ensure data packages are  */
        /* completely sent.                                                                              */
        /* Can throw an InvalidPacketMessageNameException if `message` field is not valid.               */
        template<typename T, typename std::size_t S = sizeof(T)>
        bool    emit_now(const std::string& message, const T& data) throw (InvalidPacketMessageNameException)
        {
            this->_emit_base(message, data);
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted now to the server");
            return this->_send_data();
        }
        
        /* ================================================ */
        /* Other utils                                      */
        /* ================================================ */

        /* Disconnects the client from the server */
        void    disconnect()
        {
#ifdef ENABLE_TLS
            if (this->_ssl_connection != nullptr)
                SSL_free(this->_ssl_connection);
#endif
            this->_socket_handler.delSocket(*this);
            this->close();

            this->connected = false;
            this->_handler.onDisconnected();
            LOG_WARN(LOG_CATEGORY_NETWORK, "Disconnected from server.");
        }

        /* returns whether if the client uses TLS for its connection */
        /* ( set by Client::enableTLS() )                            */
        bool    useTLS() const
        {
            return (this->_useTLS);
        }

    




        /* ======================================================================================================== */
        /* PRIVATE MEMBERS                                                                                          */
        /* ======================================================================================================== */

    private:

        /* ================================================ */
        /* SSL context init                                 */
        /* ================================================ */
#ifdef ENABLE_TLS
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

        // like in openssl:
        // returns 0 => needs to send more data
        // returns 1 => handshake complete
        // returns -1 => error
        int     _do_ssl_handshake()
        {
            int handshake_error = SSL_connect(this->_ssl_connection);
            if (handshake_error != 1)
            {
                int ssl_error = SSL_get_error(this->_ssl_connection, handshake_error);
                if (ssl_error == SSL_ERROR_WANT_WRITE)
                {
                    // needs to send more data
                    LOG_INFO(LOG_CATEGORY_NETWORK, "Handshake partially sent, need to send more data...");
                    return (0);
                }
                ERR_print_errors_fp(stderr);
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Error on ssl handshake, aborting.");
                return (-1);
            }
            // FD_CLR(this->getSocket(), &this->_send_fd);
            this->_socket_handler.socketWantsWrite(*this, false);
            this->_handshake_done = true;
            this->_handler.onConnected();
            LOG_INFO(LOG_CATEGORY_NETWORK, "Client upgraded connection to TLS successfully");
            return (1);
        }
#endif


    #define     RECV_BLK_SIZE 1024
        bool    _receive()
        {
            uint8_t buffer[RECV_BLK_SIZE] = {0};
            ssize_t size;
#ifdef ENABLE_TLS
            if (this->_useTLS)
                size = SSL_read(this->_ssl_connection, buffer, RECV_BLK_SIZE);
            else
#endif
                size = recv(this->getSocket(), buffer, RECV_BLK_SIZE, MSG_DONTWAIT);
            if (size == 0)
            {
                this->disconnect();
                return (false);
            }
            else if (size < 0)
            {
                // we dont know what made recv fail, but for safety disconnect client.
                this->disconnect();
                return (false);
            }

            typename Client::msg_list_type::iterator handler = this->_messages_handlers.end();

            // already received a chunk of packet 
            if (!this->_received_packet.empty())
            {
                int append_result = this->_appendPacket(buffer, size);
                
                if (append_result != 0)
                    return (true); // packet is incomplete, or error, needs more data chunks
                // packet complete
                handler = this->_messages_handlers.find(this->_received_packet.message_name);
                if (handler == this->_messages_handlers.end())
                {
                    this->_handler.onMessageMismatch(this->_received_packet.message_name);
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Cannot get data handler in parsed packet for message `" << this->_received_packet.message_name << "`, this should have been resolved before parsing the packet.");
                    return (true);
                }
            }
            // no packet defined in client, expecting a header for inserting a new one
            else
            {
                // copy header from buffer
                packed_data_header<0>   data_header;
                if (!unpack_data_header(data_header, (char*)buffer, size))
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Invalid new packet size is smaller than data_header size: packet ignored.");
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
                    this->_handler.onMessageMismatch(message_name);
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Unknown data handler for message: `" << message_name << "`");
                    return (true);
                }
                if (handler->second.sizeof_type != data_header.data_size)
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Packet size for message `" << message_name << "` doesnt correspond with current message packet type definition: expected a data containing " 
                    << handler->second.sizeof_type << " bytes but header specifies " << data_header.data_size << " data bytes");
                    return (true);
                }

                // create new packet in client _received_packet in case of incomplete packet
                int new_packet_result = this->_newPacket(data_header, buffer, size);
                if (new_packet_result < 0)
                    return (true); // error on packet.
                else if (new_packet_result == 1) // packet is incomplete, needs more data chunks
                    return (true);
                // packet complete
            }

            return (this->_handle_packet(handler->second));
        }




        bool    _handle_packet(const message_handler_type& handler)
        {
            if (handler.sizeof_type != this->_received_packet.data_size)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Parsed packet size doesn't correspond to size in handler !!!");
                this->_cancelPacket();
                return (true);
            }

            uint8_t data_buffer[handler.sizeof_type];
            std::memcpy(data_buffer, this->_received_packet.data.c_str(), handler.sizeof_type);
            handler.handle(*this, reinterpret_cast<DTO*>(data_buffer));

            // handeled packet needs to be cleared
            this->_cancelPacket();

            return (this->getSocket() != -1);
        }



        template<typename T, typename std::size_t S = sizeof(T)>
        void    _emit_base(const std::string& message, const T& data) throw (InvalidPacketMessageNameException)
        {
            packed_data<S> pack = pack_data<T>(message, data);
            char    data_buffer[sizeof(packed_data<S>)] = {0};

            std::memcpy(data_buffer, &pack, sizeof(data_buffer));
            this->_data_to_send.push(std::string(data_buffer, sizeof(data_buffer)));
            // FD_SET(this->getSocket(), &this->_send_fd);
            this->_socket_handler.socketWantsWrite(*this, true);
        }



        bool    _send_data()
        {
#ifdef ENABLE_TLS
            if (this->_useTLS && this->_handshake_done == false)
            {
                if (this->_do_ssl_handshake() == -1)
                    this->disconnect();
                return (false);
            }
#endif

            if (this->_data_to_send.empty())
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "fd_set was set for sending however no data is provider to send.");
                this->_socket_handler.socketWantsWrite(*this, false);
                return (false);
            }

            LOG_INFO(LOG_CATEGORY_NETWORK, "emitting to server");
            ssize_t sent_bytes;
#ifdef ENABLE_TLS
            if (this->_useTLS)
                sent_bytes = SSL_write(this->_ssl_connection, this->_data_to_send.top().c_str(), this->_data_to_send.top().size());
            else
#endif
                sent_bytes = send(this->getSocket(), this->_data_to_send.top().c_str(), this->_data_to_send.top().size(), 0);
            
            if (sent_bytes < 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Send failed with error: " << std::strerror(errno))
                return (false);
            }
            else if (sent_bytes == 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "sent 0 bytes of data");
                return (false);
            }
            else if ((size_t)sent_bytes != this->_data_to_send.top().size())
            {
                std::string left = this->_data_to_send.top().substr(sent_bytes, this->_data_to_send.top().length());
                this->_data_to_send.pop();
                this->_data_to_send.push(left);   
                LOG_INFO(LOG_CATEGORY_NETWORK, "Data sent was cropped " << ": " << this->_data_to_send.top().length() << " bytes left to send");
                return (false);
            }
            // sent full packet.
            this->_data_to_send.pop();
            if (this->_data_to_send.empty())
                this->_socket_handler.socketWantsWrite(*this, false);
            return (true);
        }





        class SocketException : std::exception
        {
            public:
                virtual const char *what() const noexcept
                {
                    return ("Socket exception");
                }
        };
        
        class ConnectException : std::exception
        {
            public:
                virtual const char *what() const noexcept
                {
                    return ("Connect exception");
                }
        };

        class SSLInitException : std::exception
        {
            public:
                virtual const char *what() const noexcept
                {
                    return ("SSL initialisation exception");
                }
        };

    public:
        bool        connected;

    private:
        //int         _socket;

        // fd_set      _read_fd;
        // fd_set      _send_fd;
        SocketsHandler  _socket_handler;

        sockaddr_in     _client_address;

        msg_list_type   _messages_handlers;

#ifdef ENABLE_TLS
        bool                _useTLS;
        SSL_CTX*            _ssl_ctx;
        const SSL_METHOD*   _ssl_method;
        SSL*                _ssl_connection;
        bool                _handshake_done;
#endif

        handler_type    _handler;
};