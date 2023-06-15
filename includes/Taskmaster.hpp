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
# include "AThread.hpp"
# include "Tintin_reporter.hpp"
# include "SignalCatcher.hpp"
# include "JobManager.hpp"
# include "Job.hpp"

bool		isRunningRootPermissions( void );

class Taskmaster : public virtual AThread<Taskmaster>
{
	private:
		Config					_config;
		std::condition_variable _ready;
		/* Config values */
		
		std::string		_lockpath;
		std::string		_logpath;
		uint			_max_connections;
		bool			_logcolor;
		std::atomic<const char**>	_parentEnv; 
		std::list<Job>			_joblist;

	protected:
		/* Constructor */
		Taskmaster() :
			AThread(*this, "Taskmaster"),
			_ready(),
			_lockpath(TM_DEF_LOCKPATH),
			_logpath(TM_DEF_LOGPATH),
			_max_connections(TM_DEF_MAX_CONNECTIONS),
			_logcolor(TM_DEF_LOGCOLOR),
		{
			this->_parentEnv.store(nullptr);
		}
		/* Destructor */
		// ~Taskmaster() {}

		/* Uniq instance */
		// static Taskmaster* taskmaster_;

		int			_parseConfigServer( void );
		int			_parseConfigPrograms( void );



	public:


		void		operator()( void );


		/* Init */
		int			initialization( const char** env );
		void		initCategories( void ) const;
		pid_t		getpid( void ) const;

		const char** getEnv( void ) const;
		void		setEnv( const char **env);

		int			loadConfigFile(const std::string & path);
		int			reloadConfigFile( void );
		
		void		startThreads( void );
		void		stopThreads( void );

		int			exitProperly( void );

	// friend class JobManager;
	friend class AThread;
};

//REVIEW Not implemented for now
// std::ostream &			operator<<( std::ostream & o, Taskmaster const & i );

#endif /* ****************************************************** TASKMASTER_H */