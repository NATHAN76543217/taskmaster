#include "JobManager.hpp"
/* Static methods definitions and static variables initalisation. */
// std::atomic<JobManager*>	JobManager::jobManager_ ;
//TODO I don't know why but JobManager is created twice
// JobManager*	JobManager::GetInstance( void )
// {
//     /**
//      * This is a safer way to create an instance. instance = new Singleton is
//      * dangeruous in case two instance threads wants to access at the same time
//      */
//     if(jobManager_.load() == nullptr)
// 	{
// 		LOG_INFO(LOG_CATEGORY_THREAD, "Create a new jobManager in thread " << std::this_thread::get_id())
//         jobManager_.store(new JobManager());
// 		//REVIEW Is it smart to init JobLanager handlers here ? 
// 		// jobManager_.load()->initSignalsJm();
// 		// jobManager_->startThreadJM();
//     }
//     return jobManager_.load();
// }

// void	JobManager::DestroyInstance( void )
// {
// 	if(jobManager_.load() == nullptr)
// 		return ;
// 	jobManager_.load()->_stopThreadJM();
// 	delete jobManager_.load();
// 	jobManager_.store(nullptr);

// }



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
	std::cerr << "JobManager wait to start" << std::endl;
	{
		std::unique_lock<std::mutex> lock_start(this->_internal_mutex);
		this->_ready.wait(lock_start);
	}
	std::cerr << "JobManager start." << std::endl;

	/* Init */
	LOG_INFO(LOG_CATEGORY_THREAD, "JM thread - start - id: " << std::this_thread::get_id())
	bool	tmpCycleUsefull = false;
	std::unique_lock<std::mutex> lock(this->_internal_mutex);

	/* Loop */
	do {
		tmpCycleUsefull = false;
		LOG_DEBUG(LOG_CATEGORY_JM, "JobManager - Mutex released")
		if (!this->_running)
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
			std::lock_guard<std::mutex> lk(this->_mutexChildlist);
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
	while (this->_running);
	LOG_INFO(LOG_CATEGORY_THREAD, "JM thread - end - id: " << std::this_thread::get_id())
}

int			JobManager::_updateConfig( void )
{

	Taskmaster& TM = Taskmaster::GetInstance();
	/* Update the configuration */
	for (std::list<Job>::const_iterator job = TM._joblist.begin(); job != TM._joblist.end(); job++)
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
	{
		std::lock_guard<std::mutex> lock(this->_mutexChildlist);
		this->_childChanged = true;
		this->_changedChilds.push_back(pid);
	}
	this->_hasUpdate.notify_one();
	(void) stat;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/


void		JobManager::stop( void )
{
	AThread<JobManager>::stop();
	this->_hasUpdate.notify_one();
}


void			JobManager::setConfigChanged( void )
{
	{
		std::lock_guard<std::mutex> lk(this->_internal_mutex);
		this->_configChanged = true;
	}
	this->_hasUpdate.notify_one();
}


/*
** --------------------------------- SIGNALS ---------------------------------
*/

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


/* ************************************************************************** */