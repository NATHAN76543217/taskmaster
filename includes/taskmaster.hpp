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
# include "Job.hpp"
# include "Tintin_reporter.hpp"



class Taskmaster
{
	private:
		pid_t		_pid;
		Config		_config;

		/* Config values */
		std::string	_lockpath;
		std::string	_logpath;
		uint		_max_connections;
		bool		_logcolor;

	protected:
		/* Constructor */
		Taskmaster() : _pid(::getpid()), _lockpath(TM_DEF_LOCKPATH), _logpath(TM_DEF_LOGPATH), _max_connections(TM_DEF_MAX_CONNECTIONS), _logcolor(TM_DEF_LOGCOLOR) 
		{
			(void) _max_connections;
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

		int			reloadConfigFile( void );
		int			exitProperly( void );

//REVIEW move this to prive
		std::list<Job>	_joblist;

};


std::ostream &			operator<<( std::ostream & o, Taskmaster const & i );

#endif /* ****************************************************** TASKMASTER_H */