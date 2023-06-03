# include "Taskmaster.hpp"

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
//TODO define all auto restart possible values for conf
//TODO use strsignal to interpret signal number to signal string

void	signal_handler(int signal)
{

	LOG_DEBUG(LOG_CATEGORY_SIGNAL, "Signal °" + ntos(signal) + " catched.")
	if (signal == SIGHUP)
	{
		LOG_WARN(LOG_CATEGORY_CONFIG, "SIGHUP received: Reload config")
		Taskmaster::GetInstance()->reloadConfigFile();
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
	(void)ac;
	(void)av;
	struct sigaction sa;
	Taskmaster *TM = Taskmaster::GetInstance();


#if LOG_CATEGORY_AUTO == false
	TM->initCategories();
#endif

	if (TM->isRunningRootPermissions() == false)
	{
		LOG_DEBUG(LOG_CATEGORY_INIT, "You must haved to run this program.")
		LOG_WARN(LOG_CATEGORY_DEFAULT, "You must have root permissions to run this program.")
		LOG_ERROR(LOG_CATEGORY_INIT, "You must have root pdsffsdfsdsdfssfdermissions to run this program.")
		LOG_CRITICAL(LOG_CATEGORY_NETWORK, "A big network error have root pdsffsdfsdsdfssfdermissions to run this program.")
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

	if (TM->loadConfigFile(TM_DEF_CONFIGPATH))
	{
		LOG_CRITICAL(LOG_CATEGORY_INIT, "Failed to load configuration file. Aborting")
		TM->exitProperly();
		exit(EXIT_FAILURE);
	}
	
	// Display all jobs
	std::cout << "=== JOBS ===" << std::endl;
	for (std::list<Job>::const_iterator it = TM->_joblist.begin(); it != TM->_joblist.end(); it++)
	{
		std::cout << *it;
	}
	// Here start to daemonize
	int i = 0;
	while (i < 60000)
	{
		usleep(100);
		i++;
	}

	TM->exitProperly();
	LOG_INFO(LOG_CATEGORY_INIT, "Quit program.")

	return EXIT_SUCCESS;
}