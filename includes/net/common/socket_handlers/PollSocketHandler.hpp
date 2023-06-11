
#pragma once

#include <sys/poll.h>
#include <vector>
#include <map>


/* SocketsHandler implementation for poll() */
class SocketsHandler
{
    public:
        typedef std::map<int, struct pollfd>    fd_map_type;
        typedef std::vector<struct pollfd>      fd_vector_type;

        SocketsHandler() { }

        // Adds the socket to the list of checked sockets
        template<typename T>
        typename std::enable_if<
            std::is_convertible<decltype(std::declval<T>().getSocket()), int>::value,
        void>::type 
        addSocket(T& socketHolder)
        {
            struct pollfd poll_fd;
            poll_fd.fd = socketHolder.getSocket();
            poll_fd.events = POLLIN; // by default always poll in on new sockets
            poll_fd.revents = 0;
            if (this->_poll_fds.insert(std::make_pair(socketHolder.getSocket(), poll_fd)).second == false)
                throw std::logic_error("Cannot insert pollfd struct in pollfd list.");
        }

        // Deletes the socket from the list of checked sockets
        template<typename T>
        typename std::enable_if<
            std::is_convertible<decltype(std::declval<T>().getSocket()), int>::value,
        void>::type   
        delSocket(const T& socketHolder)
        {
            if (this->_poll_fds.erase(socketHolder.getSocket()) == 0)
                throw (std::logic_error("trying to delete a socket that is not in pollfd list."));
        }

        // Set the socket flag for reading for socket depending on selected
        template<typename T>
        typename std::enable_if<
            std::is_convertible<decltype(std::declval<T>().getSocket()), int>::value,
        void>::type
        socketWantsRead(const T& socketHolder, bool selected)
        {
            fd_map_type::iterator it = this->_poll_fds.find(socketHolder.getSocket());
            if (it == this->_poll_fds.end())
                throw std::logic_error("socket not found in poll_fds asked for reading: " + std::to_string(socketHolder.getSocket()));
            if (selected)
                it->second.events |= POLLIN;
            else
                it->second.events = it->second.events & ~POLLIN;
        }

        // Set the socket flag for writing for socket depending on selected
        template<typename T>
        typename std::enable_if<
            std::is_convertible<decltype(std::declval<T>().getSocket()), int>::value,
        void>::type
        socketWantsWrite(const T& socketHolder, bool selected)
        {
            fd_map_type::iterator it = this->_poll_fds.find(socketHolder.getSocket());
            if (it == this->_poll_fds.end())
                throw std::logic_error("socket not found in poll_fds asked for writing: " + std::to_string(socketHolder.getSocket()));
            if (selected)
                it->second.events |= POLLOUT;
            else
                it->second.events = it->second.events & ~POLLOUT;
        }

        // process a poll on the given sockets list
        bool   processPoll(const int timeout_ms = -1)
        {
            this->_poll_fds_aligned.clear();

            // only reserve if more capacity is needed
            if (this->_poll_fds_aligned.capacity() < this->_poll_fds.size())
                this->_poll_fds_aligned.reserve(this->_poll_fds.size());

            for (fd_map_type::iterator it = this->_poll_fds.begin(); it != this->_poll_fds.end(); ++it)
                this->_poll_fds_aligned.push_back(it->second);

            if (poll(this->_poll_fds_aligned.data(), this->_poll_fds_aligned.size(), timeout_ms) < 0)
                return true;

            // change poll id on success
            // this value can overflow, but for safety clamp it each 32 polls
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

        // macros to use the event struct
        #define EV_IS_SOCKET(ev, sock)    (ev.socket == (sock))
        #define EV_IS_READABLE(ev) ((ev.events & POLLIN) != 0)
        #define EV_IS_WRITABLE(ev) ((ev.events & POLLOUT) != 0)
        #define EV_IS_ERROR(ev)    ((ev.events & (POLLERR | POLLNVAL)) != 0)
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

            if (n == this->_poll_fds_aligned.size())
            {
                return (_SOCKET_END);
            }
            while (n < this->_poll_fds_aligned.size())
            {
                const pollfd& fd = this->_poll_fds_aligned.at(n);
                if (fd.revents != 0)
                {
                    ++n;
                    return socket_event(fd.fd, fd.revents);
                }
                ++n;
            }
            return (_SOCKET_END);
        }

    
    private:
        fd_map_type     _poll_fds;
        fd_vector_type  _poll_fds_aligned;

        // id of previous poll, needed to reset nextSocketEvent counter,
        // can overflow, it just need to change value between each poll.
        size_t          _poll_id;

        // number of touches to the _poll_fds list, if touches are 0 when polling, 
        // there is no need to reconstruct the _poll_fds_aligned
};
