
#pragma once

#include <string>
#include <ostream>
#include <exception>


class InvalidPacketException : std::exception
{
    public:
        const char *what() const noexcept
        {
            return ("Invalid packet");
        }
};


template<std::size_t S = 0>
struct packed_data_header
{
    char    message_name[32];
    size_t  data_size = S;
} __attribute__((packed));

template<std::size_t S>
struct packed_data
{
    packed_data_header<S>   header;
    char                    data[S];
} __attribute__((packed));


// data packer, packs data of T into a packed_data<sizeof(T)> struct with a message name 
template<typename T, std::size_t S = sizeof(T)>
packed_data<S> pack_data(const std::string& message_name, const T& data)
{
    packed_data<S>  pack;

    std::strncpy(pack.header.message_name, message_name.c_str(), 32);
    pack.header.data_size = sizeof(T);
    if (!is_valid_message_name(pack.header))
    {
        throw InvalidPacketException();
    }
    std::memcpy(pack.data, &data, S);
    return (pack);
}

// Data unpacker, unpacks data from header into buffer depending on buffer_size
// buffer_size can be larger than header size however not smaller
template<std::size_t S>
bool   unpack_data_header(packed_data_header<S> &header, const char *buffer, const size_t buffer_size)
{
    if (buffer_size < sizeof(packed_data_header<S>))
    {
        return 0;
    }

    // copy header from buffer
    std::memcpy(&header, buffer, sizeof(packed_data_header<S>));
    // add trailing '\0' to message_name in case of non teminated char string
    header.message_name[sizeof(header.message_name) - 1] = 0;
    return (true);
}




bool        is_valid_message_name(const char* name)
{
    if (!name)
        return (false);
    size_t i = 0;
    while (i < sizeof(packed_data_header<0>::message_name))
    {
        if (!name[i])
            break ;
        if (
            // is numeric check
            (name[i] < '0' || name[i] > '9')
            // is alpha check
         && (name[i] < 'a' || name[i] > 'z')
         && (name[i] < 'A' || name[i] > 'Z')
            // other allowed special characters
         && !(name[i] == '_' || name[i] == '-' || name[i] == '/' || name[i] == '.' 
           || name[i] == '<' || name[i] == '>' || name[i] == '[' || name[i] == ']' )
        )
        {
            // invalid character
            return (false);
        }
        ++i;
    }
    //packet message name must at least be 3 character long
    return (i >= 3);
}


template<std::size_t S>
bool        is_valid_message_name(const packed_data_header<S>& pack)
{
    return (is_valid_message_name(pack.message_name));
}
