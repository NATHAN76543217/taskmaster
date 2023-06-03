#include "Job.hpp"


/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/
//TODO default values are all defines
Job::Job(): 
_name(""), 
_cmd(""),
_nbprocs(TM_DEF_NBPROCS),
_workingdir(""),
_autostart(TM_DEF_AUTOSTART),
_restartpolicy(TM_DEF_RESTARTPOLICY),
_nbretrymax(TM_DEF_NBRETRYMAX),
_exitcode(),
_starttime(TM_DEF_STARTTIME),
_stoptime(TM_DEF_STOPTIME),
_stopsignal(TM_DEF_STOPSIGNAL),
_stdout(TM_DEF_STDOUT),
_stderr(TM_DEF_STDERR),
_env()
{
}

Job::Job( const Job & src ) : 
_name(src._name), 
_cmd(src._cmd),
_nbprocs(src._nbprocs),
_workingdir(src._workingdir),
_autostart(src._autostart),
_restartpolicy(src._restartpolicy),
_nbretrymax(src._nbretrymax),
_exitcode(src._exitcode),
_starttime(src._starttime),
_stoptime(src._stoptime),
_stopsignal(src._stopsignal),
_stdout(src._stdout),
_stderr(src._stderr),
_env(src._env)
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Job::~Job()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Job &				Job::operator=( Job const & rhs )
{
	if ( this == &rhs )
		return *this;
	
	this->_name = rhs._name;
	this->_cmd = rhs._cmd;
	this->_nbprocs = rhs._nbprocs;
	this->_workingdir = rhs._workingdir;
	this->_autostart = rhs._autostart;
	this->_restartpolicy = rhs._restartpolicy;
	this->_nbretrymax = rhs._nbretrymax;
	this->_exitcode = rhs._exitcode;
	this->_starttime = rhs._starttime;
	this->_stoptime = rhs._stoptime;
	this->_stopsignal = rhs._stopsignal;
	this->_stdout = rhs._stdout;
	this->_stderr = rhs._stderr;
	this->_env = rhs._env;
	return *this;
}

std::ostream &			operator<<( std::ostream & o, Job const & i )
{
	o << "Job '" << i.getName() << ":" << std::endl;
	o << " - cmd : " << i.getCmd() << std::endl;
	o << " - nbprocs : " << i.getNbProcs() << std::endl;
	o << " - umask : " << i.getUmask() << std::endl;
	o << " - workingdir : " << i.getWorkingdir() << std::endl;
	o << " - autostart : " << i.getAutostart() << std::endl;
	o << " - restartpolicy : " << i.getRestartPolicyString() << std::endl;
	o << " - nbretrymax : " << i.getNbRetry() << std::endl;
	std::string exitcodelist = "[";
	for (std::vector<uint>::const_iterator ecode = i.getExitCodes().begin(); ecode != i.getExitCodes().end(); ecode++)
	{
		exitcodelist += ntos(*ecode);
		if (ecode + 1 != i.getExitCodes().end())
		{
			exitcodelist += ", ";
		}
	}
	exitcodelist.append("]");
	o << " - exitcode : " << exitcodelist << std::endl;
	o << " - starttime : " << i.getStarttime() << std::endl;
	o << " - stoptime : " << i.getStoptime() << std::endl;
	o << " - stopsignal : " << i.getStopSignal() << std::endl;
	o << " - stdout : " << i.getStdout() << std::endl;
	o << " - stderr : " << i.getStderr() << std::endl;
	o << " - env : " << std::endl;
	for (std::map<std::string, std::string>::const_iterator env = i.getEnv().begin(); env != i.getEnv().end(); env++)
	{
		o << "    - " << env->first << " : " << env->second << std::endl;
	}
	return o;
}


/*
** --------------------------------- METHODS ----------------------------------
*/
//TODO set default value here and call this function in constructor? 
void			Job::resetDefault( void )
{
	this->_name = ""; 
	this->_cmd = "";
	this->_nbprocs = TM_DEF_NBPROCS;
	this->_workingdir = "";
	this->_autostart = TM_DEF_AUTOSTART;
	this->_restartpolicy = TM_DEF_RESTARTPOLICY;
	this->_nbretrymax = TM_DEF_NBRETRYMAX;
	this->_exitcode.clear();
	this->_starttime = TM_DEF_STARTTIME;
	this->_stoptime = TM_DEF_STOPTIME;
	this->_stopsignal = TM_DEF_STOPSIGNAL;
	this->_stdout = TM_DEF_STDOUT;
	this->_stderr = TM_DEF_STDERR;
	this->_env.clear();
}

