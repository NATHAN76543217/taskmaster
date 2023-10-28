#include "Job.hpp"


std::ostream &			operator<<( std::ostream & o, Job const & i );

//TODO remove all debug stuff
int						Job::Compare( const Job & j1, const Job & j2)	
{
	if (j1.getName().compare(j2.getName()))
		return -1;
	
	if (j1.getCmd().compare(j2.getCmd()))
	{
		std::cout << "CMD" << std::endl;
		return 1;
	}
	if (j1.getNbProcs() != j2.getNbProcs())
	{
		std::cout << "NBPROCS" << std::endl;
		return 1;
	}
	if (j1.getAutostart() != j2.getAutostart())
	{
		std::cout << "AUTOSTART" << std::endl;
		return 1;
	}
	if (j1.getWorkingdir().compare(j2.getWorkingdir()))
	{
		std::cout << "WORKDIR" << std::endl;
		return 1;
	}
	if ( (j1.getAutostart() != j2.getAutostart())
	|| (j1.getRestartPolicy() != j2.getRestartPolicy())
	|| (j1.getNbRetry() != j2.getNbRetry())
	// || (j1.getExitCodes() != j2.getExitCodes())
	|| (j1.getStarttime() != j2.getStarttime())
	|| (j1.getStoptime() != j2.getStoptime())
	|| (j1.getStopSignal() != j2.getStopSignal())
	|| (j1.getStdout().compare(j2.getStdout()))
	|| (j1.getStderr().compare(j2.getStderr()))
	// || (j1.getEnvfromparent() != j2.getEnvfromparent())
	// || (j1.getEnv() != j2.getEnv())
	)
	{
		return 1;
	}
	return 0;
}

// int						Job::Compare( const Job & j1, const Job & j2)	
// {
// 	if (j1.getName().compare(j2.getName()))
// 	{
// 		if ((j1.getCmd().compare(j2.getCmd()))
// 		|| (j1.getNbProcs() != j2.getNbProcs())
// 		|| (j1.getWorkingdir().compare(j2.getWorkingdir()))
// 		|| (j1.getAutostart() != j2.getAutostart())
// 		|| (j1.getRestartPolicy() != j2.getRestartPolicy())
// 		|| (j1.getNbRetry() != j2.getNbRetry())
// 		// || (j1.getExitCodes() != j2.getExitCodes())
// 		|| (j1.getStarttime() != j2.getStarttime())
// 		|| (j1.getStoptime() != j2.getStoptime())
// 		|| (j1.getStopSignal() != j2.getStopSignal())
// 		|| (j1.getStdout().compare(j2.getStdout()))
// 		|| (j1.getStderr().compare(j2.getStderr()))
// 		// || (j1.getEnvfromparent() != j2.getEnvfromparent())
// 		// || (j1.getEnv() != j2.getEnv())
// 		)
// 		{
// 			std::cout << "CMP J1 " << j1 << " and J2 " << j2 << " differs." << std::endl;
// 			return 1;
// 		}
// 		return 0;
// 	}
	
// 	return -1;
// }

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/
Job::Job(): 
_status(not_started),
_complete(true),
_pid(),
_name(""),
_cmd(""),
_nbprocs(TM_DEF_NBPROCS),
_workingdir(TM_DEF_WORKDIR),
_autostart(TM_DEF_AUTOSTART),
_restartpolicy(TM_DEF_RESTARTPOLICY),
_nbretrymax(TM_DEF_NBRETRYMAX),
_exitcode(),
_starttime(TM_DEF_STARTTIME),
_stoptime(TM_DEF_STOPTIME),
_stopsignal(TM_DEF_STOPSIGNAL),
_stdout(TM_DEF_STDOUT),
_stderr(TM_DEF_STDERR),
_envfromparent(TM_DEF_ENVFROMPARENT),
_env()
{
}

Job::Job( const Job & src ) : 
_status(src._status),
_complete(src._complete),
_pid(src._pid),
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
_envfromparent(src._envfromparent),
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
	
	this->_status = rhs._status;
	this->_complete = rhs._complete;
	this->_pid = rhs._pid;
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
	this->_envfromparent = rhs._envfromparent;
	this->_env = rhs._env;
	return *this;
}

std::ostream &			operator<<( std::ostream & o, Job const & i )
{
	o << "Job config:" << std::endl;
	o << " - shouldUpdate : " << i.getName() << std::endl;
	o << " - name : " << i.getName() << std::endl;
	o << " - complete : " << i.getName() << std::endl;
	o << " - status : " << i.getStatusString(i.getStatus()) << std::endl;
	o << " - cmd : " << i.getCmd() << std::endl;
	o << " - nbprocs : " << i.getNbProcs() << std::endl;
	o << " - pid : " << std::endl;
	for (std::map<pid_t, child_status>::const_iterator it = i._getpids().begin() ; it != i._getpids().end() ; it++)
	{
		o << "   - (" << it->first << ")(" << i.getChildStatusString(it->second) << ")"<< std::endl;
	}
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
	o << " - envfromparent : " << i.getEnvfromparent() << std::endl;
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

int				Job::spawnProcess( void )
{
	int pid = fork();
	if (pid < 0)
	{
		LOG_ERROR(LOG_CATEGORY_JOB,  "[" << this->_name << "] Failed to `fork()` : " << strerror(errno))
		return EXIT_FAILURE;
	}
	else if (pid == 0)
	{
		/* child process */

		/* Set umask */
		LOG_DEBUG(LOG_CATEGORY_JOB, "[" << this->_name << "][" << getpid() << "] umask: " << umask(this->getUmask()) << " -> " << this->getUmask()) 
	
		/* Set cwd */
		if (this->_workingdir.empty())
			LOG_CRITICAL(LOG_CATEGORY_JOB, "Job's pwd shouldn't be empty.")
		else
		{
			if (chdir(this->getWorkingdir().c_str()) == -1)
				LOG_ERROR(LOG_CATEGORY_JOB, "[" << this->_name << "][" << getpid() << "] Failed to change working direcotry : " << strerror(errno))
			else
				LOG_DEBUG(LOG_CATEGORY_JOB, "[" << this->_name << "][" << getpid() << "] Set `workingdir` to `" << this->_workingdir << "`.")
		}

		/* Set environnement */
		std::vector<std::vector<char>> env_vector = this->splitQuotes(this->_cmd);

		if (this->_envfromparent == true)
		{
			uint i = 0;
			uint y = 0;
			const char** env_parent = Taskmaster::GetInstance().getEnv();
			std::vector<char> tmpvector;
			while (env_parent[i] != NULL)
			{
				y = 0;
				while(env_parent[i][y] != '\0')
				{
					tmpvector.push_back(env_parent[i][y]);
					y++;
				}
				env_vector.push_back(tmpvector);
				tmpvector.clear();
				i++;
			}
		}
	
	
		std::vector<char *>		env_array = std::vector<char*>();
		for (std::vector<char>& v : env_vector)
		{
			env_array.push_back(v.data());
		}
	
		std::vector<std::vector<char>> arg_vector = this->splitQuotes(this->_cmd);
	
		std::vector<char *>		arg_array = std::vector<char*>();
		for (std::vector<char>& v : arg_vector)
		{
			arg_array.push_back(v.data());
		}
		/* Launch command */
		if (execve(arg_array[0], arg_array.data() ,env_array.data()) == -1)
		{
			LOG_ERROR(LOG_CATEGORY_JOB, "[" << this->_name << "][" << getpid() << "] Failed to `execve` : " << strerror(errno))
		}
		exit(EXIT_FAILURE);
	}
	else
	{
		//Parent
		this->_pid.insert(std::make_pair(pid, child_starting));
		LOG_INFO(LOG_CATEGORY_JOB, "[" << this->_name << "] Process '" << pid << "' started.")
	}
	return EXIT_SUCCESS;
}

int				Job::kill_pid( pid_t pid )
{
	int error = 0;
	int ret = ::kill(pid, SIGKILL);
	if ( ret )
	{
		LOG_ERROR(LOG_CATEGORY_JOB, "Failed to kill pid [" << pid << "] : " << strerror(errno))
	}
	LOG_INFO(LOG_CATEGORY_JOB, "Kill sent to pid [" << pid << "] : " << strerror(errno))

	return (error);
}

int				Job::kill( void )
{
	int ret = 0;
	int error = 0;

	for (std::map<pid_t, child_status>::iterator it = this->_pid.begin(); it != this->_pid.end(); it++)
	{
		if (it->second != child_running)
			continue;
		ret = ::kill(it->first, SIGKILL);
		if ( ret )
		{
			LOG_ERROR(LOG_CATEGORY_JOB, "Failed to kill subprocess [" << it->first << "] : " << strerror(errno))
		}
		LOG_DEBUG(LOG_CATEGORY_JOB, "[" << it->first << "] killed successfuly")
	}
	return (error);
}

int				Job::stop( void )
{
	int		ret = 0;
	bool	error = false;

	for (std::map<pid_t, child_status>::iterator it = this->_pid.begin(); it != this->_pid.end(); it++)
	{
		if (it->second != child_running)
			continue;
		ret = ::kill(it->first, SIGSTOP);
		if ( ret )
		{
			error = true;
			LOG_ERROR(LOG_CATEGORY_JOB, "Failed to stop subprocess [" << it->first << "] : " << strerror(errno))
		}
		LOG_DEBUG(LOG_CATEGORY_JOB, "[" << it->first << "] stopped successfuly")
	}
	return (error);
}

int				Job::terminate( void )
{
	int		ret = 0;
	bool	error = false;

	for (std::map<pid_t, child_status>::iterator it = this->_pid.begin(); it != this->_pid.end(); it++)
	{
		if (it->second != child_running)
			continue;
		ret = ::kill(it->first, SIGTERM);
		if ( ret )
		{
			error = true;
			LOG_ERROR(LOG_CATEGORY_JOB, "Failed to terminate subprocess [" << it->first << "] : " << strerror(errno))
		}
		LOG_DEBUG(LOG_CATEGORY_JOB, "[" << it->first << "] terminated successfuly")
	}
	return (error);
}

int				Job::suspend( void )
{
	int		ret = 0;
	bool	error = false;

	for (std::map<pid_t, child_status>::iterator it = this->_pid.begin(); it != this->_pid.end(); it++)
	{
		if (it->second != child_running)
			continue;
		ret = ::kill(it->first, SIGTSTP);
		if ( ret )
		{
			error = true;
			LOG_ERROR(LOG_CATEGORY_JOB, "Failed to suspend subprocess [" << it->first << "] : " << strerror(errno))
		}
		LOG_DEBUG(LOG_CATEGORY_JOB, "[" << it->first << "] suspended successfuly")
	}
	return (error);
}

//REVIEW resume only stopped childs
int				Job::resume( void )
{
	int		ret = 0;
	bool	error = false;

	for (std::map<pid_t, child_status>::iterator it = this->_pid.begin(); it != this->_pid.end(); it++)
	{
		ret = ::kill(it->first, SIGCONT);
		if ( ret )
		{
			error = true;
			LOG_ERROR(LOG_CATEGORY_JOB, "Failed to wake subprocess [" << it->first << "] : " << strerror(errno))
		}
		LOG_DEBUG(LOG_CATEGORY_JOB, "[" << it->first << "] resumed successfuly")
	}
	return (error);
}


int				Job::start( void )
{
	LOG_DEBUG(LOG_CATEGORY_JOB,  "[" << this->_name << "] start.")
	
	this->_complete = true;
	this->_status = starting;

	for ( uint i = 0; i < this->_nbprocs; i++ )
	{
		if (this->spawnProcess() == EXIT_FAILURE)
		{
			this->_complete = false;
		}
	}
	/* TODO Check restart policy here */
	/* Check if everything is running smoothly */
	if (this->_complete == true)
	{
		this->_status = running;
		LOG_INFO(LOG_CATEGORY_JOB,  "[" << this->_name << "] started successfuly.")
	}
	else
	{
		this->_status = incomplete;
		LOG_WARN(LOG_CATEGORY_JOB,  "[" << this->_name << "] start incompletly.")
	}
	return EXIT_SUCCESS;
}

int				Job::gracefullStop( void )
{
	if (this->terminate() != EXIT_SUCCESS)
		return this->kill();
	std::this_thread::sleep_for(std::chrono::seconds(this->_stoptime));
	//REVIEW here to avoid waiting a long time a process alredy successfuly stopped
	int stat = 0;
	int ret = 0;
	for (std::map<pid_t, child_status >::iterator it = this->_pid.begin(); it != this->_pid.end(); it++)
	{
		ret = waitpid(it->first, &stat, WNOHANG);
		if ( ret == 1)
		{
			LOG_ERROR(LOG_CATEGORY_JOB, "Failed to `waitpid` : " << strerror(errno) )
			continue;
		}
		if (!(WIFEXITED(stat) || WIFSIGNALED(stat)))
		{
			LOG_INFO(LOG_CATEGORY_JOB, "process [" << it->first << "] not exited after a timeout of [" << this->_stoptime << "] seconds. It will be Killed immediatly.")
			this->kill_pid(it->first);
			break;
		}
		ret = 0;
		stat = 0;
		this->setStatus(terminated);
	}
	return EXIT_SUCCESS;
}

// int			Job::refreshStatus()
// {

// }

//TODO set default value here and call this function in constructor? 
//TODO put this method in private
//TODO before restet: catch death of all childs
void			Job::resetDefault( void )
{
	this->_name = "";
	this->_status = not_started; 
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

//BUG fix here
bool		Job::hasPid( pid_t pid )
{
	return (this->_pid.find(pid) != this->_pid.end());
}

bool		Job::rmPid( pid_t pid )
{
	return this->_pid.erase(pid);
}

void						Job::setChildStatus( pid_t pid, child_status status)
{
	std::map<pid_t, child_status>::iterator it = this->_pid.find(pid);
	if (it == this->_pid.end())
	{
		LOG_ERROR(LOG_CATEGORY_JOB, "[" << this->_name << "] pid " << pid << " not found.")
		return ;
	}
	it->second = status ;
	LOG_DEBUG(LOG_CATEGORY_JOB, "Change status of: " << *this)
	return ;
}


/*
** --------------------------------- CHECKERS ---------------------------------
*/

void			Job::setStatus( job_status status )
{
	this->_status = status;
}

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

void			Job::setRestartPolicy(const job_policy policy)
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

void			Job::setEnvfromparent( const bool pass)
{
	this->_envfromparent = pass;
}

void			Job::addEnv(const std::string & key, const std::string & value)
{
	this->_env[key] = value;
}

void			Job::_setpid( const std::map<pid_t, child_status > & pid)
{
	this->_pid = pid;
}

/*
** --------------------------------- GETTERS ---------------------------------
*/

job_status				Job::getStatus( void ) const
{
	return this->_status;
}


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

job_policy					Job::getRestartPolicy( void ) const
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
//TODO remove all ntos() call in all the code
	}
	return "unknown";
}

