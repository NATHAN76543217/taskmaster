#ifndef TASKMASTER_HPP
# define TASKMASTER_HPP
 
# include <iostream>
# include <string>
# include <cstdlib>
# include <cstdio>
# include <unistd.h>
# include <fcntl.h>
# include <csignal>

# include "yaml-cpp/yaml.h"
# include "tm_values.hpp"
# include "Config.hpp"
# include "AThread.hpp"
# include "Tintin_reporter.hpp"
# include "SignalCatcher.hpp"
# include "JobManager.hpp"
# include "Job.hpp"

class Taskmaster : public virtual AThread<Taskmaster>
{
	private:
		Config						_config;
		/* Config values */
		
		std::string					_workingdir;
		std::string					_logdir;
		uint						_max_connections;
		bool						_logcolor;
		std::atomic<const char**>	_parentEnv; 
		std::list<Job>				_joblist;

	protected:

	/* Constructor */
		Taskmaster(const std::string & name = "Taskmaster") :
			AThread(*this, name),
			_workingdir(""),
			_logdir(TM_DEF_LOGDIR),
			_max_connections(TM_DEF_MAX_CONNECTIONS),
			_logcolor(TM_DEF_LOGCOLOR)
		{
			char * pwd = NULL;
			pwd = ::getcwd(NULL, 0);
			if (pwd == NULL)
			{
				LOG_ERROR(LOG_CATEGORY_DEFAULT, "Failed to `getcwd` : " << strerror(errno))
			}
			else
			{
				this->_workingdir.assign(pwd);
				free(pwd);
			}
			this->_parentEnv.store(nullptr);
		}

		/* Destructor */
		~Taskmaster() {}

		int			_parseConfigServer( void );
		int			_parseConfigPrograms( void );

	public:

		static Taskmaster&			CreateInstance( const std::string & name );
		static void					Destroy( void );

		static	uint				getFieldStopSignal(YAML::const_iterator &it);
		static const std::string	getFieldStdout(YAML::const_iterator & it);
		void						operator()( void );

		void						startAll( void );

		/* Init */
		int					initialization( const char** env );
		void				initCategories( void ) const;

		void				setWorkingdir( const std::string & str);
		const std::string&	getWorkingdir( void ) const;

		int					loadConfigFile(const std::string & path);
		int					reloadConfigFile( void );

		/* Getters and Setters */		
		const std::string & getLogdir( void ) const;
		void				setLogdir( const std::string & path );
		bool				getLogColor( void ) const;
		const char**		getEnv( void ) const;
		void				setEnv( const char **env);

	friend class JobManager;
	friend class AThread;
};

//REVIEW Not implemented for now
// std::ostream &			operator<<( std::ostream & o, Taskmaster const & i );

#endif /* ****************************************************** TASKMASTER_H */