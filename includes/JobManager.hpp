#ifndef JOBMANAGER_HPP
# define JOBMANAGER_HPP

# include <chrono>
# include <condition_variable>

class JobManager;

# include "AThread.hpp"
# include "Job.hpp"
# include "Tintin_reporter.hpp"
# include "Taskmaster.hpp"

class JobManager : public virtual  AThread<JobManager>
{
	/* Private variables */
	private:
		std::list<Job>				_runningjobs;
		std::vector<pid_t>			_changedChilds;
		std::condition_variable		_hasUpdate;
		// bool						_somethingChanged;
		bool						_configChanged;

	protected:
	/* Constructor */ 
	//TODO init all variables
		JobManager(const std::string & name = "JobManager") :
			AThread<JobManager>(*this, name),
			_hasUpdate(),
			_configChanged(true)
		{
			LOG_DEBUG(LOG_CATEGORY_THREAD, "JobCatcher - Constructor")
		};

		~JobManager() {
		};

	
		int		_updateConfig( void );

	public:

	static JobManager&	GetInstance( const std::string & name = "JobManager" )
	{
		return AThread<JobManager>::GetInstance(name);
	}

	/* Logic loop for execution */
	void	operator()();

	/* JobManager methods */
	
	std::list<Job>::iterator	getJobEnd( void );
	std::list<Job>::iterator	getJobByPid(pid_t pid);

	void			setConfigChanged( void );
	void			stop( void );
	void			notifyChildDeath( pid_t pid, int stat);

	friend class AThread<JobManager>;
};

std::ostream &			operator<<( std::ostream & o, JobManager const & i );

#endif /* ****************************************************** JOBMANAGER_H */