const char*				Job::getStatusString( job_status status ) const
{
	switch (status)
	{	
		case not_started:
			return "not started";
			break;
		case starting:
			return "starting";
			break;
		case running:
			return "running";
			break;
		case stopped:
			return "stopped";
			break;
		case suspended:
			return "suspended";
			break;
		case incomplete:
			return "incomplete";
			break;
		case exited:
			return "exited";
			break;
		case terminated:
			return "terminated";
			break;
		default:
			LOG_CRITICAL(LOG_CATEGORY_JOB, "Invalid status value '" + ntos(this->_restartpolicy) + "' for job '" + this->_name + "'.") 
//TODO remove all ntos() call in all the code
	}
	return "unknown";
}

const char*				Job::getChildStatusString( child_status status ) const
{
	switch (status)
	{	
		case child_not_started:
			return "not started";
			break;
		case child_starting:
			return "starting";
			break;
		case child_running:
			return "running";
			break;
		case child_stopped:
			return "stopped";
			break;
		case child_suspended:
			return "suspended";
			break;
		case child_terminating:
			return "terminating";
			break;
		case child_exited:
			return "exited";
			break;
		case child_terminated:
			return "terminated";
			break;
		default:
			LOG_CRITICAL(LOG_CATEGORY_JOB, "Invalid status value '" + ntos(this->_restartpolicy) + "' for job '" + this->_name + "'.") 
//TODO remove all ntos() call in all the code
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

bool						Job::getEnvfromparent( void ) const
{
	return this->_envfromparent;
}

const std::map<std::string, std::string>&	Job::getEnv( void ) const
{
	return this->_env;
}



const std::map<pid_t, child_status >&			Job::_getpids( void ) const
{
	return this->_pid;
}



std::vector< std::vector<char> >    Job::splitQuotes(const std::string &str) const
{
    std::vector<std::vector<char> > strlist =  std::vector<std::vector<char> >();

    char quote = -1;
    char escape = -1;
    
    strlist.push_back(std::vector<char>());
    
    for (const char& c : str)
    {
        if ((c == ' ' || c == '\t') && quote == -1)
        {
            if (!strlist.rbegin()->empty())
            {
                strlist.rbegin()->push_back(0);
                strlist.push_back(std::vector<char>());
            }
        }
        else if ((c == '\"' || c == '\'') && (quote == -1 || quote == c) && escape != '\\')
        {
            if (quote == c)
                quote = -1;
            else if (quote == -1)
                quote = c;
        }
        else if (c != '\\' || escape == '\\')
        {
            strlist.rbegin()->push_back(c);
            escape = -1;
        }
        else
            escape = c;
    }
    if (quote != -1)
        throw std::logic_error("Unterminated quote string (no matching ending quote for `" + std::string(&quote, 1) + "`)");

    if (!strlist.rbegin()->empty()) 
        strlist.push_back(std::vector<char>());
    return (strlist);
}

/* ************************************************************************** */