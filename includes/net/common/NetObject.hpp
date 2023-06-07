#pragma once

#include <string>
#include <exception>
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
class NetObject
{
    public:
        // constructs from IPv4 addr (i.e. on connection accepted)
        NetObject(const struct sockaddr_in& addr) throw(std::logic_error)
        : _address_4(addr),
          _address_family(AF_INET)
        {
            char    ip_addr_buffer[INET_ADDRSTRLEN + 1] = {0};
            if (inet_ntop(AF_INET, &this->_address_4.sin_addr, ip_addr_buffer, sizeof(this->_address_4)) == NULL)
                throw std::logic_error(std::string("Invalid address type for IPv4 sockaddr_in: cannot parse ip string: ") + std::strerror(errno));
            this->_ip_address = std::string(ip_addr_buffer, std::strlen(ip_addr_buffer));
            this->_port = ntohs(this->_address_4.sin_port);
            // zeroing ipv6 struct
            std::memset(&this->_address_6, 0, sizeof(this->_address_6));
        }

        // constructs from IPv6 addr (i.e. on connection accepted)
        NetObject(const struct sockaddr_in6& addr) throw(std::logic_error)
        : _address_6(addr),
          _address_family(AF_INET6)
        {
            char    ip_addr6_buffer[INET6_ADDRSTRLEN + 1] = {0};
            if (inet_ntop(AF_INET6, &this->_address_6.sin6_addr, ip_addr6_buffer, sizeof(this->_address_6)) == NULL)
                throw std::logic_error(std::string("Invalid address type for IPv6 sockaddr_in6: cannot parse ip string: ") + std::strerror(errno));
            this->_ip_address = std::string(ip_addr6_buffer, std::strlen(ip_addr6_buffer));
            this->_port = ntohs(this->_address_6.sin6_port);
            // zeroing ipv4 struct
            std::memset(&this->_address_4, 0, sizeof(this->_address_4));
        }

        // Constructs a new sockaddr depending on family (i.e. for initialisation of struct sockaddr)
        NetObject(const std::string& ip_address, const int port, const sa_family_t family) throw(std::logic_error)
        : _ip_address(ip_address),
          _port(static_cast<in_port_t>(port)),
          _address_family(family)
        {
            if (this->_address_family == AF_INET)
            {
                this->_address_4.sin_port = htons(this->_port);
                this->_address_4.sin_family = AF_INET;
                if (inet_pton(AF_INET, ip_address.c_str(), &this->_address_4.sin_addr) != 1)
                    throw std::logic_error(std::string("Invalid ip format for IPv4 address `") + this->_ip_address + "`: " + std::strerror(errno));
                // zeroing ipv6 struct
                std::memset(&this->_address_6, 0, sizeof(this->_address_6));
            }
            else if (this->_address_family == AF_INET6)
            {
                this->_address_6.sin6_port = htons(this->_port);
                this->_address_6.sin6_family = AF_INET6;
                if (inet_pton(AF_INET6, ip_address.c_str(), &this->_address_6.sin6_addr) != 1)
                    throw std::logic_error(std::string("Invalid ip format for IPv6 address `") + this->_ip_address + "`: " + std::strerror(errno));
                // zeroing ipv4 struct
                std::memset(&this->_address_4, 0, sizeof(this->_address_4));
            }
            else
                throw std::invalid_argument("unexpected family type, expected AF_INET or AF_INET6.");
        }

        // todo: do a better implementation for parsing _ip_address
        // using gethostaddrbyname2 
        std::string     getHostname() const { return (this->_ip_address + ":" + std::to_string(this->_port)); }
        
        std::string     getIpAddress() const { return (this->_ip_address); }
        int             getPort() const { return (this->_port); }
        
        sa_family_t     getAddressFamily() const { return (this->_address_family); }
        sockaddr_in     getAddress4() const { return (this->_address_4); }
        sockaddr_in6    getAddress6() const { return (this->_address_6); }

    protected:
        // string/in representation
        std::string _ip_address;
        in_port_t   _port;

        // real addr structs
        struct sockaddr_in  _address_4;
        struct sockaddr_in6 _address_6;
        sa_family_t         _address_family;
};