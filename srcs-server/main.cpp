# include "Taskmaster.hpp"
# include "cpp_argparse.hpp"
# include "net/server.hpp"
# include "dto.hpp"
# include "utils.hpp"

//DONE implement logging to files
//DONE Finish to well format the LOGs
//DONE Reload config file on SIGHUP
//SONE Install yaml-cpp 
//TODO Parse all server config file
//DONE Parse all programs config file
//DONE for logger implement defaultDestination for default output file
//DONE add lock file 
//TODO daemonized main process 
//TODO start childs according to config
//TODO Code a client with the grfic library FTXUI
//TODO ptrace on ourselves to avoid process monitoring
//TODO parse config file at start to have color enabled on every log
//TODO define all auto restart possible values for conf
//TODO use strsignal to interpret signal number to signal string
//TODO Use <chrono> in all our time stuff (specialy timestamp)
//TODO for logger add mutex for every log_destination (with required checks) but only for files (std::cout/cerr is thread safe)
//TODO implement a flood protection from a valid connection
//TODO add config key envfromparent
//TODO avoid duplicate in job names
//TODO when change job._nbprocs (setnbprocs) just spawn od unspawn the right number of processes without touching the good ones
//TODO add struct job_worker that contain the pid and status of child processes
//DONE implement signal handler as a separate thread 
//TODO transform all current code in thread-safe code
//TODO implement restart policy for jobs

class ClientData
{
	public:
		// independant data storage for every client
		bool	asked_status = false;
};

class TaskmasterClientsHandler : public ServerClientHandler<TaskmasterClientsHandler, ClientData>
{
    public:
        TaskmasterClientsHandler(server_type& server)
        : handler_type(server)
        {}

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



int main(int ac, char** av, const char** env)
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
	
	if (isRunningRootPermissions() == false)
	{
		std::cerr << "You must have root permissions to run this program. Current UID: " << geteuid() << std::endl;
		return EXIT_FAILURE;
	}
	if (takeLockFile(TM_DEF_LOCKPATH))
		return EXIT_FAILURE;

	Taskmaster &TM = Taskmaster::GetInstance();


	if (TM.initialization(env) == EXIT_FAILURE)
	{
		Taskmaster::DestroyInstance();
		return EXIT_FAILURE;
	}

	// Display all jobs
	// std::cout << "=== JOBS ===" << std::endl;
	// for (std::list<Job>::const_iterator it = TM->_joblist.begin(); it != TM->_joblist.end(); it++)
	// {
	// 	LOG_DEBUG(LOG_CATEGORY_JOB, *it);
	// }

	TM.startThreads();


	LOG_DEBUG(LOG_CATEGORY_DEFAULT, "Main thread - exit main loop");
	LOG_INFO(LOG_CATEGORY_INIT, "Quit program.")
	TM.exitProperly();

	freeLockFile(TM_DEF_LOCKPATH);
	std::cout << "Lock file '" << TM_DEF_LOCKPATH  << "' successfuly released." << std::endl;
	return EXIT_SUCCESS;


// 	// TODO daemonize better than this
// 	int pid = fork();
// 	if (pid < 0)
// 	{
// 		LOG_CRITICAL(LOG_CATEGORY_INIT, "Failed to initialize daemon, aborting.");
// 		LOG_INFO(LOG_CATEGORY_INIT, "Destroying logger.")
// 		Tintin_reporter::destroyLogManager();
// 		return (EXIT_FAILURE);
// 	}
// 	else if (pid == 0)
// 	{
// 		LOG_INFO(LOG_CATEGORY_INIT, "Started daemon");
		
// 		Server<TaskmasterClientsHandler>	*server = new Server<TaskmasterClientsHandler>(serverIp, serverPort);
// 		server->start_listening();
// try{

// server->wait_update());
// }catch(std::exception & e)
// {
// 		LOG_INFO(LOG_CATEGORY_INIT, "Catch error for select")
// }

// 	}
// 	// also print on stdout (maybe ?)
// 	LOG_INFO(LOG_CATEGORY_INIT, "Started daemon (" << pid << ")")
// 	LOG_INFO(LOG_CATEGORY_INIT, "Destroying logger.")
//TODO should not close logger in main 
// 	Tintin_reporter::destroyLogManager();
// 	return (EXIT_SUCCESS);
}