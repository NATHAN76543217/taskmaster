#include "SignalCatcher.hpp"


// std::ostream &			operator<<( std::ostream & o, SignalCatcher const & i )
// {
// 	//o << "Value = " << i.getValue();
// 	return o;
// }


void			SignalCatcher::operator()()
{
	LOG_INFO(LOG_CATEGORY_THREAD, "SC thread - start - id: " << std::this_thread::get_id())
	LOG_INFO(LOG_CATEGORY_SIGNAL, "Start thread - SignalCatcher")

	sigemptyset(&(this->_sigSet));
	sigaddset(&(this->_sigSet), SIGINT);
	// sigaddset(&(this->_sigSet), SIGTERM);
	sigaddset(&(this->_sigSet), SIGHUP);
	sigaddset(&(this->_sigSet), SIGCHLD);

	this->_running = true;
	while(this->_running)
	{
		LOG_DEBUG(LOG_CATEGORY_SIGNAL, " - Wait a signal - ")

		// if (sigsuspend(&(this->_sigSet)) != -1)
		if (sigwait(&(this->_sigSet), &(this->_sigNum)) != 0)
		{
			LOG_CRITICAL(LOG_CATEGORY_SIGNAL, "Failed to `sigwait` : Invalid signal number.")
			break ;
		}

		LOG_DEBUG(LOG_CATEGORY_SIGNAL, "Signal received: N° " << this->_sigNum << ".")

		int		stat = 0;
		pid_t	child_pid = 0;
		switch(this->_sigNum)
		{
			case SIGINT:
				LOG_INFO(LOG_CATEGORY_SIGNAL, "SIGINT - Stop Taskmaster.")
				/* Also Stop SignalCatcher Thread */
				this->_running = false;
				Taskmaster::GetInstance()->stop(true);
			break;
			case SIGHUP:
				LOG_INFO(LOG_CATEGORY_SIGNAL, "SIGHUP - Reload config.")
				Taskmaster::GetInstance()->reloadConfigFile();
				JobManager::GetInstance().setConfigChanged();

			break;
			case SIGCHLD:
				LOG_INFO(LOG_CATEGORY_SIGNAL, "SIGCHLD - A child had exited.")
				if ((child_pid = waitpid(0, &stat, 0)) == -1)
				{
					LOG_ERROR(LOG_CATEGORY_JM, "Failed to `waitpid` : " << strerror(errno))
					break ;
				}
				JobManager::GetInstance().notifyChildDeath(child_pid, stat);

			break;
			default:
				LOG_ERROR(LOG_CATEGORY_JM, "Invalid signal number.")
				break;
		}
	} 
	LOG_INFO(LOG_CATEGORY_SIGNAL, "Out thread - SignalCatcher")
}

// void			SignalCatcher::_stopThreadSC( void )
// {
// 	LOG_WARN(LOG_CATEGORY_THREAD, "Joining SignalCatcher thread")
// 	this->_threadSC.join();
// 	LOG_WARN(LOG_CATEGORY_THREAD, "SignalCatcher thread joined")
// 	return EXIT_SUCCESS;
// }

/*
** --------------------------------- METHODS ----------------------------------
*/


/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */