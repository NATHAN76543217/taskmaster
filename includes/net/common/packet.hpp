#pragma once

#include <iostream>
#include <stack>

#include "packed_data.hpp"
#include "NetObject.hpp"

#include "Tintin_reporter.hpp"

#define MAX_PACKET_SIZE 1024

class PacketManager : public NetObject
{
    protected:
        PacketManager(const struct sockaddr_in& addr)
        : NetObject(addr) {}

        PacketManager(const struct sockaddr_in6& addr)
        : NetObject(addr) {}
        
        PacketManager(const std::string& ip, const int port, const sa_family_t family)
        : NetObject(ip, port, family) {}

        void    _cancelPacket()
        {
            LOG_INFO(LOG_CATEGORY_NETWORK, "Cleared packet data from " << this->getHostname() << " for message `" << this->_received_packet.message_name << "`.");
            this->_received_packet.message_name = "";
            this->_received_packet.last_time_packed = (timeval){.tv_sec=0,.tv_usec=0};
            this->_received_packet.data_size = 0;
            this->_received_packet.data = "";
        }


        // creates a new _received_packet if data size does not exceed max packet size
        // returns -1 on error 
        // returns  0 on packet completed
        // returns  1 on packet added to _received_packet;
        template<size_t S>
        int    _newPacket(const packed_data_header<S> packet_hdr, const uint8_t *buffer, const size_t buffer_size)
        {
            if (packet_hdr.data_size > MAX_PACKET_SIZE)
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Invalid packet header received from " << this->getHostname() << " for message `" << packet_hdr.message_name << "`: required size too big: specified " << (packet_hdr.data_size)    << " however maximum size for a packet is  " << MAX_PACKET_SIZE << ".");  
                return (-1);
            }
            if (buffer_size - sizeof(packet_hdr) > packet_hdr.data_size)   
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Invalid packet received from " << this->getHostname() << "  for message `" << this->_received_packet.message_name << "`: size overflow: expected " << (packet_hdr.data_size)  << " got " << (buffer_size - sizeof(packet_hdr)) << ".");
                return -1;
            }

            this->_received_packet.message_name = std::string(packet_hdr.message_name, std::strlen(packet_hdr.message_name));
            this->_received_packet.data_size = packet_hdr.data_size;
            //todo
            this->_received_packet.last_time_packed = (timeval){.tv_sec=0,.tv_usec=0};
            this->_received_packet.data = std::string((char*)(buffer + sizeof(packet_hdr)), buffer_size - sizeof(packet_hdr));

            // packet completed, needs to be flushed by _cancelPacket() call
            if (packet_hdr.data_size == buffer_size - sizeof(packet_hdr))
            {
                LOG_INFO(LOG_CATEGORY_NETWORK, "Completed packet from " << this->getHostname() << "  for message `" << this->_received_packet.message_name << "`.");
                return (0);
            }
            LOG_INFO(LOG_CATEGORY_NETWORK, "Queued new incomplete packet from " << this->getHostname() << "  for message `" << this->_received_packet.message_name << "`,  misses " << (this->_received_packet.data_size - this->_received_packet.data.length()) << " data bytes");
            return (1);
        }

        // appends packet data to the current _received_packet if it exists
        // returns -1 on error
        // returns  0 on packet completed
        // returns  1 on success but packet partially retrieved (i.e. needs another chunk)
        int     _appendPacket(const uint8_t *buffer, const size_t buffer_size)
        {
            if (this->_received_packet.empty())
            {
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Cannot append packet from " << this->getHostname() << "  because no extra packet chunk was expected.");
                return -1;
            }
            if (buffer_size + this->_received_packet.data.length() > this->_received_packet.data_size)   
            {
                this->_cancelPacket();
                LOG_ERROR(LOG_CATEGORY_NETWORK, "Invalid packet received from " << this->getHostname() << "  for message `" << this->_received_packet.message_name << "`: size overflow: expected " << (this->_received_packet.data_size)    << " got " << (this->_received_packet.message_name.length() + buffer_size) << ".");
                return -1;
            }
            this->_received_packet.last_time_packed = (timeval){.tv_sec=0,.tv_usec=0};
            this->_received_packet.data += std::string((char*)buffer, buffer_size);

            if (this->_received_packet.data.length() == this->_received_packet.data_size)   
            {
                LOG_INFO(LOG_CATEGORY_NETWORK, "Completed packet from " << this->getHostname() << "  for message `" << this->_received_packet.message_name << "`.");
                return 0;
            }
            return (1);
        }

        std::stack<std::string>  _data_to_send;

        struct {
            std::string             message_name;       // name of packet
            size_t                  data_size;          // size of packet for message `message_name` 
            timeval                 last_time_packed;   // timestamp of the last time the packet was parsed
                                                        // if time between 2 packet is too long pack_timeout is returned
            std::string             data;               // concatenated data of all previoulsy matching packets.
        
            bool    empty() { return this->message_name.empty() || this->data_size == 0; }
        }                        _received_packet;
};