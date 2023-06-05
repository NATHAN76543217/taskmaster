
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
    serialize(data, (uint8_t*)pack.data);
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



template<std::size_t S>
bool        is_valid_message_name(const packed_data_header<S>& pack)
{
    size_t i = 0;
    while (i < sizeof(packed_data_header<S>::message_name))
    {
        if (
            // is numeric check
            !(pack.message_name[i] < '0' || pack.message_name[i] > '9')
            // is alpha check
         && !(pack.message_name[i] < 'a' || pack.message_name[i] > 'z')
         && !(pack.message_name[i] < 'A' || pack.message_name[i] > 'Z')
            // other allowed special characters
         && !(pack.message_name[i] == '_' || pack.message_name[i] == '-' || pack.message_name[i] == '/')
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



// template<typename T, std::size_t S = sizeof(T)>
// void    unpack_data(T* obj, const packed_data_header<S>& data_header, uint8_t data_buffer[sizeof(T) + sizeof(packed_data_header<0>) + 1])
// {
//     if (data_header.data_size != sizeof(T))
//     {
//         std::cerr << "Size mismatch." << std::endl;
//         throw UnpackException();
//     }
//     deserialize(obj, data_buffer + sizeof(packed_data_header<0>));
// }



// serialization doesnt take in charge network byte order : todo

// SERIALIZATION/DESERIALIZATION IN BUFFER

template<typename T>
void serialize(const T& t, uint8_t buffer[sizeof(T)])
{
    static const size_t _size = sizeof(T);

    uint8_t *ptr = (uint8_t*)(&t);
    size_t i = 0;
    
    while (i < _size)
    {
        buffer[i] = ((uint8_t)ptr[i]);
        ++i;
    }
}


template<typename T, typename std::size_t S = sizeof(T)>
void    deserialize(T *obj, const uint8_t buffer[S])
{
    uint8_t *ptr = (uint8_t*)obj;
    size_t i = 0;

    while (i < S)
    {
        ptr[i] = (uint8_t)buffer[i];    
        ++i;
    }
}


// STANDART SERIALIZATION/DESERIALIZATION


// template<typename T>
// std::string serialize(const T& t)
// {
//     static const size_t _size = sizeof(T);

//     uint8_t *ptr = (uint8_t*)(&t);
//     uint8_t buffer[_size + 1] = {0};
//     size_t i = 0;
    
//     while (i < _size)
//     {
//         buffer[i] = ((uint16_t)ptr[i]);
//         ++i;
//     }
//     buffer[_size] = 0;
//     return std::string((char*)buffer, _size);
// }
    
// template<typename T>
// T  deserialize(const std::string& serialized_value)
// {
//     static const size_t _size = sizeof(T);

//     T   obj;
//     uint8_t *ptr = (uint8_t*)&obj;
//     size_t i = 0;

//     while (i < _size)
//     {
//         ptr[i] = (uint16_t)serialized_value[i];    
//         ++i;
//     }
//     return obj;
// }
