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
	LOG_INFO(LOG_CATEGORY_JM, "Wait to start");
	{
		std::unique_lock<std::mutex> lock_start(JobManager::static_mutex);
		this->_ready.wait(lock_start);
	}
	LOG_INFO(LOG_CATEGORY_JM, "Start")

	std::this_thread::sleep_for(std::chrono::microseconds(10000));

	/* Init */
	bool	tmpCycleUsefull = false;
	std::unique_lock<std::mutex> lock(JobManager::static_mutex);

	/* Loop */
	do {
		tmpCycleUsefull = false;
		LOG_DEBUG(LOG_CATEGORY_JM, "JobManager - Mutex released")
		if (this->_running == false)
		{

			LOG_CRITICAL(LOG_CATEGORY_JM, "JobManager stopped. (NOT SUPPOSED TO BE EXECUTED)")
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
			/* Adapt job status according to child status */
			for (std::list<Job>::iterator job_it = this->_runningjobs.begin(); job_it != this->_runningjobs.end(); job_it++ )
			{
				/* TODO remove terminated process with restart policy never or onfailure : if all process of a job are done : set the job as done*/
					job_status jobFinalStatus = not_started;
					/* Need update */
					for (std::map<pid_t, child_status>::const_iterator child_it = job_it->_getpids().begin(); child_it != job_it->_getpids().end(); child_it++)
					{
						switch (child_it->second)
						{
							case child_not_started:
								/* Should start */
								break;
							case child_starting:
								/* REVIEW JOB status stay as starting as long it have a child with status child_starting */
								/* Check if starting since long enough */
									// jobFinalStatus = starting;
									jobFinalStatus = incomplete;
								break;
							case child_terminated:
								if (jobFinalStatus == not_started || jobFinalStatus == terminated)
									jobFinalStatus = terminated;
								break;
							case child_exited:
								if (jobFinalStatus != killed)
									jobFinalStatus = exited;
								break;
							case child_killed:
								jobFinalStatus = killed;
								/* Restart according to policy */
								break;
							case child_running:
								if (jobFinalStatus != terminated
									&& jobFinalStatus != exited
									&& jobFinalStatus != killed)
										jobFinalStatus = running;
									

								break;
							default:
								LOG_CRITICAL(LOG_CATEGORY_JOB, "We are supposed to handle every kind of child status")
						}
					}
					if (job_it->getStatus() != jobFinalStatus)
					{
						job_it->setStatus(jobFinalStatus);
						LOG_INFO(LOG_CATEGORY_JOB, job_it->getName() << " status set to : " << job_it->getStatusString(jobFinalStatus))
						tmpCycleUsefull = true;
					}
			}
			
		}

		if (!tmpCycleUsefull)
			LOG_CRITICAL(LOG_CATEGORY_JM, "Mutex released but nothing happen...") 
		LOG_DEBUG(LOG_CATEGORY_JM, "Wait an update")
		this->_hasUpdate.wait(lock);
	}
	while (this->_running);
	LOG_INFO(LOG_CATEGORY_JM, "End.")
}



int			JobManager::_updateConfig( void )
{
	std::list<Job> 	tmp;
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
			changedJob._setpid(rjob->_getpids());
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

/*
	Return an iterator to the job holding the process `pid`
	Otherwise return this->getJobEnd()
*/
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
		std::lock_guard<std::mutex> lock(JobManager::static_mutex);
		std::list<Job>::iterator job_it = this->getJobByPid(pid);

		if (job_it == this->getJobEnd())
		{
			LOG_CRITICAL(LOG_CATEGORY_JM, "Notify the death of a pid not existing in JobManager")
			return ;
		}

		if (WIFEXITED(stat))
		{
			uint return_value = WEXITSTATUS(stat);
			
			// TODO implement here multiple success values
			if (job_it->isSuccessExitCode(return_value))
				job_it->setChildStatus(pid, child_terminated);
			else
				job_it->setChildStatus(pid, child_exited);
			LOG_INFO(LOG_CATEGORY_JOB, "Child [" << pid << "] exited with code: " << return_value << ".")
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
					job_it->setChildStatus(pid, child_killed);
			}
			job_it->setStatus(incomplete);
			job_it->rmPid(pid);
		}
		else if (WIFSTOPPED(stat))
		{
			switch (WSTOPSIG(stat))
			{
				case SIGTSTP:
					job_it->setChildStatus(pid, child_suspended);
				break;
				case SIGSTOP:
					job_it->setChildStatus(pid, child_stopped);
				break;
				default:
					job_it->setChildStatus(pid, child_stopped);
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


void			JobManager::stop( void )
{
	/* Stop jobs before stopping JobManager */
	for (std::list<Job>::iterator it = this->_runningjobs.begin(); it != this->_runningjobs.end(); it++)
	{
		LOG_INFO(LOG_CATEGORY_JM, "Gracefuly stopping job `" << it->getName() << "`.")
		LOG_DEBUG(LOG_CATEGORY_JM, (*it))
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
		std::lock_guard<std::mutex> lk(JobManager::static_mutex);
		this->_configChanged = true;
	}
	this->_hasUpdate.notify_one();
}

std::list<Job>::iterator	JobManager::getJobEnd( void )
{
	return this->_runningjobs.end();
}





/* ************************************************************************** */