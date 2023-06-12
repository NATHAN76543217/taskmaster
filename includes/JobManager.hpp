#ifndef JOBMANAGER_HPP
# define JOBMANAGER_HPP

# include <iostream>
# include <string>
# include <atomic>
# include <thread>

class JobManager;

# include "Job.hpp"
# include "Tintin_reporter.hpp"
# include "Taskmaster.hpp"

class JobManager
{
	/* Private variables */
	private:
		std::thread		_threadJM;
		bool			_shouldStop;
		std::list<Job>	_runningjobs;

	protected:
	/* Constructor */ 
		JobManager() : _threadJM(std::thread(std::ref(*this))), _shouldStop(false) {

		};

	/* Destructor */ 
		~JobManager() {

		};

	/* unique instance */ 
		static std::atomic<JobManager*>	jobManager_;

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

	void		stop( bool stop );
	int			startThreadJM( void );
	int			stopThreadJM( void );
};

std::ostream &			operator<<( std::ostream & o, JobManager const & i );

#endif /* ****************************************************** JOBMANAGER_H */