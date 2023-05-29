# include "taskmaster.hpp"

//TODO implement logging to files
//TODO Finish to well format the LOGs
//TODO Implement yamlLib and start parsing config file

int	parse_arguments( void )
{
	return EXIT_SUCCESS;
}


void	signal_handler(int signal)
{
	dprintf(STDERR_FILENO, "Signal caught !!!\n");
	LOG_INFO(LOG_CATEGORY_SIGNAL, "Signal catched" + ntos(signal))
	exit(0);
}


int	init_signals(struct sigaction *sa)
{
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_SIGNAL);

	memset(sa, 0, sizeof(struct sigaction));
	// sa.sa_sigaction = signal_handler;
	sigemptyset(&sa->sa_mask);
	sa->sa_handler = signal_handler;
	sa->sa_flags = 0; 
	// sa->sa_flags = SA_SIGINFO; 
	// sigaddset(&sa.sa_mask, SIGINT);
	// sigprocmask(&sa.sa_mask);
	sigaction(SIGINT, sa, NULL);
	// if (signal(SIGQUIT, signal_handler) == SIG_ERR)
		// return EXIT_FAILURE;
	// if (signal(SIGINT, signal_handler) == SIG_ERR)
		// return EXIT_FAILURE;
	LOG_INFO(LOG_CATEGORY_SIGNAL, "Signal handlers successfuly started")
	
	return EXIT_SUCCESS;
}


int	check_root_permissions()
{
	uid_t euid = geteuid();
	if (euid != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}


// TODO ptrace on ourselves to avoid process monitoring


int main(int ac, char** av)
{
	(void)ac;
	(void)av;
	struct sigaction sa;

	LOG_INFO(LOG_CATEGORY_DEFAULT, "PID: " + ntos(getpid()))

	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_INIT);
	if (check_root_permissions() != EXIT_SUCCESS)
	{
		LOG_ERROR(LOG_CATEGORY_INIT, "You must have root permissions to run this program.")
		return EXIT_FAILURE;
	}
	if (parse_arguments() != EXIT_SUCCESS)
	{
		LOG_ERROR(LOG_CATEGORY_INIT, "Failed to parse arguments.")
		return EXIT_FAILURE;
	}
	if (init_signals(&sa) != EXIT_SUCCESS)
	{
		LOG_ERROR(LOG_CATEGORY_INIT, "Failed to init signal handlers.")
		return EXIT_FAILURE;
	}
	// Here start to daemonize
	int i = 0;
	while (i < 40000)
	{
		usleep(100);
		i++;
	}
	return EXIT_SUCCESS;
}