/*
** --------------------------------- CHECKERS ---------------------------------
*/

void			Job::setName( const std::string &name)
{
	this->_name = name;
}

void			Job::setCmd( const std::string &cmd)
{
	this->_cmd = cmd;
}

void			Job::setNbProcs(const uint nbprocs)
{
	//REVIEW Do we enable a warning on 0 subprocess or do we coonsider this normal
	// if (nbprocs == 0)
	// {
	// 	LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field 'nbprocs'.")
	// 	this->_nbprocs = TM_DEF_NBPROCS;
	// 	return EXIT_FAILURE;
	// }
	this->_nbprocs = nbprocs;
}

void			Job::setUmask(const uint umask)
{
	this->_umask = umask;
}

void			Job::setWorkingdir(const std::string & dir)
{
	this->_workingdir = dir;
}

void			Job::setAutostart(const bool autostart)
{
	this->_autostart = autostart;
}

void			Job::setRestartPolicy(const policy policy)
{
	this->_restartpolicy = policy;
}

void			Job::setNbRetry( const uint nbretrymax ) 
{
	this->_nbretrymax = nbretrymax;
}

void			Job::addExitCode(const uint code)
{
	this->_exitcode.push_back(code);
}

void			Job::setStarttime( const uint starttime )
{
	this->_starttime = starttime;
}

void			Job::setStoptime( const uint stoptime )
{
	this->_stoptime = stoptime;
}

void			Job::setStopSignal( const int signal )
{
	this->_stopsignal = signal;
}

void			Job::setStdout( const std::string &stdout_value)
{
	// if (stdout_value == "allow")
	// 	this->_stdout = stdout_value;
	// else if (stdout_value == "discard")
	// 	this->_stdout = stdout_value;
	this->_stdout = stdout_value;

}

void			Job::setStderr( const std::string &stderr_value)
{
	this->_stderr = stderr_value;
}

void			Job::addEnv(const std::string & key, const std::string & value)
{
	this->_env[key] = value;
}

/*
** --------------------------------- GETTERS ---------------------------------
*/

const std::string&		Job::getName( void ) const
{
	return this->_name;
}

const std::string&		Job::getCmd( void ) const
{
	return this->_cmd;
}

uint					Job::getNbProcs( void ) const
{
	return this->_nbprocs;
}

uint					Job::getUmask( void ) const
{
	return this->_umask;
}

const std::string&		Job::getWorkingdir( void ) const
{
	return this->_workingdir;
}

bool					Job::getAutostart( void ) const
{
	return this->_autostart;
}

policy					Job::getRestartPolicy( void ) const
{
	return this->_restartpolicy;
}

const char*				Job::getRestartPolicyString( void ) const
{
	switch (this->_restartpolicy)
	{	
		case always:
			return "always";
			break;
		case never:
			return "never";
			break;
		case onfailure:
			return "onfailure";
			break;
		default:
			LOG_CRITICAL(LOG_CATEGORY_JOB, "Invalid policy value '" + ntos(this->_restartpolicy) + "' for job '" + this->_name + "'.") 
	}
	return "unknown";
}

uint					Job::getNbRetry( void ) const
{
	return this->_nbretrymax;
}

//DONE when using the _exitcode for the first time if its empty, populate it with default value
const std::vector<uint>		&Job::getExitCodes( void ) const
{
	if (this->_exitcode.empty())
		const_cast<Job*>(this)->_exitcode.push_back(TM_DEF_EXITCODE);
	return this->_exitcode;
}

uint				Job::getStarttime( void ) const
{
	return this->_starttime;
}

uint				Job::getStoptime( void ) const
{
	return this->_stoptime;
}

int					Job::getStopSignal( void ) const
{
	return this->_stopsignal;
}

const std::string&			Job::getStdout( void ) const
{
	// if (stdout_value == "allow")
	// 	this->_stdout = stdout_value;
	// else if (stdout_value == "discard")
	// 	this->_stdout = stdout_value;
		return this->_stdout;

}

const std::string&			Job::getStderr( void ) const
{
	return this->_stderr;
}

const std::map<std::string, std::string>&	Job::getEnv( void ) const
{
	return this->_env;
}

/* ************************************************************************** */