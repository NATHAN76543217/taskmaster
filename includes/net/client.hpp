#pragma once

#include <string>
#include <stack>
#include <map>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "packet.hpp"
#include "dto_base.hpp"
#include "Tintin_reporter.hpp"

template<typename D>
class Client;


template<typename H, typename D>
class ClientHandler
{
    public:
        typedef D                          client_data_type;
        typedef Client<H>                  client_type;
        typedef ClientHandler<H, D>        handler_type;
    
    public:
        ClientHandler(client_type& client)
            : client(client)
            {}

        virtual void    onConnected() { };
        //virtual void    onConnectedIPv6() { };
        virtual void    onDisconnected() { };

        // called when the server tried to send data on message_name but data type mismatched.
        virtual void    onMessageMismatch(const std::string& message_name) { (void)message_name; };


        virtual void    declareMessages() = 0;

    protected:
        client_type& client;
};


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
class Client : public H::client_data_type, protected PacketManager
{
    public:
        typedef Client                          client_type;
        typedef H                               handler_type;
        typedef typename H::client_data_type    data_type;

    public:
        Client(const std::string& server_ip, const short server_port)
        : data_type(), connected(false), server_ip(server_ip), server_port(server_port), _socket(-1), _handler(*this)
        {
            _client_address.sin_family = AF_INET;
            _client_address.sin_port = htons(static_cast<in_port_t>(this->server_port));
            if (inet_aton(server_ip.c_str(), &_client_address.sin_addr) == 0)
                throw InetException();
            
            this->_handler.declareMessages();

            FD_ZERO(&this->_read_fd);
            FD_ZERO(&this->_send_fd);
        }

        /* connects the client to the server */
        void    connect()
        {
            this->_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (this->_socket <= 0)
                throw SocketException();
   
            if (::connect(this->_socket, reinterpret_cast<sockaddr*>(&this->_client_address), sizeof(this->_client_address)) != 0)
                throw ConnectException();
            
            FD_SET(this->_socket, &this->_read_fd);
            this->_handler.onConnected();
            LOG_INFO(LOG_CATEGORY_NETWORK, "Client connected on address " << this->server_ip << " on port " << this->server_port);
        }



        /* waits an update this the server, in the client, emits are sent on the wait_update call too */
        /* returns true if the connection is still active after handling                              */
        bool    wait_update()
        {
            // preserve fd sets 
            fd_set read_set;
            fd_set write_set;
            FD_ZERO(&write_set);
            FD_ZERO(&read_set);
            FD_COPY(&this->_read_fd, &read_set);
            FD_COPY(&this->_send_fd, &write_set);

            int nfds = this->_socket + 1;
            if (select(nfds, &read_set, &write_set, 0, 0) < 0)
                throw SelectException();

            // data pending to be read
            if (FD_ISSET(this->_socket, &read_set))
            {
                if (_receive() != true)
                {
                    return (false); // disconnected from server
                }
            }

            // socket able to emit data
            if (FD_ISSET(this->_socket, &write_set))
            {
                this->_send_data();
            }

            return (true);
        }


        /* Message list type definition */

        struct functor_handler
        {
            size_t  sizeof_type;
            void(*handler)(client_type&, DTO*);
        };

        template<typename T>
        static functor_handler    make_handler(void(*handler_function)(client_type&, DTO*))
        {
            functor_handler handler;
            handler.sizeof_type = sizeof(T);
            handler.handler = handler_function;
            return (handler);
        }

        typedef functor_handler message_handler_type;

