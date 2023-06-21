#ifndef JOB_HPP
# define JOB_HPP

# include <iostream>
# include <string>
# include <map>
// # include <list>
# include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
# include <cstdlib>
# include <vector>
# include <thread>
# include "yaml-cpp/yaml.h"

class Job;
# include "Taskmaster.hpp"
# include "tm_values.hpp"
# include "Tintin_reporter.hpp"

class Job
{

	public:

		/*
			Should exclude status variable from this check
		*/
		static int			Compare( const Job & j1, const Job & j2);	


		class JobConfigException : std::exception
		{

		};
	
		Job();
		Job( Job const & src );
		~Job();

		Job &		operator=( Job const & rhs );
		

		bool			hasPid( pid_t pid );
		bool			rmPid( pid_t pid );
		int				spawnProcess( void );
		void			resetDefault( void );
		int				start( void );
		int				terminate( void );
		int				resume( void );
		int				suspend( void );
		int				stop( void );
		int				kill( void );
		int				kill_pid( pid_t pid );
		int				gracefullStop( void );

		void			setChildStatus( pid_t pid, child_status status);
		job_status		getStatus( void ) const;
		void			setStatus( job_status status );
		std::vector< std::vector<char> >    splitQuotes(const std::string &str) const;


		void			setName(const std::string & name);
		void			setCmd(const std::string & cmd);
		void			setNbProcs(const uint nb_processes);
		void			setUmask(const uint umask);
		void			setWorkingdir(const std::string & dir);
		void			setAutostart(const bool autostart);
		void			setRestartPolicy(const job_policy policy);
		void			setNbRetry( const uint nbretrymax );
		void			addExitCode(const uint);
		void			setStarttime( const uint starttime );
		void			setStoptime( const uint stoptime );
		void			setStopSignal( const int signal );
		void			setStdout(const std::string & pwd);
		void			setStderr(const std::string & pwd);
		void			setEnvfromparent(const bool pass);
		void			addEnv(const std::string & key, const std::string & value);

		void			_setpid( const std::map<pid_t, child_status> & pid );



		const std::string&	getName( void ) const;
		const std::string&	getCmd( void ) const;
		uint				getNbProcs( void ) const;
		uint				getUmask( void ) const;
		const std::string&	getWorkingdir( void ) const;
		bool				getAutostart( void ) const;
		job_policy			getRestartPolicy( void ) const;
		const char*			getRestartPolicyString( void ) const;
		const char*			getStatusString( job_status status ) const;
		uint				getNbRetry( void ) const;
		const std::vector<uint>		&getExitCodes( void ) const;
		uint				getStarttime( void ) const;
		uint				getStoptime( void ) const;
		int					getStopSignal( void ) const;
		const std::string&	getStdout( void ) const;
		const std::string&	getStderr( void ) const;
		bool				getEnvfromparent( void ) const;
		const std::map<std::string, std::string>&	getEnv( void ) const;

		const std::map<pid_t, child_status>&		_getpids( void ) const;



	private:
		bool				_shouldUpdate;
		job_status			_status;
		bool				_complete;
		std::map<pid_t, child_status>	_pid;

		std::string			_name;
		std::string 		_cmd;
		uint				_nbprocs;
		uint				_umask;
		std::string			_workingdir;
		bool				_autostart;
		job_policy			_restartpolicy;
		uint				_nbretrymax;
		std::vector<uint>	_exitcode;
		uint				_starttime;
		uint				_stoptime;
		int					_stopsignal;
		std::string			_stdout;
		std::string			_stderr;
		bool				_envfromparent;
		std::map<std::string, std::string> _env;


	friend class Taskmaster;
};



#endif /* ************************************************************* JOB_H */