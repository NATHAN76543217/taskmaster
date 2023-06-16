#include "SignalCatcher.hpp"


// std::ostream &			operator<<( std::ostream & o, SignalCatcher const & i )
// {
// 	//o << "Value = " << i.getValue();
// 	return o;
// }

void			SignalCatcher::operator()()
{
	std::cout << "SignalCatcher wait to start" << std::endl;
	{

		std::unique_lock<std::mutex> lock(this->_internal_mutex);
		this->_ready.wait(lock);
	}
	std::cout << "Signal catcher start." << std::endl;

	LOG_INFO(LOG_CATEGORY_THREAD, THREADTAG_SIGNALCATCHER << "SC thread - start - id: " << std::this_thread::get_id())
	LOG_INFO(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "Start thread - SignalCatcher")

	sigemptyset(&(this->_sigSet));
	sigaddset(&(this->_sigSet), SIGINT);
	// sigaddset(&(this->_sigSet), SIGTERM);
	sigaddset(&(this->_sigSet), SIGHUP);
	sigaddset(&(this->_sigSet), SIGCHLD);

	while(this->_running)
	{
		LOG_DEBUG(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << " - Wait a signal - ")

		// if (sigsuspend(&(this->_sigSet)) != -1)
		if (sigwait(&(this->_sigSet), &(this->_sigNum)) != 0)
		{
			LOG_CRITICAL(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "Failed to `sigwait` : Invalid signal number.")
			break ;
		}

		LOG_DEBUG(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "Signal received: N° " << this->_sigNum << ".")

		int		stat = 0;
		pid_t	child_pid = 0;
		switch(this->_sigNum)
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
				JobManager::GetInstance().setConfigChanged();

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
				LOG_ERROR(LOG_CATEGORY_JM, THREADTAG_SIGNALCATCHER << "Invalid signal number.")
				break;
		}
	}
exit_tag:
	LOG_INFO(LOG_CATEGORY_SIGNAL, THREADTAG_SIGNALCATCHER << "Out thread - SignalCatcher")
}

/*
** --------------------------------- METHODS ----------------------------------
*/


/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */