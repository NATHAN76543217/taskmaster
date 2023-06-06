# include "Taskmaster.hpp"
# include "cpp_argparse.hpp"

# define ENABLE_TLS
# include "net/server.hpp"
# include "dto.hpp"

//DONE implement logging to files
//DONE Finish to well format the LOGs
//DONE Reload config file on SIGHUP
//SONE Install yaml-cpp 
//TODO Parse all config file
//DONE for logger implement defaultDestination for default output file
//TODO add lock file 
//TODO daemonized main process 
//TODO start childs according to config
//TODO Code a client with the grfic library FTXUI
// TODO ptrace on ourselves to avoid process monitoring
// TODO parse config file at start to have color enabled on every log


class ClientData
{
	public:
		// independant data storage for every client
		bool	asked_status = false;
};

class TaskmasterClientsManager : public ServerClientHandler<TaskmasterClientsManager, ClientData>
{
    public:
        TaskmasterClientsManager(server_type& server)
        : handler_type(server)
        {}

		// void onConnected(client_type& client)
		// {
		// 	if (client.isSSL())
		// 		std::cout << client.getCertificate() << std::endl;
		// }

        void declareMessages()
        {
            this->server.onMessage("status", 
                server_type::make_handler<StatusDTO>([](server_type& server, client_type& client, DTO* dto)
                {
                    // always safe
					StatusDTO* status = reinterpret_cast<StatusDTO*>(dto);
					
					client.asked_status = status->value;
					LOG_INFO(LOG_CATEGORY_NETWORK, "Received status with value of " << status->value << ", sending back value 42");

					// resending back packet with different value
					status->value = 42;
					server.emit("status_response", *status, client);
				}
            ));
        }
};





void	signal_handler(int signal)
{

	LOG_DEBUG(LOG_CATEGORY_SIGNAL, "Signal °" + ntos(signal) + " catched.")
	if (signal == SIGHUP)
	{
		LOG_WARN(LOG_CATEGORY_CONFIG, "SIGHUP received: Reload config")
		Taskmaster::GetInstance()->reloadConfigFile();
	}
	// for developpement
	if (signal == SIGINT)
	{
		Taskmaster::GetInstance()->freeLockFile();
		exit(signal);
	}
	// exit(0);
}


int	init_signals(struct sigaction *sa)
{

	memset(sa, 0, sizeof(struct sigaction));
	// sa.sa_sigaction = signal_handler;
	sigemptyset(&sa->sa_mask);
	sa->sa_handler = signal_handler;
	sa->sa_flags = 0; 
	// sa->sa_flags = SA_SIGINFO; 
	// sigaddset(&sa.sa_mask, SIGINT);
	// sigprocmask(&sa.sa_mask);
	sigaction(SIGINT, sa, NULL);
	sigaction(SIGHUP, sa, NULL);
	// if (signal(SIGQUIT, signal_handler) == SIG_ERR)
		// return EXIT_FAILURE;
	// if (signal(SIGINT, signal_handler) == SIG_ERR)
		// return EXIT_FAILURE;
	LOG_INFO(LOG_CATEGORY_SIGNAL, "Signal handlers successfuly started")
	
	return EXIT_SUCCESS;
}




int main(int ac, char** av)
{
	bool		help = false;

	std::string	serverIp;
	int			serverPort;

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
		ARG_USAGE(" === Taskmaster Server ===");
		return (EXIT_FAILURE);
	}
	if (help)
	{
		ARG_USAGE(" === Taskmaster Server ===");
		return (EXIT_SUCCESS);
	}




	struct sigaction sa;
	Taskmaster *TM = Taskmaster::GetInstance();


#if LOG_CATEGORY_AUTO == false
	TM->initCategories();
#endif

	if (TM->isRunningRootPermissions() == false)
	{
		LOG_ERROR(LOG_CATEGORY_INIT, "You must have root permissions to run this program.")
		return EXIT_FAILURE;
	}

	TM->takeLockFile();

	LOG_INFO(LOG_CATEGORY_INIT, "PID: " + ntos(TM->getpid()))

	if (TM->parse_arguments() != EXIT_SUCCESS)
	{
		LOG_ERROR(LOG_CATEGORY_INIT, "Failed to parse arguments.")
		return EXIT_FAILURE;
	}

	if (init_signals(&sa) != EXIT_SUCCESS)
	{
		LOG_ERROR(LOG_CATEGORY_INIT, "Failed to init signal handlers.")
		return EXIT_FAILURE;
	}

	TM->loadConfigFile(TM_DEF_CONFIGPATH);

	// TODO daemonize better than this
	// int pid = fork();
	// if (pid < 0)
	// {
	// 	LOG_ERROR(LOG_CATEGORY_INIT, "Failed to initialize daemon, aborting.");
	// 	return (EXIT_FAILURE);
	// }
	// else if (pid == 0)
	// {
		LOG_INFO(LOG_CATEGORY_INIT, "Started daemon");
	
		Server<TaskmasterClientsManager>	*server = new Server<TaskmasterClientsManager>(serverIp, serverPort);
		server->ssl_cert_file = 		"./certificates/cert.pem";
		server->ssl_private_key_file =	"./certificates/cert.pem";
		
		server->start_listening();
		do
		{
			// do taskmaster things...
		}
		while (server->wait_update());

		TM->exitProperly();
		LOG_INFO(LOG_CATEGORY_INIT, "Quit program.")

	//	return EXIT_SUCCESS;
	// }
	// // also print on stdout (maybe ?)
	// std::cout << "Started daemon (" << pid<< ")" << std::endl;
	return (EXIT_SUCCESS);
}