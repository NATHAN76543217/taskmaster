#pragma once

#include <string>
#include <exception>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>

#define MAX_HOST_RESOLVE_RETRIES 3
#define MAX_HOSTNAME_LEN         255

class InetAddress
{
    public:
        // constructs from IPv4 addr (i.e. on connection accepted)
        InetAddress(const struct sockaddr_in& addr) throw(std::logic_error)
        : _address_4(addr),
          _address_family(AF_INET)
        {
            this->_ip_address = this->_get_ip_of_addr4(this->_address_4.sin_addr);
            this->_hostname = this->_get_hostname_of_addr(reinterpret_cast<sockaddr*>(&this->_address_4), sizeof(addr));
            this->_port = ntohs(this->_address_4.sin_port);
            // zeroing ipv6 struct
            std::memset(&this->_address_6, 0, sizeof(this->_address_6));
        }

        // constructs from IPv6 addr (i.e. on connection accepted)
        InetAddress(const struct sockaddr_in6& addr) throw(std::logic_error)
        : _address_6(addr),
          _address_family(AF_INET6)
        {
            this->_ip_address = this->_get_ip_of_addr6(this->_address_6.sin6_addr);
            this->_hostname = this->_get_hostname_of_addr(reinterpret_cast<sockaddr*>(&this->_address_6), sizeof(addr));
            this->_port = ntohs(this->_address_6.sin6_port);
            // zeroing ipv4 struct
            std::memset(&this->_address_4, 0, sizeof(this->_address_4));
        }

        // Constructs a new sockaddr depending on family (i.e. for initialisation of struct sockaddr)
        InetAddress(const std::string& hostname, const int port, const sa_family_t family = AF_INET) throw(std::logic_error)
        : _hostname(hostname),
          _port(static_cast<in_port_t>(port)),
          _address_family(family)
        {
            if (this->_address_family == AF_INET)
            {
                this->_get_addr_by_hostname(reinterpret_cast<sockaddr*>(&this->_address_4), sizeof(this->_address_4), this->_ip_address, this->_hostname, AF_INET);
                this->_address_4.sin_port = htons(this->_port);
                // zeroing ipv6 struct
                std::memset(&this->_address_6, 0, sizeof(this->_address_6));
            }
            else if (this->_address_family == AF_INET6)
            {
                this->_get_addr_by_hostname(reinterpret_cast<sockaddr*>(&this->_address_6), sizeof(this->_address_6), this->_ip_address, this->_hostname, AF_INET6);
                this->_address_6.sin6_port = htons(this->_port);
                // zeroing ipv4 struct
                std::memset(&this->_address_4, 0, sizeof(this->_address_4));
            }
            else
                throw std::invalid_argument("unexpected family type, expected AF_INET or AF_INET6.");
        }

        std::string     getHostname() const { return (this->_hostname + "(" + this->_ip_address + ")"); }
        
        std::string     getIpAddress() const { return (this->_ip_address); }
        int             getPort() const { return (this->_port); }
        
        sa_family_t     getAddressFamily() const { return (this->_address_family); }
        sockaddr_in     getAddress4() const { return (this->_address_4); }
        sockaddr_in6    getAddress6() const { return (this->_address_6); }
    
    private:
        void    _get_addr_by_hostname(sockaddr *const addr, const socklen_t addr_len, std::string& ip_address, const std::string &hostname, const sa_family_t host_family) throw (std::logic_error)
        {
            // try to get addr from hostname
            int         err = 0;
            addrinfo    hints;
            addrinfo*   res = nullptr;
            hints.ai_family = host_family;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = 0;
            hints.ai_canonname = NULL;
            hints.ai_addr = NULL;
            hints.ai_next = NULL;

            int n_retries = MAX_HOST_RESOLVE_RETRIES;
            do
            {
                err = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
                if (err == 0)
                    break ;
                else if (err == EAI_AGAIN)
                    std::cout << "Failed to retrieve host ip address, retrying..." << std::endl;
                else
                    throw std::logic_error(std::string("Cannot retrieve host ip address: ") + gai_strerror(err) + ": " + std::to_string(err));
            } while (--n_retries > 0);
            if (n_retries == 0 || res == nullptr)
                throw std::logic_error(std::string("Unable to retrieve host ip address: ") + gai_strerror(err) + ": " + std::to_string(err));

            if (res->ai_family == AF_INET)
                ip_address = this->_get_ip_of_addr4(((sockaddr_in*)(res->ai_addr))->sin_addr);
            else if (res->ai_family == AF_INET6)
                ip_address = this->_get_ip_of_addr6(((sockaddr_in6*)(res->ai_addr))->sin6_addr);

            if (addr_len < res->ai_addrlen)
                throw std::logic_error("Provided sockaddr is too small for ai_addrlen (expected " + std::to_string(res->ai_addrlen) + " got " + std::to_string(addr_len) + ")");
            std::memcpy(addr, res->ai_addr, res->ai_addrlen);
        }

        std::string _get_hostname_of_addr(const struct sockaddr* addr, const socklen_t addr_len) throw (std::logic_error)
        {
            // try to find a hostname corresponding with addr struct
            char hostname_buffer[MAX_HOSTNAME_LEN + 1] = {0};
            int err = 0;

            int n_retries = MAX_HOST_RESOLVE_RETRIES;
            do
            {
                err = getnameinfo(addr, addr_len, hostname_buffer, MAX_HOSTNAME_LEN, nullptr, 0, 0);
                if (err == 0)
                    break ;
                else if (err == EAI_AGAIN)
                    std::cout << "Failed to retrieve host name, retrying..." << std::endl;
                else
                    throw std::logic_error(std::string("Cannot retrieve host name: ") + gai_strerror(err) + ": " + std::to_string(err));
            } while (--n_retries > 0);
            if (n_retries == 0 || hostname_buffer[0] == 0)
                throw std::logic_error(std::string("Unable to retrieve host name: ") + gai_strerror(err) + ": " + std::to_string(err));

            return (std::string(hostname_buffer));
        }

        std::string _get_ip_of_addr4(const struct in_addr& addr) throw (std::logic_error)
        {
            char    ip_addr_buffer[INET_ADDRSTRLEN + 1] = {0};
            if (inet_ntop(AF_INET, &addr, ip_addr_buffer, sizeof(sockaddr_in)) == NULL)
                throw std::logic_error(std::string("Invalid address type for IPv4 sockaddr_in: cannot parse ip string: ") + std::strerror(errno));
            return (std::string(ip_addr_buffer));
        }

        std::string _get_ip_of_addr6(const struct in6_addr& addr) throw (std::logic_error)
        {
            char    ip_addr_buffer[INET6_ADDRSTRLEN + 1] = {0};
            if (inet_ntop(AF_INET6, &addr, ip_addr_buffer, sizeof(sockaddr_in6)) == NULL)
                throw std::logic_error(std::string("Invalid address type for IPv4 sockaddr_in: cannot parse ip string: ") + std::strerror(errno));
            return (std::string(ip_addr_buffer));
        }

    protected:
        // hostname, ip, and port, from sockaddr structs
        std::string _hostname;
        std::string _ip_address;
        in_port_t   _port;

        // actual addr structs, _address_family indicates the current one
        struct sockaddr_in  _address_4;
        struct sockaddr_in6 _address_6;
        sa_family_t         _address_family;
};