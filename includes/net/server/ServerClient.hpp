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
        ServerClient(const int socket, const struct sockaddr_in& addr, SSL* ssl = nullptr) throw(std::logic_error)
            : D(), PacketManager(addr), _socket(socket), _ssl_connection(ssl), _is_ssl(ssl != nullptr), _accept_done(false)
            {} 

        // constructor for IPv6
        ServerClient(const int socket, const struct sockaddr_in6& addr, SSL* ssl = nullptr) throw(std::logic_error)
            : D(), PacketManager(addr), _socket(socket), _ssl_connection(ssl), _is_ssl(ssl != nullptr), _accept_done(false)
            {} 
#else
        ServerClient(const int socket, const struct sockaddr_in& addr) throw(std::logic_error)
            : D(), PacketManager(addr), _socket(socket)
            {} 

        // constructor for IPv6
        ServerClient(const int socket, const struct sockaddr_in6& addr) throw(std::logic_error)
            : D(), PacketManager(addr), _socket(socket)
            {} 
#endif


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

    private:
        int         _socket;

#ifdef ENABLE_TLS
        SSL    *_ssl_connection;
        bool    _is_ssl;
        bool    _accept_done;
#endif

    friend class Server<H>;
};