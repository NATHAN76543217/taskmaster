#pragma once

#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <errno.h>


class TcpSocket
{
    public:
        TcpSocket(const int socket, const sa_family_t family = AF_INET) 
        : _socket(socket), _family(family)
        { }

        TcpSocket(const sa_family_t family = AF_INET) 
        {
            this->_socket = ::socket(family, SOCK_STREAM, 0);
            if (this->_socket <= 0)
                throw std::logic_error(std::string("Unable to create socket: ") + strerror(errno));
        }

        int     getSocket() const
        {
            return (this->_socket);
        }

        int     getFamily() const
        {
            return (this->_family);
        }

        void    close()
        {
            ::close(this->_socket);
            this->_socket = -1;
        }
    
    private:
        int         _socket;
        sa_family_t _family;
};