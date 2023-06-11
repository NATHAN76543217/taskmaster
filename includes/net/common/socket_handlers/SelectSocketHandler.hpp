
#pragma once

#include <iostream>
#include <sys/select.h>
#include <vector>

#ifndef FD_COPY
#define FD_COPY(src, dst) std::memcpy(dst, src, sizeof(*dst));
#endif

/* SocketsHandler implementation for select() */
class SocketsHandler
{
    public:
        typedef std::vector<int>    fd_list_type;

        SocketsHandler()
        {
            FD_ZERO(&this->_read_fds);
            FD_ZERO(&this->_write_fds);
        }

        // Adds the socket to the list of checked sockets
        template<typename T>
        typename std::enable_if<
            std::is_convertible<decltype(std::declval<T>().getSocket()), int>::value,
        void>::type 
        addSocket(T& socketHolder)
        {
            // select for reading by default
            FD_SET(socketHolder.getSocket(), &this->_read_fds);
            this->_selected_fds.push_back(socketHolder.getSocket());
        }

        // Deletes the socket from the list of checked sockets
        template<typename T>
        typename std::enable_if<
            std::is_convertible<decltype(std::declval<T>().getSocket()), int>::value,
        void>::type   
        delSocket(const T& socketHolder)
        {
            FD_CLR(socketHolder.getSocket(), &this->_read_fds);
            FD_CLR(socketHolder.getSocket(), &this->_write_fds);
            for (std::vector<int>::iterator it = this->_selected_fds.begin(); it != this->_selected_fds.end(); ++it)
            {
                if (*it == socketHolder.getSocket())
                {
                    this->_selected_fds.erase(it);
                    return ;
                }
            }
            throw (std::logic_error("Deleted socket was not in pollfd list."));
        }

        // Set the socket flag for reading for socket depending on selected
        template<typename T>
        typename std::enable_if<
            std::is_convertible<decltype(std::declval<T>().getSocket()), int>::value,
        void>::type
        socketWantsRead(const T& socketHolder, bool selected)
        {
            if (selected)
                FD_SET(socketHolder.getSocket(), &this->_read_fds);
            else
                FD_CLR(socketHolder.getSocket(), &this->_read_fds);
        }

        // Set the socket flag for writing for socket depending on selected
        template<typename T>
        typename std::enable_if<
            std::is_convertible<decltype(std::declval<T>().getSocket()), int>::value,
        void>::type
        socketWantsWrite(const T& socketHolder, bool selected)
        {
            if (selected)
                FD_SET(socketHolder.getSocket(), &this->_write_fds);
            else
                FD_CLR(socketHolder.getSocket(), &this->_write_fds);
        }

        // process a poll on the given sockets list
        bool   processPoll(const int timeout_ms = -1)
        {
            // preserve main fd sets
            FD_ZERO(&this->_selected_read_fds);
            FD_ZERO(&this->_selected_write_fds);
            FD_COPY(&this->_read_fds, &this->_selected_read_fds);
            FD_COPY(&this->_write_fds, &this->_selected_write_fds);
            
            int nfds = this->_getnfds();
            struct timeval *timeout_ptr = nullptr;
            struct timeval timeout = {.tv_sec=timeout_ms / 1000,.tv_usec=(timeout_ms % 1000) * 1000};
            if (timeout_ms >= 0)
            {
                timeout_ptr = &timeout;
            }
            if (select(nfds, &this->_selected_read_fds, &this->_selected_write_fds, nullptr, timeout_ptr) < 0)
            {
                return (true);
            }

            // change poll id on success
            // this value can overflow, but for safety clamp it to 32
            this->_poll_id = ++this->_poll_id % 32;
            return false;
        }

        // event struct to interface socket handlers return values
        struct socket_event
        {
            int socket;
            int events;
            
            socket_event(int sock, int ev) : socket(sock), events(ev) {}

            socket_event() : socket(-1), events(-1) {}
        };
        #define _SOCK_READ   0b0001
        #define _SOCK_WRITE  0b0010

        // macros to use the event struct
        #define EV_IS_SOCKET(ev, sock)    (ev.socket == (sock))
        #define EV_IS_READABLE(ev) ((ev.events & _SOCK_READ) != 0)
        #define EV_IS_WRITABLE(ev) ((ev.events & _SOCK_WRITE) != 0)
        #define EV_IS_ERROR(ev)    (false) // there is no way of checking errors on fd with select
        #define EV_IS_END(ev)      (ev.events == -1 && ev.socket == -1)

        // Needs to be called in a loop after processPoll() until EV_IS_END(result) is reached
        // if the returned event is not the end, it contains data about the socket to handle and its event.
        #define _SOCKET_END        (socket_event())
        socket_event    nextSocketEvent()
        {
            static size_t n = 0;
            static size_t poll_id = 0;

            // reset n for each new poll
            if (this->_poll_id != poll_id)
            {
                poll_id = this->_poll_id;
                n = 0;
            }

            if (n == this->_selected_fds.size())
            {
                return (_SOCKET_END);
            }
            while (n < this->_selected_fds.size())
            {
                int sock = this->_selected_fds.at(n);
                int events = 0;
                
                if (FD_ISSET(sock, &this->_selected_read_fds))
                    events |= _SOCK_READ;
                if (FD_ISSET(sock, &this->_selected_write_fds))
                    events |= _SOCK_WRITE;
                
                if (events != 0)
                {
                    ++n;
                    return socket_event(sock, events);
                }
                ++n;
            }
            return (_SOCKET_END);
        }
    
    private:
#define _MAX(a, b) (a > b ? a : b)
        int _getnfds()
        {
            static size_t nfds = 0;
            static size_t last_sock_count = -1;

            if (last_sock_count != this->_selected_fds.size())
            {
                last_sock_count = this->_selected_fds.size();
                for (int i : this->_selected_fds)
                {
                    nfds = _MAX(nfds, (size_t)i);
                }
            }

            return (nfds + 1);
        }

    
    private:
        // main fd sets
        fd_set          _read_fds;
        fd_set          _write_fds;

        // copy sets for polling
        fd_set          _selected_read_fds;
        fd_set          _selected_write_fds;

        // list of selected fds for processPoll
        fd_list_type    _selected_fds;

        // id of previous poll, needed to reset nextSocketEvent counter,
        // can overflow, it just need to change value between each poll.
        size_t          _poll_id;
};
