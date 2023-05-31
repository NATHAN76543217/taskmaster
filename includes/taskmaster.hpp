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
# include "Config.hpp"
# include "Tintin_reporter.hpp"

# define TM_DEF_MAX_CONNECTIONS 3
# define TM_DEF_CONFIGPATH "./config_template.yaml"
# define TM_DEF_LOCKPATH "./test.lock"
// # define TM_DEF_LOCKPATH "/var/lock/taskmaster.lock"
# define TM_DEF_LOGPATH "/var/log/taskmaster/taskmaster.log"

class Taskmaster
{
	private:
		std::string	_lockpath;
		bool		_logcolor;
		std::string	_logpath;
		uint		_max_connections;
		pid_t		_pid;
		Config		_config;

	protected:
		Taskmaster() : _lockpath(TM_DEF_LOCKPATH), _logpath(TM_DEF_LOGPATH), _max_connections(TM_DEF_MAX_CONNECTIONS), _pid(::getpid())
		{
			(void) _max_connections;
		}
		~Taskmaster() {}

		static Taskmaster* taskmaster_;

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

		int			parse_arguments( void );
		
		int			loadConfigFile(const std::string & path);
		int			reloadConfigFile( void );
		int			exitProperly( void );

};


/* Methods implementation */


void		Taskmaster::takeLockFile( void ) const
{
	int fd = 0;

	if ((fd = open(this->_lockpath.c_str(), O_CREAT|O_EXCL )) == -1) {
		if (errno == EEXIST)
		{
			LOG_CRITICAL(LOG_CATEGORY_INIT, "The lock file already exist. Impossible to start a second instance of this program")
		}
		else
			perror("open");
		//TODO exit properly
		exit(1);
	}
	LOG_INFO(LOG_CATEGORY_INIT, "Lock file '" + this->_lockpath + "' successfuly taken.");
	close(fd);
}

void		Taskmaster::freeLockFile( void ) const
{
	std::remove(this->_lockpath.c_str() );
}

bool		Taskmaster::isRunningRootPermissions( void ) const
{
	uid_t euid = geteuid();
	if (euid != 0)
		return false;
	return true;	
}

void		Taskmaster::initCategories( void ) const
{
	Tintin_reporter::getLogManager("./default.log").addCategory(LOG_CATEGORY_DEFAULT);
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_INIT);
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_NETWORK);
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_SIGNAL, "./signal.log");
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_CONFIG);
}

int			Taskmaster::parse_arguments( void )
{
	return EXIT_SUCCESS;
}

int			Taskmaster::loadConfigFile(const std::string & path)
{
	if (this->_config.loadConfigFile(path) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}
	if (this->_config["server"])
	{
		if (this->_config["server"]["logcolor"])
		{
			this->_logcolor = this->_config["server"]["logcolor"].as<bool>();
			Tintin_reporter::getLogManager().setColor(this->_logcolor);
		}
	}
	return EXIT_SUCCESS;
}

int			Taskmaster::reloadConfigFile( void )
{
	if (this->_config.reloadConfigFile())
	{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int			Taskmaster::exitProperly( void )
{
	this->freeLockFile();
	LOG_INFO(LOG_CATEGORY_INIT, "Lock file '" + this->_lockpath + "' successfuly released.")
	return EXIT_SUCCESS;
}


/**
 * Static methods should be defined outside the class.
 */
Taskmaster* Taskmaster::taskmaster_= nullptr;

Taskmaster *Taskmaster::GetInstance()
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
    if(taskmaster_==nullptr){
        taskmaster_ = new Taskmaster();
    }
    return taskmaster_;
}

pid_t			Taskmaster::getpid( void ) const
{
	return this->_pid;
}


std::ostream &			operator<<( std::ostream & o, Taskmaster const & i );

#endif /* ****************************************************** TASKMASTER_H */