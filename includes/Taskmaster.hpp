#ifndef TASKMASTER_HPP
# define TASKMASTER_HPP
 
# include <iostream>
# include <string>
# include <cstdlib>
# include <cstdio>
# include <unistd.h>
# include <fcntl.h>
# include <signal.h>


# include "yaml-cpp/yaml.h"
# include "tm_values.hpp"
# include "Config.hpp"
# include "Tintin_reporter.hpp"
# include "JobManager.hpp"
# include "Job.hpp"

class Taskmaster
{
	private:
		pid_t		_pid;
		Config		_config;
		JobManager* _jobManager;

		/* Config values */
		std::string		_lockpath;
		std::string		_logpath;
		uint			_max_connections;
		bool			_logcolor;
		bool			_shouldStop;
		std::atomic<const char**>	_parentEnv; 
		std::list<Job>			_joblist;

	protected:
		/* Constructor */
		Taskmaster() :
			_pid(::getpid()),
			_lockpath(TM_DEF_LOCKPATH),
			_logpath(TM_DEF_LOGPATH),
			_max_connections(TM_DEF_MAX_CONNECTIONS),
			_logcolor(TM_DEF_LOGCOLOR),
			_shouldStop(false)
		{
			this->_parentEnv.store(nullptr);
		}
		/* Destructor */
		~Taskmaster() {}

		/* Uniq instance */
		static Taskmaster* taskmaster_;

		int			_parseConfigServer( void );
		int			_parseConfigPrograms( void );



	public:

		/**
		 * Singletons should not be cloneable.
		 */
		Taskmaster(Taskmaster &other) = delete;
		Taskmaster(const Taskmaster &other) = delete;
		
		/**
		 * Singletons should not be assignable.
		 */
	    void	operator=(const Taskmaster &) = delete;

	    /**
		 * This is the static method that controls the access to the singleton
		 * instance. On the first run, it creates a singleton object and places it
		 * into the static field. On subsequent runs, it returns the client existing
		 * object stored in the static field.
		 */

		static Taskmaster *GetInstance();


		/**
		 * Finally, any singleton should define some business logic, which can be
		 * executed on its instance.
		 */

		void		initCategories( void ) const;
		pid_t		getpid( void ) const;
		void		takeLockFile( void ) const;
		void		freeLockFile( void ) const;
		bool		isRunningRootPermissions( void ) const;
		int			loadConfigFile(const std::string & path);
		const char** getEnv( void ) const;
		void		setEnv( const char **env);

		int			reloadConfigFile( void );
		int			startJobManager( void );
		void		stopJobManager( void );
		void		jobManager( void );
		void		stop( bool stop );
		bool		shouldStop( void ) const;
		int			exitProperly( void );

	friend class JobManager;
};

//REVIEW Not implemented for now
// std::ostream &			operator<<( std::ostream & o, Taskmaster const & i );

#endif /* ****************************************************** TASKMASTER_H */