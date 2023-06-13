#ifndef JOBMANAGER_HPP
# define JOBMANAGER_HPP

# include <iostream>
# include <string>
# include <chrono>
# include <atomic>
# include <mutex>
# include <thread>
# include <condition_variable>

class JobManager;

# include "Job.hpp"
# include "Tintin_reporter.hpp"
# include "Taskmaster.hpp"

class JobManager
{
	/* Private variables */
	private:
		struct sigaction			_sig_tm;
		std::thread					_threadJM;
		bool						_shouldStop;
		std::list<Job>				_runningjobs;
		std::mutex					_mutexChildlist;
		std::vector<pid_t>			_changedChilds;
		std::mutex					_mutex;
		std::condition_variable		_hasUpdate;
		// bool						_somethingChanged;
		bool						_childChanged;//TODO remove this variable
		bool						_configChanged;

	protected:
	/* Constructor */ 
	//TODO init all variables
		JobManager() : 
			_threadJM(std::thread(std::ref(*this))),
			_shouldStop(false),
			_hasUpdate(),
			// _somethingChanged(false),
			_childChanged(false),
			_configChanged(false)
		{

		};

	/* Destructor */ 
		~JobManager() {
		};

	/* unique instance */ 
		static std::atomic<JobManager*>	jobManager_;
	
		int		_updateConfig( void );

	public:
	/* Singletons should not be cloneable. */
		JobManager(JobManager &other) = delete;
		JobManager(const JobManager &other) = delete;
	/* Singletons should not be assignable. */
		void	operator=(const JobManager &) = delete;

	/* Logic loop for execution */
	void	operator()();

	/* Unique instance getter and destroyer */
		static JobManager*	GetInstance( void );
		static void			DestroyInstance( void );
	/* JobManager methods */

	void			setConfigChanged( void );
	int				initSignalsJm( void );
	static void		signalHandler( int signal );
	void			stop( bool stop );
	void			notifyChildDeath( pid_t pid, int stat);
	// int			startThreadJM( void );
	int			stopThreadJM( void );
};

std::ostream &			operator<<( std::ostream & o, JobManager const & i );

#endif /* ****************************************************** JOBMANAGER_H */