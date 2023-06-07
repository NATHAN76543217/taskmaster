# pragma once

#include "sys/socket.h"
#include <arpa/inet.h>
#include <string>
#include <exception>
#include "../common/NetObject.hpp"

#include "Tintin_reporter.hpp"

class ServerEndpoint : public NetObject
{
    public:

#ifdef ENABLE_TLS
        ServerEndpoint(const std::string& ip_address, const int port, const bool useTLS = false, const sa_family_t family = AF_INET)
        : NetObject(ip_address, port, family), _useTLS(useTLS)
        {
            this->_socket = ::socket(this->_address_family, SOCK_STREAM, 0);
            if (this->_socket <= 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Socket syscall failed for address initialization on port " << ntohs(this->_port) << ": " << strerror(errno));
                throw SocketException();
            }
        }
#endif


        int     getSocket() const { return (this->_socket); }
#ifdef ENABLE_TLS
        bool    useTLS() const { return (this->_useTLS); }
#endif


        void    start_listening(const int max_pending_connections) const
        {
            if (this->_address_family == AF_INET)
            {
                if (bind(this->_socket, (sockaddr*)(&this->_address_4), sizeof(this->_address_4)) != 0)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Bind syscall failed for address initialization on port " << ntohs(this->_port) << ": " << strerror(errno));
                    throw BindException();
                }
            }
            else if (this->_address_family == AF_INET6)
            {
                if (bind(this->_socket, (sockaddr*)(&this->_address_6), sizeof(this->_address_6)) != 0)
                {
                    LOG_ERROR(LOG_CATEGORY_NETWORK, "Bind syscall failed for address on IPv6 initialization on port " << ntohs(this->_port) << ": " << strerror(errno));
                    throw BindException();
                }
            }

            if (listen(this->_socket, max_pending_connections) != 0)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Listen syscall failed for address initialization on port " << ntohs(this->_port) << ": " << strerror(errno));
                throw ListenException();
            }
#ifdef ENABLE_TLS
            if (this->_useTLS)
                LOG_INFO(LOG_CATEGORY_NETWORK, "Started listening on TLS endpoint " << this->_ip_address << " on port " << this->_port)
            else
#endif
                LOG_INFO(LOG_CATEGORY_NETWORK, "Started listening on endpoint " << this->_ip_address << " on port " << this->_port)
        }

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

    private:
        int             _socket;

        #ifdef ENABLE_TLS
        bool            _useTLS;
        #endif
};
