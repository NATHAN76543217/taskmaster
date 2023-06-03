#ifndef JOB_HPP
# define JOB_HPP

# include <iostream>
# include <string>
# include <map>
// # include <list>
# include <vector>
# include "yaml-cpp/yaml.h"

# include "tm_values.hpp"
# include "Tintin_reporter.hpp"

class Job
{

	public:

		class JobConfigException : std::exception
		{

		};
	
		Job();
		Job( Job const & src );
		~Job();

		Job &		operator=( Job const & rhs );
		
		void			resetDefault( void );

		void			setName(const std::string & name);
		void			setCmd(const std::string & cmd);
		void			setNbProcs(const uint nb_processes);
		void			setUmask(const uint umask);
		void			setWorkingdir(const std::string & dir);
		void			setAutostart(const bool autostart);
		void			setRestartPolicy(const policy policy);
		void			setNbRetry( const uint nbretrymax );
		void			addExitCode(const uint);
		void			setStarttime( const uint starttime );
		void			setStoptime( const uint stoptime );
		void			setStopSignal( const int signal );
		void			setStdout(const std::string & pwd);
		void			setStderr(const std::string & pwd);
		void			addEnv(const std::string & key, const std::string & value);

		const std::string&	getName( void ) const;
		const std::string&	getCmd( void ) const;
		uint				getNbProcs( void ) const;
		uint				getUmask( void ) const;
		const std::string&	getWorkingdir( void ) const;
		bool				getAutostart( void ) const;
		policy				getRestartPolicy( void ) const;
		const char*			getRestartPolicyString( void ) const;
		uint				getNbRetry( void ) const;
		const std::vector<uint>		&getExitCodes( void ) const;
		uint				getStarttime( void ) const;
		uint				getStoptime( void ) const;
		int					getStopSignal( void ) const;
		const std::string&	getStdout( void ) const;
		const std::string&	getStderr( void ) const;
		const std::map<std::string, std::string>&	getEnv( void ) const;

	private:
		std::string			_name;
		std::string 		_cmd;
		uint				_nbprocs;
		uint				_umask;
		std::string			_workingdir;
		bool				_autostart;
		policy				_restartpolicy;
		uint				_nbretrymax;
		std::vector<uint>	_exitcode;
		uint				_starttime;
		uint				_stoptime;
		int					_stopsignal;
		std::string			_stdout;
		std::string			_stderr;
		std::map<std::string, std::string> _env;

		//demander a luca si on fait une architecture avec les jobs qui tournent independament sur des thread ou processus differents et on vient hot-update leur version de la config pour changer leur behavior

		// dataAssesment() on each data modification
		// a status variable 
	friend class Taskmaster;
};


std::ostream &			operator<<( std::ostream & o, Job const & i );

#endif /* ************************************************************* JOB_H */