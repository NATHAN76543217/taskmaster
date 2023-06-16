#include <iostream>
#include "cpp_argparse.hpp"
#include "net/client.hpp"
#include "dto.hpp"

class ClientData
{
    // data storage for client to use in handlers
};

class TaskmasterClientHandler : public ClientHandler<TaskmasterClientHandler, ClientData>
{
    public:
        TaskmasterClientHandler(client_type& Client)
        : handler_type(Client)
        {}

        void onConnected()
        {
            StatusDTO dto;
            dto.value = 101;
            this->client.emit("status", dto);
        }

        void declareMessages()
        {
            this->client.onMessage("status_response", 
                client_type::make_handler<StatusDTO>([](client_type& client, DTO* dto)
                {
                    StatusDTO *status = reinterpret_cast<StatusDTO*>(dto);
                    LOG_INFO(LOG_CATEGORY_NETWORK, "Received status response from server with value: " << status->value);
                    client.disconnect();
                }
            ));
        }
};


int main(int ac, char **av)
{
    bool        help = false;

    std::string serverIp;
    int         serverPort;

	ARG_INIT(
		ARG_GROUP("server", "Daemonized server running taskmaster",
			ARG<std::string>("-i", "--ip", "ip address of taskmaster server", &serverIp, ARG_REQUIRED),
			ARG<int>("-p", "--port", "port of taskmaster server", &serverPort, ARG_REQUIRED)
		),
		ARG_NOVALUE("-h", "--help", "shows this usage", &help)
	);


	int parsing_failed = ARG_PARSE(ac, av);
	if (parsing_failed)
	{
		ARG_USAGE("  === Taskmaster client === ");
		return (EXIT_FAILURE);
	}
	if (help)
	{
		ARG_USAGE("  === Taskmaster client ===");
		return (EXIT_SUCCESS);
	}


    #if LOG_CATEGORY_AUTO == false
        int err = 0;
        err += Tintin_reporter::getLogManager("./client.log").addCategory(LOG_CATEGORY_DEFAULT);
        err += Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_INIT);
        err += Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_NETWORK);
        err += Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_SIGNAL, "./signal.log");
        err += Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_CONFIG);
        std::cout << "init err: " << err << std::endl;
    #endif


    Client<TaskmasterClientHandler> *client = new Client<TaskmasterClientHandler>(serverIp, serverPort);
    client->connect();

    do
    {
    } while (client->wait_update());
    
    delete client;

    return (0);
}