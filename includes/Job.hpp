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
# include "yaml-cpp/yaml.h"

class Job;
# include "Taskmaster.hpp"
# include "tm_values.hpp"
# include "Tintin_reporter.hpp"

# define JOB_STATUS_NOTSTARTED	0
# define JOB_STATUS_STOPED		1
# define JOB_STATUS_STARTING	2
# define JOB_STATUS_INCOMPLETE	3
# define JOB_STATUS_RUNNING		10
/* If you add a JOB_STATUS define */
/* Update Job::setSatus function with the highest JOB_STATUS define */

class Job
{

	public:

		static int			Compare( const Job & j1, const Job & j2)	
		{
			if (j1.getName() != j2.getName())
			{
				if ((j1.getCmd() != j2.getCmd())
				|| (j1.getStatus() != j2.getStatus())
				|| (j1.getCmd() != j2.getCmd())
				|| (j1.getNbProcs() != j2.getNbProcs())
				|| (j1.getWorkingdir() != j2.getWorkingdir())
				|| (j1.getAutostart() != j2.getAutostart())
				|| (j1.getRestartPolicy() != j2.getRestartPolicy())
				|| (j1.getNbRetry() != j2.getNbRetry())
				|| (j1.getExitCodes() != j2.getExitCodes())
				|| (j1.getStarttime() != j2.getStarttime())
				|| (j1.getStoptime() != j2.getStoptime())
				|| (j1.getStopSignal() != j2.getStopSignal())
				|| (j1.getStdout() != j2.getStdout())
				|| (j1.getStderr() != j2.getStderr())
				|| (j1.getEnvfromparent() != j2.getEnvfromparent())
				|| (j1.getEnv() != j2.getEnv())
				)
					return 1;
				return 0;
			}
			
			return -1;
		}

		class JobConfigException : std::exception
		{

		};
	
		Job();
		Job( Job const & src );
		~Job();

		Job &		operator=( Job const & rhs );
		
		int				spawnProcess( void );
		void			resetDefault( void );
		int				start( void );
		uint			getStatus( void ) const;
		void			setStatus( uint status );
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

		const std::string&	getName( void ) const;
		const std::string&	getCmd( void ) const;
		uint				getNbProcs( void ) const;
		uint				getUmask( void ) const;
		const std::string&	getWorkingdir( void ) const;
		bool				getAutostart( void ) const;
		job_policy				getRestartPolicy( void ) const;
		const char*			getRestartPolicyString( void ) const;
		uint				getNbRetry( void ) const;
		const std::vector<uint>		&getExitCodes( void ) const;
		uint				getStarttime( void ) const;
		uint				getStoptime( void ) const;
		int					getStopSignal( void ) const;
		const std::string&	getStdout( void ) const;
		const std::string&	getStderr( void ) const;
		bool				getEnvfromparent( void ) const;
		const std::map<std::string, std::string>&	getEnv( void ) const;

	private:
		bool				_shouldUpdate;
		uint				_status;
		std::vector<int>	_pid;

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

		//demander a luca si on fait une architecture avec les jobs qui tournent independament sur des thread ou processus differents et on vient hot-update leur version de la config pour changer leur behavior

		// dataAssesment() on each data modification
		// a status variable 
	friend class Taskmaster;
};


std::ostream &			operator<<( std::ostream & o, Job const & i );

#endif /* ************************************************************* JOB_H */