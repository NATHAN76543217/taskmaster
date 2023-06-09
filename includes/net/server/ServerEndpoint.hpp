# pragma once

#include "sys/socket.h"
#include <arpa/inet.h>
#include <string>
#include <exception>
#include "../common/InetAddress.hpp"

#include "Tintin_reporter.hpp"

class ServerEndpoint : public InetAddress
{
    public:

#ifdef ENABLE_TLS
        ServerEndpoint(const std::string& ip_address, const int port, const bool useTLS = false, const sa_family_t family = AF_INET) throw(std::logic_error)
        : InetAddress(ip_address, port, family), _useTLS(useTLS)

#else
        ServerEndpoint(const std::string& ip_address, const int port, const sa_family_t family = AF_INET) throw(std::logic_error)
        : InetAddress(ip_address, port, family)
#endif
        {
            this->_socket = ::socket(this->_address_family, SOCK_STREAM, 0);
            if (this->_socket <= 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Socket syscall failed for address initialization on port " << ntohs(this->_port) << ": " << strerror(errno));
                throw SocketException();
            }
        }


        int     getSocket() const { return (this->_socket); }
#ifdef ENABLE_TLS
        bool    useTLS() const { return (this->_useTLS); }
#endif


        void    start_listening(const int max_pending_connections) const throw(std::logic_error)
        {
            if (this->_address_family == AF_INET)
            {
                if (bind(this->_socket, (sockaddr*)(&this->_address_4), sizeof(this->_address_4)) != 0)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Bind syscall failed for address initialization on port " << this->_port << ": " << strerror(errno));
                    throw BindException();
                }
            }
            else if (this->_address_family == AF_INET6)
            {
                if (bind(this->_socket, (sockaddr*)(&this->_address_6), sizeof(this->_address_6)) != 0)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Bind syscall failed for address on IPv6 initialization on port " << this->_port << ": " << strerror(errno));
                    throw BindException();
                }
            }

            if (listen(this->_socket, max_pending_connections) != 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Listen syscall failed for address initialization on port " << this->_port << ": " << strerror(errno));
                throw ListenException();
            }
#ifdef ENABLE_TLS
            if (this->_useTLS)
                LOG_INFO(LOG_CATEGORY_NETWORK, "Started listening on TLS endpoint " <<  this->getHostname() << " on port " << this->_port)
            else
#endif
                LOG_INFO(LOG_CATEGORY_NETWORK, "Started listening on endpoint " << this->getHostname() << " on port " << this->_port)
        }

        class SocketException : public std::logic_error
        {
            public:
                SocketException() : std::logic_error("socket exception occured: abort") {}
        };

        class BindException : public std::logic_error
        {
            public:
                BindException() : std::logic_error("bind exception occured: abort") {}
        };

        class ListenException : public std::logic_error
        {
            public:
                ListenException() : std::logic_error("listen exception occured: abort") {}
        };

    private:
        int             _socket;

        #ifdef ENABLE_TLS
        bool            _useTLS;
        #endif
};
