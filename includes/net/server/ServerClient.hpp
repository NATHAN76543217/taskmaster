# pragma once

#include <arpa/inet.h>
#include <poll.h>
#include <string>

#ifdef ENABLE_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include "../common/PacketManager.hpp"
#include "../common/TcpSocket.hpp"

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
class ServerClient : public D, public PacketManager, public TcpSocket
{
#ifdef ENABLE_TLS
    
    public:
        ServerClient(const int socket, const InetAddress& addr_info, SSL* ssl = nullptr) throw(std::logic_error)
            : D(), PacketManager(addr_info), TcpSocket(socket, addr_info.getAddressFamily()), _ssl_connection(ssl), _useTLS(ssl != nullptr), _accept_done(false)
            {} 
#else
        ServerClient(const int socket, const InetAddress& addr_info) throw(std::logic_error)
            : D(), PacketManager(addr_info), TcpSocket(socket, addr_info.getAddressFamily())
            {} 
#endif


#ifdef ENABLE_TLS
        SSL    *getSSL() const { return this->_ssl_connection; }
        bool    useTLS() const { return this->_useTLS; }

        // todo 
        std::string getCertificate()
        {
            if (!this->_useTLS)
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


#ifdef ENABLE_TLS
    private:
        SSL         *_ssl_connection;
        const bool  _useTLS;
        bool        _accept_done;
#endif

    friend class Server<H>;
};