
#pragma once

#include <string>
#include <ostream>
#include <exception>

class UnpackException : std::exception
{
    public:
        const char *what() const noexcept
        {
            return ("unpacking of packet failed");
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
    serialize(data, (uint8_t*)pack.data);
    return (pack);
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
        buffer[i] = ((uint16_t)ptr[i]);
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
        ptr[i] = (uint16_t)buffer[i];    
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
