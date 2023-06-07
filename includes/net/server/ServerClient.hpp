# pragma once

#include <arpa/inet.h>
#include <string>

#ifdef ENABLE_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

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
class ServerClient : public D, public PacketManager
{
#ifdef ENABLE_TLS
    
    public:
        // constructor for IPv4
        ServerClient(const int socket, const struct sockaddr_in& addr, SSL* ssl = nullptr)
            : D(), PacketManager(addr), _socket(socket), _ssl_connection(ssl), _is_ssl(ssl != nullptr), _accept_done(false)
            {} 

        // constructor for IPv6
        ServerClient(const int socket, const struct sockaddr_in6& addr, SSL* ssl = nullptr)
            : D(), PacketManager(addr), _socket(socket), _ssl_connection(ssl), _is_ssl(ssl != nullptr), _accept_done(false)
            {} 
#else
        ServerClient(const int socket, const struct sockaddr_in& addr)
            : PacketManager(addr), D(), _socket(socket)
            {} 

        // constructor for IPv6
        ServerClient(const int socket, const struct sockaddr_in6& addr)
            : PacketManager(addr), D(), _socket(socket)
            {} 
#endif

// #ifdef ENABLE_TLS
//     private:
//         ServerClient(int socket, SSL* ssl = nullptr)
//             : D(), _socket(socket), _ssl_connection(ssl), _is_ssl(ssl != nullptr), _accept_done(false)
//             {}
    
//     public:
//         // ipv4 SSL client
//         ServerClient(int socket, const sockaddr_in& address, SSL* ssl = nullptr)
//             : ServerClient(socket, ssl)
//             {
//                 this->_address = std::string(inet_ntoa(address.sin_addr));
//                 this->_port = ntohs(address.sin_port);
//             }

//         // ipv6 SSL client
//         ServerClient(int socket, const sockaddr_in6& address, const socklen_t address_len, SSL* ssl = nullptr)
//             : ServerClient(socket, ssl)
//             {
//                 char addr_buffer[address_len];
//                 if (inet_ntop(AF_INET6, &address.sin6_addr, addr_buffer, address_len) == NULL)
//                 {
//                     LOG_ERROR(LOG_CATEGORY_NETOWRK, "Invalid address for initializing a new client");
//                     // TODO
//                 }
                
//                 this->_address = std::string(addr_buffer);
//                 this->_port = ntohs(address.sin6_port);
//             }
// #else
//     private:
//         ServerClient(int socket)
//             : D(), _socket(socket)
//             {}
    
//     public:
//         // ipv4 client
//         ServerClient(int socket, const sockaddr_in& address)
//             : ServerClient(socket)
//             {
//                 this->_address = std::string(inet_ntoa(address.sin_addr));
//                 this->_port = ntohs(address.sin_port);
//             }
        
//         // ipv6 client
//         ServerClient(int socket, const sockaddr_in6& address, const socklen_t address_len)
//             : ServerClient(socket)
//             {
//                 char addr_buffer[address_len];
//                 if (inet_ntop(AF_INET6, &address.sin6_addr, addr_buffer, address_len) == NULL)
//                 {
//                     LOG_ERROR(LOG_CATEGORY_NETOWRK, "Invalid address for initializing a new client");
//                     // TODO
//                 }
                
//                 this->_address = std::string(addr_buffer);
//                 this->_port = ntohs(address.sin6_port);
//             }
// #endif

#ifdef ENABLE_TLS
        SSL    *getSSL() const { return this->_ssl_connection; }
        bool    isSSL() const { return this->_is_ssl; }

        // todo 
        std::string getCertificate()
        {
            if (!this->_is_ssl)
                return "";

            X509 *cert;
            cert = SSL_get_peer_certificate(this->_ssl_connection);
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
            line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
            std::string certificate = std::string(line, std::strlen(line));
            free(line);
            X509_free(cert);
            return (certificate);
        }

#endif

        int     getSocket() const { return this->_socket; }

        // std::string getFullAddress() const
        // {
        //     return (this->_address + ":" + std::to_string(this->_port));
        // }

        // std::string getAddress() const
        // {
        //     return (this->_address);
        // }

        // int         getPort() const
        // {
        //     return (this->_port);
        // }

    private:
        int         _socket;
        // std::string _address;
        // int         _port;

#ifdef ENABLE_TLS
        SSL    *_ssl_connection;
        bool    _is_ssl;
        bool    _accept_done;
#endif

    friend class Server<H>;
};