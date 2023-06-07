#pragma once

#include <string>

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

