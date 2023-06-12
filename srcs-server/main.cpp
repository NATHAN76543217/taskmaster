# include "Taskmaster.hpp"
# include "cpp_argparse.hpp"
# include "net/server.hpp"
# include "dto.hpp"

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
// TODO ptrace on ourselves to avoid process monitoring
// TODO parse config file at start to have color enabled on every log
//TODO define all auto restart possible values for conf
//TODO use strsignal to interpret signal number to signal string
// TODO Use <chrono> in all our time stuff (specialy timestamp)
//TODO for logger add mutex for every log_destination (with required checks) but only for files (std::cout/cerr is thread safe)
//TODO implement a flood protection from a valid connection
//TODO add config key envfromparent
//TODO avoid duplicate in job names
//TODO when change job._nbprocs (setnbprocs) just spawn od unspawn the right number of processes without touching the good ones
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




void	signal_handler(int signal)
{

	LOG_DEBUG(LOG_CATEGORY_SIGNAL, "Signal °" + ntos(signal) + " catched.")
	if (signal == SIGHUP)
	{
		LOG_WARN(LOG_CATEGORY_CONFIG, "SIGHUP received: Reload config")
		//TODO protect race contidiotn 
		Taskmaster::GetInstance()->reloadConfigFile();
	}
	else if (signal == SIGINT)
	{
		LOG_WARN(LOG_CATEGORY_INIT, "SIGINT received: Prepare stopping...")
		Taskmaster::GetInstance()->stop(true);
	}	
	else if (signal == SIGCHLD)
	{
		LOG_WARN(LOG_CATEGORY_JM, "SIGCHLD received: a child process as exited.")
		int stat = 0;
		pid_t child_pid = 0;
		if ((child_pid = waitpid(0, &stat, 0)) == -1)
		{
			LOG_ERROR(LOG_CATEGORY_JM, "Failed to `waitpid` : " << strerror(errno))
			return ;
		}
		if (WIFEXITED(stat) || WIFSIGNALED(stat))
		{
			if (WIFEXITED(stat))
			{
				LOG_INFO(LOG_CATEGORY_JM, "Process [" << child_pid << "] exit with status '" << WEXITSTATUS(stat) << "'.")
			}
			else
			{
				LOG_INFO(LOG_CATEGORY_JM, "Process [" << child_pid << "] exit with signal '" << WTERMSIG(stat) << "'.")
			}
			//TODO implement JobManager::getJobByPid(), JobManager::jobEnd() qui retournent tout deux un iterateur
			//get the job, remove from his list the terminated pid, store the status as exit or signaled, success or error

		}
		else
		{
			LOG_CRITICAL(LOG_CATEGORY_JM, "Process [" << child_pid << "] marked as 'not exited' and not 'signaled' but waitpid is set on 'NOHANG'.")
			return ;
		}
	}
	
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
	sigaction(SIGCHLD, sa, NULL);
	// if (signal(SIGQUIT, signal_handler) == SIG_ERR)
		// return EXIT_FAILURE;
	// if (signal(SIGINT, signal_handler) == SIG_ERR)
		// return EXIT_FAILURE;
	LOG_INFO(LOG_CATEGORY_SIGNAL, "Signal handlers successfuly started")
	
	return EXIT_SUCCESS;
}




int main(int ac, char** av, const char** env)
{
	bool		help = false;

	std::string	serverIp;
	int			serverPort;
	struct sigaction sa;
	Taskmaster *TM = nullptr;

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
	
	TM = Taskmaster::GetInstance();

	if (TM->isRunningRootPermissions() == false)
	{
		std::cerr << "You must have root permissions to run this program." << std::endl;
		return EXIT_FAILURE;
	}


	TM->takeLockFile();

	TM->setEnv(env);

#if LOG_CATEGORY_AUTO == false
	TM->initCategories();
	// std::cout << Tintin_reporter::getLogManager() << std::endl;
	LOG_INFO(LOG_CATEGORY_LOGGER, Tintin_reporter::getLogManager())

# else
	Tintin_reporter::getLogManager("./default.log");
#endif


	LOG_INFO(LOG_CATEGORY_INIT, "PID: " + ntos(TM->getpid()))

	if (init_signals(&sa) != EXIT_SUCCESS)
	{
		LOG_CRITICAL(LOG_CATEGORY_INIT, "Failed to init signal handlers.")
		return EXIT_FAILURE;
	}

	if (TM->loadConfigFile(TM_DEF_CONFIGPATH))
	{
		LOG_CRITICAL(LOG_CATEGORY_INIT, "Failed to load configuration file. Aborting")
		TM->exitProperly();
		//REVIEW call to exit() here? serioulys? 
 		exit(EXIT_FAILURE);
	}


	if (TM->startJobManager() != EXIT_SUCCESS)
	{
		LOG_CRITICAL(LOG_CATEGORY_INIT, "Failed to start JobManager. Aborting")
		return EXIT_FAILURE;
	}
	// Display all jobs
	// std::cout << "=== JOBS ===" << std::endl;
	// for (std::list<Job>::const_iterator it = TM->_joblist.begin(); it != TM->_joblist.end(); it++)
	// {
	// 	LOG_DEBUG(LOG_CATEGORY_JOB, *it);
	// }




		do
		{
			// do taskmaster things...
			std::this_thread::sleep_for(std::chrono::seconds(5));
			LOG_DEBUG(LOG_CATEGORY_DEFAULT, "Main thread - main loop");
		}
		while (!TM->shouldStop());

		LOG_DEBUG(LOG_CATEGORY_DEFAULT, "Main thread - exit main loop");
		TM->stopJobManager();
		LOG_INFO(LOG_CATEGORY_INIT, "Quit program.")
		TM->exitProperly();

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