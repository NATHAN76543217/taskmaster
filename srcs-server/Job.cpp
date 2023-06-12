#include "Job.hpp"


/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/
Job::Job(): 
_shouldUpdate(false),
_status(JOB_STATUS_NOTSTARTED),
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
_shouldUpdate(src._shouldUpdate),
_status(src._status),
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
	
	this->_shouldUpdate = rhs._shouldUpdate;
	this->_status = rhs._status;
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
	o << " - status : " << i.getName() << std::endl;
	o << " - pid : " << i.getName() << std::endl;
	o << " - name : " << i.getName() << std::endl;
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
		//child
		// REVIEWTake mutex
		/* Set umask */
		LOG_DEBUG(LOG_CATEGORY_JOB, "[" << this->_name << "][" << getpid() << "] umask: " << umask(this->getUmask()) << " -> " << this->getUmask()) 
	
		/* Set cwd */
		if (!this->_workingdir.empty())
		{
			if (chdir(this->getWorkingdir().c_str()) == -1)
			{
				LOG_ERROR(LOG_CATEGORY_JOB, "[" << this->_name << "][" << getpid() << "] Failed to change working direcotry : " << strerror(errno))
			}
		}
		std::vector<std::vector<char>> env_vector = this->splitQuotes(this->_cmd);

		if (this->_envfromparent == true)
		{
			uint i = 0;
			uint y = 0;
			const char** env_parent = Taskmaster::GetInstance()->getEnv();
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
		this->_pid.push_back(pid);
		LOG_INFO(LOG_CATEGORY_JOB, "[" << this->_name << "] Process '" << pid << "' started.")
	}
	return EXIT_SUCCESS;
}



int				Job::start( void )
{
	uint i = 0;
	
	LOG_DEBUG(LOG_CATEGORY_JOB,  "[" << this->_name << "] start.")
	this->_status = JOB_STATUS_STARTING;
	//REVIEW make a for loop
	while (i < this->_nbprocs )
	{
		if (this->spawnProcess() == EXIT_FAILURE)
		{
			this->_status = JOB_STATUS_INCOMPLETE;
		}
		i++;
	}
	/* Check if everything is running smoothly */
	if (this->_status == JOB_STATUS_STARTING)
	{
		this->_status = JOB_STATUS_RUNNING;
		LOG_INFO(LOG_CATEGORY_JOB,  "[" << this->_name << "] started successfuly.")
	}
	else
		LOG_WARN(LOG_CATEGORY_JOB,  "[" << this->_name << "] start incompletly.")
	return EXIT_SUCCESS;
}

//TODO set default value here and call this function in constructor? 
void			Job::resetDefault( void )
{
	this->_name = "";
	this->_status = JOB_STATUS_NOTSTARTED; 
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

void			Job::setStatus( uint status )
{
	if (status >= JOB_STATUS_RUNNING)
		return ;
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

/*
** --------------------------------- GETTERS ---------------------------------
*/

uint				Job::getStatus( void ) const
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