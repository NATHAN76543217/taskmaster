#include "SignalCatcher.hpp"

/* TODO implement SIGINFO to log infos about current state of the system */

void fake_sigchld_handler(int signo) {
	std::cout << "SIGCHLD : " << signo << std::endl;
}

void			SignalCatcher::operator()()
{
	LOG_INFO(LOG_CATEGORY_THREAD, "Wait to start")
	{
		std::unique_lock<std::mutex> lock(this->_internal_mutex);
		this->_ready.wait(lock);
		lock.unlock();
	}
	int		sigReceived = 0;
	int		process_status = 0;
	pid_t	child_pid = 0;

	LOG_INFO(LOG_CATEGORY_THREAD, "Start")
	LOG_INFO(LOG_CATEGORY_SIGNAL, "Start")

	sigemptyset(&(this->_sigSet));
	sigaddset(&(this->_sigSet), SIGINT);
	sigaddset(&(this->_sigSet), SIGHUP);
	sigaddset(&(this->_sigSet), SIGCHLD);
	sigaddset(&(this->_sigSet), SIGTERM);
	sigaddset(&(this->_sigSet), SIGSTOP);

	signal(SIGCHLD, fake_sigchld_handler);

	while(this->_running)
	{
		LOG_DEBUG(LOG_CATEGORY_SIGNAL, "Wait a signal")

		if (sigwait(&(this->_sigSet), &sigReceived) != 0)
		{
			LOG_CRITICAL(LOG_CATEGORY_SIGNAL, "Failed to `sigwait` : Invalid signal number.")
			break ;
		}

		LOG_DEBUG(LOG_CATEGORY_SIGNAL, "Signal received: N° " << sigReceived << ".")

		process_status = 0;
		child_pid = 0;
		switch(sigReceived)
		{
			case SIGINT:
				LOG_INFO(LOG_CATEGORY_SIGNAL, "SIGINT - Stop Taskmaster.")
				/* Also Stop SignalCatcher Thread */
				Taskmaster::GetInstance().stop();
				// this->stop();//REVIEW
				goto exit_tag;
				break ;
			case SIGHUP:
				LOG_INFO(LOG_CATEGORY_SIGNAL, "SIGHUP - Reload config.")
				Taskmaster::GetInstance().reloadConfigFile();

				break;
			case SIGCHLD:
				if ((child_pid = waitpid(0, &process_status, 0)) == -1) // TODO WNOHANG
				{
					LOG_ERROR(LOG_CATEGORY_JM, THREADTAG_SIGNALCATCHER << "Failed to `waitpid` : " << strerror(errno))
					break ;
				}
				LOG_INFO(LOG_CATEGORY_SIGNAL, "SIGCHLD - Child [" << child_pid << "] status changed.")
				JobManager::GetInstance().notifyChildDeath(child_pid, process_status);

			break;
			default:
				LOG_ERROR(LOG_CATEGORY_SIGNAL, "Invalid signal number (" << sigReceived << ").")
				break;
		}
	}

exit_tag:
	LOG_INFO(LOG_CATEGORY_SIGNAL, "End.")
	return ;
}

/*
** --------------------------------- METHODS ----------------------------------
*/


/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */