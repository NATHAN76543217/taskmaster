# Server

## Implementing a server


### 1. Server clients handler class
To implement a server, you will first need to create a ServerClientHandler class.
This class will be the handler for your clients and will give you access to
handlers for client events (i.e. connection, disconnection, etc...)
This class will also need in template argument a ClientData class, this class will
be instanced for each client of the server, and will contain specific client data (or state)
to use in handlers. The class can contain everything you want,
as long as it is defaultly constructible (it needs to implement the default constructor)
The ServerClientHandler class must also implement the declareMessage member for messages handeling (see in 3. Messages)
```cpp
class ClientData
{
    // data for each client...
    std::string     name;
};

class MyServerClientHandler : ServerClientHandler<MyServerClientHandler, ClientData>
{
    MyServerClientHandler(server_type& server) : handler_type(server)
    {}

    void    onConnected(client_type& client, const struct sockaddr_in& client_address)
    {
        // called when a new client connects to the server
    }

    void    declareMessages()
    {
        // declare messages handlers (see section 3. Messages)
    }
}
```

The server instance then needs to be specialized by the `MyServerClientHandler` class.

For instantiating a server, the constructor requires at least an ip address as a std::string, and a port value.
Next, to make it start listening, simply call the `start_listening` member.
Then, to parse data received, and emit new data, the server must call `wait_update()` in a loop,
this call will return the actual state of the connection, so that the loop can be terminated when the servers connection is terminated.
Note: the `wait_update` call will block until no data is received or no data was declared to be sent. In some cases, you may want it to unblock after a certain timeout value even if no events occured, this can be specified with the `timeout_ms` optional argument of `wait_update`. 
```cpp
#include "net/server.hpp"

int main()
{
    Server<MyServerClientHandler> server = Server<MyServerClientHandler>("127.0.0.1", 8000);
    server->start_listening();

    do
    {
        // do global server stuff...
    } while (server->wait_update()) // <- executes message handlers, emits data buffered to be sent
    
    return (0);
}
```



### 2. handler types
The Server client handler implements these default event handlers that can be overrided
```cpp
void    onConnected(client_type& client, const struct sockaddr_in& client_address)
```
This handler will be called when a client connects to the server

```cpp
void    onConnectedIPv6(client_type& client, const struct sockaddr_in6& client_address)
```
This handler will be available for servers that enables IPv6, it is called when a client connects to the server on the IPv6 port.


```cpp
void    onDisconnected(client_type& client)
```
This handler will be called when a client disconnects from the server


```cpp
void    onMessageMismatch(client_type& client, const std::string& message_name)
```
This handler will be called when an packet error occurs on a client, this might mean that the client
isn't using the same message protocol (see section 3. Messages)

### 3. Messages
To communicate with clients, the server uses it's messages protocol. this will pack the message data
in a message struct of this type
```cpp
template<typename T, std::size_t S = sizeof(T)>
struct packed_data
{
    char    message_name[32];
    size_t  data_size = S;
    char    data[S];
};
```
This is entierly handeled by the server and permits the user to exchange custom data types structs indexed on a message_name.

#### How to receive messages from clients

Firstly the user needs to define a custom data transfer object (DTO) to be received by the server on a specific message name. This dto object needs to have the \_\_attribute\_\_((packed)) attribute so that no padding is applied when compiling the struct, this ensures that data size of types are the same between hosts that might compile structs with different paddings.
```cpp
#include "net/dto_base.hpp"

struct StatusDto : DTO
{
    int value;
    // ...
} __attribute__((packed));
```

Then, in the previously implemented `declareMessages` member of the `ServerClientHandler`, we can call `server_type::onMessage()` to add a new message handler to the server. To define the required data type for a specific message, you must specialize in `server_type::make_handler<T>` the desired dto type (in this case `StatusDTO`).

Note: handlers doesn't specifically require to be declared in the `declareMessage` function, they can even be added while the server is running (i.e. adding a new handler when a specific message is received and deleting it when another is received). Though `declareMessages` it is a good practice to define them in `declareMessage` at least for default handlers. 
```cpp
void    declareMessages()
{
    this->server.onMessage("status",
        server_type::make_handler<StatusDTO>([](server_type& server, client_type& client, DTO* obj)
        {
            // retrieving DTO type, this is safe because the data pointed by `obj` is exactly sizeof(StatusDTO)
            StatusDto dto = reinterpret_cast<StatusDto*>(obj);
            // mess with dto...

            // server is a reference to the server running on this handler (same as ServerClientHandler::server)

            // client is a reference to the client that sent the message.
            // Its ClientData can be accessed directly in the client_type object like:
            // client.name = "somename"; (if name is a field of ClientData)
        }
    ));
}
```

#### How send messages to clients

To send messages, you will also need a dto struct to pass some data,
for example:
```cpp
struct StatusResponseDto: DTO
{
    int value;
}   __attribute__((packed));
```

then, this data struct can be emitted in many ways to the clients.

if we want to broadcast it to all connected clients, use
```cpp
StatusResponseDTO response_dto;
response_dto.value = 10;
server->emit("status_response", response_dto);
```

if we want to send it to a specific client (), use
```cpp
StatusResponseDTO response_dto;
response_dto.value = 10;
server->emit("status_response", response_dto, client); // <- client is a server_type::client_type (like the one obtained in a message handler)
```

for both of this cases, the server will need to call `wait_update` to safely send the packets.

In some cases we want to send the packet right now (for example just before disconnecting a client), for that, you can use `emit_now` call, it will return a bool indicating if the packet was sent as a whole once. If the client keeps being connected after that, the rest of the packet will be sent, however if the client is disconnected just after the `emit_now` call, the leftover packet will not be sent. For this reason, disconnection packets must be short in size.
This `emit_now` version is available for broadcast and client specific emits the same way as the normal `emit` call.
```cpp
ErrorResponseDTO error_dto;
error_dto.error = 404;
if (server->emit_now("error", response_dto, client) == false)
{
    // error packet wasn't sent as a whole...
}
server->disconnect(client);
```