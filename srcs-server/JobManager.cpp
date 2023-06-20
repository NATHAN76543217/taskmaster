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
	LOG_INFO(LOG_CATEGORY_THREAD, THREADTAG_JOBMANAGER << " wait to start");
	{
		std::unique_lock<std::mutex> lock_start(this->_internal_mutex);
		this->_ready.wait(lock_start);
	}
	LOG_INFO(LOG_CATEGORY_THREAD, THREADTAG_JOBMANAGER << "Thread start")
	LOG_INFO(LOG_CATEGORY_JM, THREADTAG_JOBMANAGER << "Start")

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
			for (std::list<Job>::iterator it = this->_runningjobs.begin(); it != this->_runningjobs.end(); it++ )
			{
				if (it->getStatus() == incomplete)
				{
					/* Need update */
				}
			}
			
		}

		if (!tmpCycleUsefull)
			LOG_CRITICAL(LOG_CATEGORY_JM, "Mutex released but nothing happen...") 
		LOG_DEBUG(LOG_CATEGORY_JM, "JobManager - Wait")
		this->_hasUpdate.wait(lock);
	}
	while (this->_running);
	LOG_INFO(LOG_CATEGORY_JM, THREADTAG_JOBMANAGER << "End.")
	LOG_INFO(LOG_CATEGORY_THREAD, THREADTAG_JOBMANAGER << "Thread end.")
}



int			JobManager::_updateConfig( void )
{
	std::list<Job> 	tmp;
	//TODO remove debug
	std::cout << std::endl << "JobSize before config check = " << this->_runningjobs.size() << std::endl;
	Taskmaster& TM = Taskmaster::GetInstance();


	/* Update the configuration */
	for (std::list<Job>::const_iterator job = TM._joblist.begin(); job != TM._joblist.end(); job++)
	{
		std::list<Job>::iterator rjob = this->_runningjobs.begin();
		/* find if job is in the running list */
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
			tmp.push_back(*job);
			Job & changedJob = tmp.back();
			changedJob.setStatus(incomplete);
			changedJob._setpid(rjob->_getpid());
		}
	}
	/* Remove job not found in new config */
	bool found = false;		
	for (std::list<Job>::iterator oldjob = this->_runningjobs.begin(); oldjob != this->_runningjobs.end(); oldjob++)
	{
		std::list<Job>::const_iterator newjob = TM._joblist.begin();
	
		/* find if oldjob is in the new list */
		for (; newjob != TM._joblist.end() ; newjob++ )
		{
			if (newjob->getName().compare(oldjob->getName()) == 0)
				found = true;
		}
		if (found == false)
		{
			if (oldjob->gracefullStop() == EXIT_SUCCESS)
				LOG_INFO(LOG_CATEGORY_JM, "Gracefuly stop job `" << oldjob->getName() << "`.")
			else
				LOG_ERROR(LOG_CATEGORY_JM, "Failed to gracefuly stop job `" << oldjob->getName() << "`.")
		}
		found = false;
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



/*
	Update child processes status with waitpid's infos
*/
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
		if (WIFEXITED(stat))
		{
			uint return_value = WEXITSTATUS(stat);
			if (return_value == 0)
			{
				it->setChildStatus(pid, terminated);
			}
			else
			{
				it->setStatus(incomplete);
				it->rmPid(pid);
			}
		}
		else if (WIFSIGNALED(stat))
		{
			switch(WTERMSIG(stat))
			{
				// TODO handle many signals here
				case SIGTERM:
				case SIGINT:
				break;
				default:
					it->setChildStatus(pid, killed);
			}
			it->setStatus(incomplete);
			it->rmPid(pid);
		}
		else if (WIFSTOPPED(stat))
		{
			switch (WSTOPSIG(stat))
			{
				case SIGTSTP:
					it->setChildStatus(pid, suspended);
				break;
				case SIGSTOP:
					it->setChildStatus(pid, stopped);
				break;
				default:
					it->setChildStatus(pid, stopped);
					LOG_CRITICAL(LOG_CATEGORY_JM, "Unknown stop signal " << WSTOPSIG(stat) << ".")
			}
		}
		else
		{
			LOG_CRITICAL(LOG_CATEGORY_JM, "Unknown state notified.")
		}
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





/* ************************************************************************** */