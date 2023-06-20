#include "SignalCatcher.hpp"


// std::ostream &			operator<<( std::ostream & o, SignalCatcher const & i )
// {
// 	//o << "Value = " << i.getValue();
// 	return o;
// }

void			SignalCatcher::operator()()
{
	LOG_INFO(LOG_CATEGORY_THREAD, THREADTAG_SIGNALCATCHER << "Wait to start")
	{
		std::unique_lock<std::mutex> lock(this->_internal_mutex);
		this->_ready.wait(lock);
		lock.unlock();
	}
	int		sigReceived = 0;
	int		stat = 0;
	pid_t	child_pid = 0;

	LOG_INFO(LOG_CATEGORY_THREAD, THREADTAG_SIGNALCATCHER << "Thread start")
	LOG_INFO(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "Start")

	sigemptyset(&(this->_sigSet));
	sigaddset(&(this->_sigSet), SIGINT);
	// sigaddset(&(this->_sigSet), SIGTERM);
	sigaddset(&(this->_sigSet), SIGHUP);
	sigaddset(&(this->_sigSet), SIGCHLD);
	sigaddset(&(this->_sigSet), SIGTERM);
	sigaddset(&(this->_sigSet), SIGSTOP);

	while(this->_running)
	{
		LOG_DEBUG(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << " - Wait a signal - ")

		if (sigwait(&(this->_sigSet), &sigReceived) != 0)
		{
			LOG_CRITICAL(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "Failed to `sigwait` : Invalid signal number.")
			break ;
		}

		LOG_DEBUG(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "Signal received: N° " << sigReceived << ".")

		stat = 0;
		child_pid = 0;
		switch(sigReceived)
		{
			case SIGINT:
				LOG_INFO(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "SIGINT - Stop Taskmaster.")
				/* Also Stop SignalCatcher Thread */
				Taskmaster::GetInstance().stop();
				// this->stop();
				goto exit_tag;
			break ;
			case SIGHUP:
				LOG_INFO(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "SIGHUP - Reload config.")
				Taskmaster::GetInstance().reloadConfigFile();

			break;
			case SIGCHLD:
				LOG_INFO(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "SIGCHLD - A child had exited.")
				if ((child_pid = waitpid(0, &stat, 0)) == -1)
				{
					LOG_ERROR(LOG_CATEGORY_JM, THREADTAG_SIGNALCATCHER << "Failed to `waitpid` : " << strerror(errno))
					break ;
				}
				JobManager::GetInstance().notifyChildDeath(child_pid, stat);

			break;
			default:
				LOG_ERROR(LOG_CATEGORY_JM, THREADTAG_SIGNALCATCHER << "Invalid signal number (" << sigReceived << ").")
				break;
		}
	}

exit_tag:
	LOG_INFO(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "End.")
	return ;
}

/*
** --------------------------------- METHODS ----------------------------------
*/


/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */