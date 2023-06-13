#include "JobManager.hpp"
/* Static methods definitions and static variables initalisation. */
std::atomic<JobManager*>	JobManager::jobManager_ ;
//TODO I don't know why but JobManager is created twice
JobManager*	JobManager::GetInstance( void )
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
    if(jobManager_.load() == nullptr)
	{
		LOG_INFO(LOG_CATEGORY_THREAD, "Create a new jobManager in thread " << std::this_thread::get_id())
        jobManager_.store(new JobManager());
		//REVIEW Is it smart to init JobLanager handlers here ? 
		jobManager_.load()->initSignalsJm();
		// jobManager_->startThreadJM();
    }
    return jobManager_.load();
}

void	JobManager::DestroyInstance( void )
{
	if(jobManager_.load() == nullptr)
		return ;
	jobManager_.load()->stopThreadJM();
	delete jobManager_.load();
	jobManager_.store(nullptr);

}



/*
** --------------------------------- OVERLOAD ---------------------------------
*/

//TODO implement
// std::ostream &			operator<<( std::ostream & o, JobManager const & i )
// {
// 	//o << "Value = " << i.getValue();
// 	return o;
// }
void			JobManager::operator()()
{
	/* Init */
	LOG_INFO(LOG_CATEGORY_THREAD, "JM thread - start - id: " << std::this_thread::get_id())
	bool	tmpCycleUsefull = false;
	std::unique_lock<std::mutex> lock(this->_mutex);

	/* Loop */
	do {
		tmpCycleUsefull = false;
		LOG_DEBUG(LOG_CATEGORY_JM, "JobManager - Mutex released")
		if (this->_shouldStop)
		{
			LOG_DEBUG(LOG_CATEGORY_JM, "Marked as should_stop.")
			tmpCycleUsefull = true;
			break ;
		}
	
		if (this->_configChanged)
		{
			/* Update config */
			LOG_INFO(LOG_CATEGORY_JM, "Updating config...")
			tmpCycleUsefull = true;
			if (this->_updateConfig())
			{
				LOG_ERROR(LOG_CATEGORY_JM, "Fail to update config.")
			}

		}
		
		{
			std::lock_guard<std::mutex> lock(this->_mutexChildlist);
			tmpCycleUsefull = true;
			for (std::vector<pid_t>::iterator it_pid = this->_changedChilds.begin(); it_pid != this->_changedChilds.end(); it_pid++)
			{
				LOG_INFO(LOG_CATEGORY_JM, "JobManager - main loop - Je sais que pid " << *it_pid << " est mort.")
			}
			
		}

		if (!tmpCycleUsefull)
		{
			LOG_CRITICAL(LOG_CATEGORY_JM, "Mutex released but nothing happen...")
		}
		LOG_DEBUG(LOG_CATEGORY_JM, "JobManager - Wait")
		this->_hasUpdate.wait(lock);
	}
	while (true);
	LOG_INFO(LOG_CATEGORY_THREAD, "JM thread - end - id: " << std::this_thread::get_id())
}

int			JobManager::_updateConfig( void )
{
	Taskmaster* TM = Taskmaster::GetInstance();
	/* Update the configuration */
	for (std::list<Job>::const_iterator job = TM->_joblist.begin(); job != TM->_joblist.end(); job++)
	{
		/* find job in running list */
		std::list<Job>::iterator rjob = this->_runningjobs.begin();
		for (;
			rjob != this->_runningjobs.end() && Job::Compare(*job, *rjob) != 0;
			rjob++ )
		{
		}
		if (rjob == this->_runningjobs.end())
		{
			/* Not found */
			/* A new job to start */
			LOG_INFO(LOG_CATEGORY_JM, "Job '" << job->getName() << "' detected as a new job.")
			this->_runningjobs.push_back(*job);
			Job& newjob = this->_runningjobs.back();
			if (newjob.getAutostart() == true)
			{
				LOG_DEBUG(LOG_CATEGORY_JM, "Job '" << job->getName() << "' start automatically.")				
				newjob.start();
				LOG_DEBUG(LOG_CATEGORY_JM, "Job '" << newjob.getName() << "' started.")
			}
		}
		else
		{
			/* Concern a started job */
			LOG_DEBUG(LOG_CATEGORY_JM, "Need to update job '" << rjob->getName() << "'. TO IMPLEMENT")
		}
	}
	return EXIT_SUCCESS;
}


/*
** --------------------------------- METHODS ----------------------------------
*/

// int			JobManager::startThreadJM( void )
// {
// 	// this->_threadJM(std::ref(*this));
// 	this->_threadJM = std::thread(std::ref(*this));

// 	return EXIT_SUCCESS;
// }

void		JobManager::notifyChildDeath( pid_t pid, int stat)
{
	std::lock_guard<std::mutex> lock(this->_mutexChildlist);
	this->_childChanged = true;
	this->_changedChilds.push_back(pid);
	this->_hasUpdate.notify_one();
	(void) stat;
}

int			JobManager::stopThreadJM( void )
{
	LOG_WARN(LOG_CATEGORY_THREAD, "Joining jobManager thread")
	this->_threadJM.join();
	LOG_WARN(LOG_CATEGORY_THREAD, "JobManager thread joined")
	return EXIT_SUCCESS;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/


void		JobManager::stop( bool stop )
{
	std::lock_guard<std::mutex> lk(this->_mutex);
	this->_shouldStop = stop;
	this->_hasUpdate.notify_one();
}

void			JobManager::setConfigChanged( void )
{
	std::lock_guard<std::mutex> lk(this->_mutex);
	this->_configChanged = true;
	this->_hasUpdate.notify_one();
}


/*
** --------------------------------- SIGNALS ---------------------------------
*/


int			JobManager::initSignalsJm( void )
{

	memset(&(this->_sig_tm), 0, sizeof(this->_sig_tm));

	sigemptyset(&(this->_sig_tm.sa_mask));
	this->_sig_tm.sa_handler = JobManager::signalHandler;
	this->_sig_tm.sa_flags = 0; 

	if (sigaction(SIGCHLD, &(this->_sig_tm), NULL))
	{
		LOG_ERROR(LOG_CATEGORY_SIGNAL, "Failed to `sigaction` : " << strerror(errno) )
		return EXIT_FAILURE;
	}
	LOG_INFO(LOG_CATEGORY_SIGNAL, "New handler for signal `SIGCHLD`.")

	LOG_INFO(LOG_CATEGORY_SIGNAL, "JobManager - Signal handlers successfuly started")
	
	return EXIT_SUCCESS;
}

void		JobManager::signalHandler( int signal )
{
	if (signal != SIGCHLD)
	{
		LOG_WARN(LOG_CATEGORY_SIGNAL, "Unhandled signal received [" << signal << "].")
		return ;
	}
	else
		LOG_DEBUG(LOG_CATEGORY_SIGNAL, "Signal °" + ntos(signal) + " catched.")
	
	LOG_DEBUG(LOG_CATEGORY_JM, "SIGCHLD : a child process is exited.")
	int		stat = 0;
	pid_t	child_pid = 0;
	if ((child_pid = waitpid(0, &stat, 0)) == -1)
	{
		LOG_ERROR(LOG_CATEGORY_JM, "Failed to `waitpid` : " << strerror(errno))
		return ;
	}
	// if (WIFEXITED(stat) || WIFSIGNALED(stat))
	// {
	// 	if (WIFEXITED(stat))
	// 	{
	// 		LOG_INFO(LOG_CATEGORY_JM, "Process [" << child_pid << "] exit with status '" << WEXITSTATUS(stat) << "'.")
	// 	}
	// 	else
	// 	{
	// 		LOG_INFO(LOG_CATEGORY_JM, "Process [" << child_pid << "] exit with signal '" << WTERMSIG(stat) << "'.")
	// 	}
	// 	//TODO implement JobManager::getJobByPid(), JobManager::jobEnd() qui retournent tout deux un iterateur
	// 	//get the job, remove from his list the terminated pid, store the status as exit or signaled, success or error

	// }
	// else
	// {
	// 	LOG_CRITICAL(LOG_CATEGORY_JM, "Process [" << child_pid << "] marked as 'not exited' and not 'signaled' but waitpid is set on 'NOHANG'.")
	// 	return ;
	// }

	JobManager::GetInstance()->notifyChildDeath(child_pid, stat);
}


/* ************************************************************************** */