#include "JobManager.hpp"


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
	LOG_INFO(LOG_CATEGORY_THREAD, "Thread `JobManager` - start")
	bool	tmpCycleUsefull = false;
	std::unique_lock<std::mutex> lock(this->_internal_mutex);

	/* Loop */
	do {
		tmpCycleUsefull = false;
		LOG_DEBUG(LOG_CATEGORY_JM, "JobManager - Mutex released")
		if (!this->_running)
		{

			LOG_CRITICAL(LOG_CATEGORY_JM, "JobManager stoped. (NOT SUPPOSED TO BE EXECUTED)")
			tmpCycleUsefull = false;
			break ;
		}
	
		if (this->_configChanged)
		{
			/* Update config */
			LOG_INFO(LOG_CATEGORY_JM, "Updating config...")
			tmpCycleUsefull = true;
			this->_configChanged = false;
			if (this->_updateConfig())
			{
				LOG_ERROR(LOG_CATEGORY_JM, "Fail to update config.")
			}
			else
				LOG_INFO(LOG_CATEGORY_JM, "Config updated successfuly.")

		}
		
		{
			/* Check here if jobs differ from their max state */
			//TODO
			
		}

		if (!tmpCycleUsefull)
			LOG_CRITICAL(LOG_CATEGORY_JM, "Mutex released but nothing happen...") 
		LOG_DEBUG(LOG_CATEGORY_JM, "JobManager - Wait")
		this->_hasUpdate.wait(lock);
	}
	while (this->_running);
	LOG_INFO(LOG_CATEGORY_THREAD, "JM thread - end ")
}

int			JobManager::_updateConfig( void )
{
	std::list<Job> 	tmp;
	
	std::cout << std::endl << "JobSize before config check = " << this->_runningjobs.size() << std::endl;
	Taskmaster& TM = Taskmaster::GetInstance();

	// std::lock_guard<std::mutex> lock(TM._internal_mutex);


	/* Update the configuration */
	for (std::list<Job>::const_iterator job = TM._joblist.begin(); job != TM._joblist.end(); job++)
	{
		/* find job in running list */
		std::list<Job>::iterator rjob = this->_runningjobs.begin();
		for (; rjob != this->_runningjobs.end() ; rjob++ )
		{
			LOG_DEBUG(LOG_CATEGORY_CONFIG, "Compare service: " << job->getName() << " against " << rjob->getName() )
			if (Job::Compare(*job, *rjob) == 0)
			{
				LOG_DEBUG(LOG_CATEGORY_CONFIG, "Match service: " << job->getName())
				break ;
			}
		}
		if (rjob == this->_runningjobs.end())
		{
			/* Not found */
			/* A new job to start */
			//TODO move this part outside this function
			LOG_INFO(LOG_CATEGORY_JM, "Job '" << job->getName() << "' detected as a new job.")
			tmp.push_back(*job);
			Job & newjob = tmp.back();
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
	this->_runningjobs = tmp;
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

std::list<Job>::iterator		JobManager::getJobByPid(pid_t pid)
{
	std::list<Job>::iterator it = this->_runningjobs.begin();
	
	while (it != this->_runningjobs.end())
	{
		if (it->hasPid(pid) == true)
			break ;
		it++;
	}

	return it;
}

void		JobManager::notifyChildDeath( pid_t pid, int stat)
{
	{
		std::lock_guard<std::mutex> lock(this->_internal_mutex);
		std::list<Job>::iterator it = this->getJobByPid(pid);
		if (it == this->getJobEnd())
		{
			//not found
			LOG_CRITICAL(LOG_CATEGORY_JM, "Notify the death of a pid not found in JobManager")
			return ;
		}
		it->setStatus(incomplete);
		it->rmPid(pid);
	}
	this->_hasUpdate.notify_one();
	(void) stat;
}


/*
** --------------------------------- ACCESSOR ---------------------------------
*/


void		JobManager::stop( void )
{
	/* Stop jobs before stopping JobManager */
	for (std::list<Job>::iterator it = this->_runningjobs.begin(); it != this->_runningjobs.end(); it++)
	{
		LOG_INFO(LOG_CATEGORY_JM, "Gracefuly stopping job `" << it->getName() << "`.")
		it->gracefullStop();
	}
	LOG_INFO(LOG_CATEGORY_JM, "All jobs stopped.")

	/* Stop jobManager */
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

std::list<Job>::iterator	JobManager::getJobEnd( void )
{
	return this->_runningjobs.end();
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