        typedef std::map<std::string, message_handler_type> msg_list_type;


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
            this->_messages_handlers.insert(std::make_pair(name, handler));
            LOG_INFO(LOG_CATEGORY_INIT, "added new handler for `" << name << "` message");
        }




        template<typename T, typename std::size_t S = sizeof(T)>
        void    emit(const std::string& message, const T& data)
        {
            this->_emit_base(message, data);
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted to the server");
        }


        template<typename T, typename std::size_t S = sizeof(T)>
        bool    emit_now(const std::string& message, const T& data)
        {
            this->_emit_base(message, data);
            LOG_INFO(LOG_CATEGORY_NETWORK, "set `" << message << "` to be emitted now to the server");
            try {
                return this->_send_data();
            } catch (SendException)
            {
                return (false);
            }
        }
        


        void    disconnect()
        {
            ::close(this->_socket);
            this->_socket = -1;
            this->connected = false;
            this->_handler.onDisconnected();
        }

    
    private:

    #define     RECV_BLK_SIZE 1024
        bool    _receive()
        {
            uint8_t buffer[RECV_BLK_SIZE] = {0};
            ssize_t size = recv(this->_socket, buffer, RECV_BLK_SIZE, MSG_DONTWAIT);
            if (size == 0)
            {
                this->disconnect();
                return (false);
            }
            else if (size < 0)
            {
                // we dont know what made recv fail, but for safety disconnect client.
                this->disconnect();
                throw RecvException();
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
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Unknown data handler for message: `" << this->_received_packet.message_name << "`");                    this->_cancelPacket();
                    return (true);
                }
            }
            // no packet defined in client, expecting a header for inserting a new one
            else
            {
                if ((size_t)size < sizeof(packed_data_header<0>))
                {
                    LOG_WARN(LOG_CATEGORY_NETWORK, "invalid new packet size is smaller than data_header size: cannot parse packet");
                    return (true);
                }

                // copy header from buffer
                packed_data_header<0>  data_header;
                std::memcpy(&data_header, buffer, sizeof(packed_data_header<0>));
                data_header.message_name[sizeof(data_header.message_name) - 1] = 0;

                // check message name character conformity
                if (!is_valid_message_name(data_header))
                    return (true);

                // check message validity
                std::string message_name(data_header.message_name, std::strlen(data_header.message_name));
                handler = this->_messages_handlers.find(message_name);
                if (handler == this->_messages_handlers.end())
                {
                    this->_handler.onMessageMismatch(message_name);
                    LOG_WARN(LOG_CATEGORY_NETWORK, "Unknown data handler for message: `" << this->_received_packet.message_name << "`");                    this->_cancelPacket();
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

            // packet handling
        
            if (handler->second.sizeof_type != this->_received_packet.data_size)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Parsed packet size doesn't correspond to size in handler !!!");
                this->_cancelPacket();
                return (true);
            }

            uint8_t data_buffer[handler->second.sizeof_type];
            std::memcpy(data_buffer, this->_received_packet.data.c_str(), handler->second.sizeof_type);
            handler->second.handler(*this, reinterpret_cast<DTO*>(data_buffer));

            // handeled packet needs to be cleared
            this->_cancelPacket();

            return (this->_socket != 0);
        }



        template<typename T, typename std::size_t S = sizeof(T)>
        void    _emit_base(const std::string& message, const T& data)
        {
            packed_data<S> pack = pack_data<T>(message, data);
            char    data_buffer[sizeof(packed_data<S>)] = {0};

            serialize(pack, (uint8_t*)data_buffer);
            this->_data_to_send.push(std::string(data_buffer, sizeof(data_buffer)));
            FD_SET(this->_socket, &this->_send_fd);
        }

        bool    _send_data()
        {
            if (this->_data_to_send.empty())
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "fd_set was set for sending however no data is provider to send.");
                return (false);
            }
            LOG_INFO(LOG_CATEGORY_NETWORK, "emitting to server");
            ssize_t sent_bytes = send(this->_socket, this->_data_to_send.top().c_str(), this->_data_to_send.top().size(), 0);
            if (sent_bytes < 0)
            {
                LOG_WARN(LOG_CATEGORY_NETWORK, "Send failed with error: " << std::strerror(errno))
                throw client_type::SendException();
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
            FD_CLR(this->_socket, &this->_send_fd);
            return (true);
        }






        class InetException : std::exception
        {
            public:
                virtual const char *what() const noexcept
                {
                    return ("Inet exception");
                }
        };

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

        class SelectException : std::exception
        {
            public:
                virtual const char *what() const noexcept
                {
                    return ("Select exception");
                }
        };
        
        class SendException : std::exception
        {
            public:
                virtual const char *what() const noexcept
                {
                    return ("Send exception");
                }
        };

        class RecvException : std::exception
        {
            public:
                virtual const char *what() const noexcept
                {
                    return ("Recv exception");
                }
        };

    public:
        bool        connected;
        std::string server_ip;
        short       server_port;

    private:
        int     _socket;

        fd_set      _read_fd;
        fd_set      _send_fd;

        sockaddr_in _client_address;

        msg_list_type   _messages_handlers;

        handler_type    _handler;
};