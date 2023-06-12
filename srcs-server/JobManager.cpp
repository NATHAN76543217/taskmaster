#include "JobManager.hpp"
# include <chrono>
/* Static methods definitions and static variables initalisation. */
std::atomic<JobManager*>	JobManager::jobManager_ ;

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
	Taskmaster* TM = Taskmaster::GetInstance();
	LOG_INFO(LOG_CATEGORY_THREAD, "JM thread -  operator() called - " << std::this_thread::get_id())
	while ( !this->_shouldStop )
	{
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
		this->stop(true);
		std::this_thread::sleep_for(std::chrono::seconds(5));
		LOG_INFO(LOG_CATEGORY_THREAD, "JM thread - loop - " << std::this_thread::get_id())
	}
	LOG_INFO(LOG_CATEGORY_THREAD, "JM thread - wake - " << std::this_thread::get_id())
}


/*
** --------------------------------- METHODS ----------------------------------
*/

int			JobManager::startThreadJM( void )
{
	// this->_threadJM(std::ref(*this));
	this->_threadJM = std::thread(std::ref(*this));

	return EXIT_SUCCESS;
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
	this->_shouldStop = stop;
}

/* ************************************************************************** */