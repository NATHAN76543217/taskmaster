# pragma once

#include "ServerClient.hpp"

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
class ServerClientsHandler
{
    public:
        typedef D                                       client_data_type;
        typedef ServerClient<T, client_data_type>       client_type;
        typedef Server<T>                               server_type;
        typedef ServerClientsHandler<T, D>              handler_type;
    
    public:
        ServerClientsHandler(server_type& server)
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