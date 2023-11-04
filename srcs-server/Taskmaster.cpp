# include "Taskmaster.hpp"

/*
**	INIT
*/

Taskmaster&		Taskmaster::CreateInstance( const std::string & name )
{

	/* Block Signals before starting the SignalCatcher */
	sigset_t set;
	sigfillset(&set);
	if (pthread_sigmask(SIG_SETMASK, &set, NULL))
	{
		std::cout << "Failed to `pthread_sigmask` : " << strerror(errno) ;
	}

	SignalCatcher::GetInstance("SignalCatcher");

	Taskmaster & TM = Taskmaster::GetInstance(name);

	if (TM.loadConfigFile(TM_DEF_CONFIGPATH))
	{
		LOG_CRITICAL(LOG_CATEGORY_INIT, "Failed to load configuration file. Aborting")
		return TM;
	}
	Tintin_reporter& logger = Tintin_reporter::CreateInstance(TM.getLogdir(), "default.log");
	logger.setColor(TM.getLogColor());
#if LOG_CATEGORY_AUTO == false
	TM.initCategories();
	LOG_INFO(LOG_CATEGORY_LOGGER, logger)
# else
	Tintin_reporter::GetInstance("./default.log");
#endif
	JobManager::GetInstance("JobManager");
	return TM;
}



void			Taskmaster::Destroy( void )
{
	JobManager::DestroyInstance();
	Tintin_reporter::DestroyInstance();
	SignalCatcher::DestroyInstance();
	Taskmaster::DestroyInstance();
}



int			Taskmaster::initialization( const char** env )
{
	LOG_INFO(LOG_CATEGORY_INIT, "Wait to initialize taskmaster")
	try
	{
		std::lock_guard<std::mutex> lock(Taskmaster::static_mutex);
		LOG_INFO(LOG_CATEGORY_INIT, "Initialization start.")
		this->setEnv(env);
	}
	catch (std::system_error & e )
	{
		std::cerr << "catch sys error init taskmaster" << std::endl;
	}

	LOG_INFO(LOG_CATEGORY_INIT, "PID: " << ::getpid())

	/* Init thread name array */
	Tintin_reporter::GetInstance().addThreadName(std::this_thread::get_id(), THREADTAG_MAIN);
	Tintin_reporter::GetInstance().addThreadName(SignalCatcher::GetInstance().getThreadID(), THREADTAG_SIGNALCATCHER);
	Tintin_reporter::GetInstance().addThreadName(Tintin_reporter::GetInstance().getThreadID(), THREADTAG_LOGGER);
	Tintin_reporter::GetInstance().addThreadName(Taskmaster::GetInstance().getThreadID(), THREADTAG_TASKMASTER);
	Tintin_reporter::GetInstance().addThreadName(JobManager::GetInstance().getThreadID(), THREADTAG_JOBMANAGER);
	
	LOG_INFO(LOG_CATEGORY_INIT, "Initialization done.")

	return EXIT_SUCCESS;
}



//TODO rename in init logger
/*
	Tintin_reporter must be already init
*/
void		Taskmaster::initCategories( void ) const
{
	// Tintin_reporter::getLogManager(LOG_STDOUT_MAGIC).addCategory(LOG_CATEGORY_DEFAULT);
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_DEFAULT);
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_LOGGER, LOG_STDOUT_MAGIC);
	// Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_LOGGER);
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_INIT);
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_MAIN);
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_NETWORK);
	// Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_SIGNAL, "./signal.log");
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_SIGNAL);
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_CONFIG);
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_JOB);
	Tintin_reporter::GetInstance().addCategory(LOG_CATEGORY_JM);
}


/* 
	Methods implementation
*/

void		Taskmaster::startAll( void )
{
	std::cout << "BEFORE" << std::endl;

	Tintin_reporter::GetInstance().start();
	SignalCatcher::GetInstance().start();
	JobManager::GetInstance().start();
	this->start();
	std::cout << "AFTER" << std::endl;

}



/*
	CONFIG
*/
//TODO Clean the code of this function

int			Taskmaster::_parseConfigPrograms( void )
{
	Job				job;
	bool			isInvalid = false;
	std::list<Job>	newJoblist;

	YAML::Node topnode = this->_config[TM_FIELD_PROGRAMS];
	
	for(YAML::const_iterator it = topnode.begin(); it != topnode.end(); ++it)
	{
		isInvalid = false;
		job.resetDefault();

		job.setName(ntos(it->first));

		if (it->second[TM_FIELD_CMD])
			job.setCmd(it->second[TM_FIELD_CMD].as<std::string>());
		else
		{
			//miss mendatory value
			isInvalid = true;
			LOG_ERROR(LOG_CATEGORY_CONFIG, "Missing mandatory field '" + ntos(TM_FIELD_CMD) + "' on job '" + job.getName() + "'.")
		}


		try {
			if (it->second[TM_FIELD_UMASK])
				job.setUmask(it->second[TM_FIELD_UMASK].as<uint>());
		} catch (YAML::TypedBadConversion<uint> & e)
		{
			LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_UMASK) + "' on job '" + job.getName() + "'.")
		}

		if (it->second[TM_FIELD_WORKINGDIR])
			job.setWorkingdir(it->second[TM_FIELD_WORKINGDIR].as<std::string>());
		else
			job.setWorkingdir(this->_workingdir);

		try {
			if (it->second[TM_FIELD_NBPROCS])
				job.setNbProcs(it->second[TM_FIELD_NBPROCS].as<uint>());
		} catch (YAML::TypedBadConversion<uint> & e)
		{
			LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '"  + ntos(TM_FIELD_NBPROCS) + "' on job '" + job.getName() + "'.")
		}


		try {
			if (it->second[TM_FIELD_AUTOSTART])
				job.setAutostart(it->second[TM_FIELD_AUTOSTART].as<bool>());
		} catch (YAML::TypedBadConversion<bool> & e)
		{
			LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_AUTOSTART) + "' on job '" + job.getName() + "'.")
		}


		if (it->second[TM_FIELD_RESTARTPOLICY])
		{
			std::string restartpolicy = it->second[TM_FIELD_RESTARTPOLICY].as<std::string>();
			if (restartpolicy == "always")
				job.setRestartPolicy(always);
			else if (restartpolicy == "never")
				job.setRestartPolicy(never);
			else if (restartpolicy == "onfailure")
				job.setRestartPolicy(onfailure);
			else
			{
				LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_RESTARTPOLICY) + "' on job '" + job.getName() + "'.")
				job.setRestartPolicy(TM_DEF_RESTARTPOLICY);	
			}
		}


		if (it->second[TM_FIELD_NBRETRYMAX])
		{
			try {
				if (it->second[TM_FIELD_NBRETRYMAX])
					job.setNbRetry(it->second[TM_FIELD_NBRETRYMAX].as<uint>());
			} catch (YAML::TypedBadConversion<uint> & e)
			{
				LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_NBRETRYMAX) + "' on job '" + job.getName() + "'.")
			}
		}


		// if (it->second["exitcodes"])
		// {
		// 	try {
		// 		if (it->second["exitcodes"].IsScalar())
		// 		{
		// 			job.addExitCode(it->second["exitcodes"].as<uint>());
		// 		}
		// 		else if (it->second["exitcodes"].IsSequence())
		// 		{
		// 			for (YAML::const_iterator ecode  = it->second["exitcodes"].begin(); ecode != it->second["exitcodes"].end(); ecode++)
		// 			{
		// 				if ((*ecode).IsScalar())
		// 					job.addExitCode((*ecode).as<uint>());
		// 				else
		// 					LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field 'exitcodes' on job '" + job.getName() + "'. (Not a scalar)")
		// 			}
		// 		}
		// 		else
		// 			LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field 'exitcodes' on job '" + job.getName() + "'. (Not a sequence nor a Scalar)")
		// 	} catch (YAML::TypedBadConversion<uint> & e)
		// 	{
		// 		LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field 'exitcodes' on job '" + job.getName() + "'. (Invalid number)")
		// 	}
		// }

		if (it->second[TM_FIELD_EXITCODES])
		{
				if (it->second[TM_FIELD_EXITCODES].IsScalar())
				{
					try {
						job.addExitCode(it->second[TM_FIELD_EXITCODES].as<uint>());
					} catch (YAML::TypedBadConversion<uint> & e)
					{
						LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_EXITCODES) + "' on job '" + job.getName() + "'. (Invalid number)")
					}
				}
				else if (it->second[TM_FIELD_EXITCODES].IsSequence())
				{
					for (YAML::const_iterator ecode  = it->second[TM_FIELD_EXITCODES].begin(); ecode != it->second[TM_FIELD_EXITCODES].end(); ecode++)
					{
						try {
							if ((*ecode).IsScalar())
								job.addExitCode((*ecode).as<uint>());
							else
								LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_EXITCODES) + "' on job '" + job.getName() + "'. (Not a scalar)")
						} catch (YAML::TypedBadConversion<uint> & e)
						{
							LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_EXITCODES) + "' on job '" + job.getName() + "'. (Invalid number)")
						}
					}
				}
				else
					LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_EXITCODES) + "' on job '" + job.getName() + "'. (Not a sequence nor a Scalar)")
		}

		
		if (it->second[TM_FIELD_STARTTIME])
		{
			try {
				if (it->second[TM_FIELD_STARTTIME])
					job.setStarttime(it->second[TM_FIELD_STARTTIME].as<uint>());
			} catch (YAML::TypedBadConversion<uint> & e)
			{
				LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_STARTTIME) + "' on job '" + job.getName() + "'.")
			}
		}


		if (it->second[TM_FIELD_STOPTIME])
		{
			try {
				if (it->second[TM_FIELD_STOPTIME])
					job.setStoptime(it->second[TM_FIELD_STOPTIME].as<uint>());
			} catch (YAML::TypedBadConversion<uint> & e)
			{
				LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_STOPTIME) + "' on job '" + job.getName() + "'.")
			}
		}


		job.setStopSignal(Taskmaster::getFieldStopSignal(it));
		job.setStdout(Taskmaster::getFieldStdout(it));

		if (it->second[TM_FIELD_STDERR])
			job.setStderr(it->second[TM_FIELD_STDERR].as<std::string>());

		if (it->second[TM_FIELD_ENV])
		{
			if (it->second[TM_FIELD_ENV].IsMap())
			{
				for (YAML::const_iterator env  = it->second[TM_FIELD_ENV].begin(); env != it->second[TM_FIELD_ENV].end(); env++)
				{
					job.addEnv(env->first.as<std::string>(), env->second.as<std::string>());
				}	
			}
			else
				LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid field for key '" + ntos(TM_FIELD_ENV) + "' on job '" + job.getName() + "'.")


		}


		if (isInvalid)
		{
			LOG_WARN(LOG_CATEGORY_CONFIG, "Job config '" + ntos(it->first) + "' skipped.")
			continue;
		}
		/* Successfuly loaded */
		LOG_INFO(LOG_CATEGORY_CONFIG, "New job '" + ntos(it->first) + "' loaded.")
		newJoblist.push_back(job);
	}
	
	this->_joblist = newJoblist;
	return EXIT_SUCCESS;
}

int			Taskmaster::_parseConfigServer( void )
{
	if (this->_config[TM_FIELD_SERVER][TM_FIELD_WORKINGDIR])
		this->setWorkingdir(this->_config[TM_FIELD_SERVER][TM_FIELD_WORKINGDIR].as<std::string>());


	if (this->_config[TM_FIELD_SERVER][TM_FIELD_LOGCOLOR])
		this->_logcolor = this->_config[TM_FIELD_SERVER][TM_FIELD_LOGCOLOR].as<bool>();
	else
		this->_logcolor = TM_DEF_LOGCOLOR;


	if (this->_config[TM_FIELD_SERVER][TM_FIELD_LOGDIR])
		this->setLogdir(this->_config[TM_FIELD_SERVER][TM_FIELD_LOGDIR].as<std::string>());
	else
		this->setLogdir(TM_DEF_LOGDIR);

	return EXIT_SUCCESS;
}



bool				Taskmaster::getLogColor( void ) const
{
	return this->_logcolor;
}


const char**		Taskmaster::getEnv( void ) const
{
	return this->_parentEnv.load();
}

void		Taskmaster::setEnv( const char ** env)
{
	this->_parentEnv.store(env);
}


void		Taskmaster::setWorkingdir( const std::string & path)
{
	if (chdir(path.c_str()))
	{
		LOG_ERROR(LOG_CATEGORY_CONFIG, "Failed to `chdir` : " << strerror(errno))
		LOG_ERROR(LOG_CATEGORY_CONFIG, "Current directory stay at " << this->_workingdir)
		return ;
	}
	LOG_INFO(LOG_CATEGORY_CONFIG, "Workingdir set to `" << path.c_str() << "`.")
	this->_workingdir = path;
	std::cout << ">> " << "PATH = " << path << std::endl;
}


const std::string&		Taskmaster::getWorkingdir( void ) const
{
	return this->_workingdir;
}


int			Taskmaster::loadConfigFile(const std::string & path)
{
	std::lock_guard<std::mutex> lock(Taskmaster::static_mutex);
	
	try {

		if (this->_config.loadConfigFile(path) == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}
	} catch (YAML::ParserException & e)
	{
		LOG_CRITICAL(LOG_CATEGORY_CONFIG, "Failed to parse config file cause: " + ntos(e.what()))
		return EXIT_FAILURE;
	}
	
	if (this->_config[TM_FIELD_SERVER])
	{
		if (this->_parseConfigServer() == EXIT_FAILURE)
			return EXIT_FAILURE;
	}

	if (this->_config[TM_FIELD_PROGRAMS])
	{
		if (this->_parseConfigPrograms() == EXIT_FAILURE)
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int			Taskmaster::reloadConfigFile( void )
{
	std::lock_guard<std::mutex> lock(Taskmaster::static_mutex);

	if (this->_config.reloadConfigFile())
	{
		return EXIT_FAILURE;
	}
	JobManager::GetInstance().setConfigChanged();
	return EXIT_SUCCESS;
}





/*
** --------------------------------- SERVER ---------------------------------
*/

void		Taskmaster::operator()( void )
{
	std::this_thread::sleep_for(std::chrono::microseconds(10000));
	LOG_INFO(LOG_CATEGORY_DEFAULT, " wait to start")
	try
	{
		std::unique_lock<std::mutex> lock(Taskmaster::static_mutex);
		this->_ready.wait(lock);
	}
	catch( std::system_error & e)
	{
		std::cerr << "catch tasjmaster loop start" << std::endl; 
	}
	LOG_INFO(LOG_CATEGORY_DEFAULT, "Start")

	while ( true )
	{
		LOG_DEBUG(LOG_CATEGORY_DEFAULT, "TM/Server thread - loop");

		std::this_thread::sleep_for(std::chrono::seconds(4));
		
		{
			std::lock_guard<std::mutex> lk(Taskmaster::static_mutex);
			if (!this->_running)
			{
				break ;
			}
		}
	}
	LOG_INFO(LOG_CATEGORY_DEFAULT, "End")
	return ;
}

/*
** --------------------------------- GETTERS AND SETTERS ---------------------------------
*/

const std::string&			Taskmaster::getLogdir( void ) const
{
	return this->_logdir;
}

void						Taskmaster::setLogdir( const std::string & path )
{
	if (path.back() != '/')
		this->_logdir = path + '/';
	else
		this->_logdir = path;
}

// For config

//REVIEW add a switch to check at config load if stdout/err file is
const std::string		Taskmaster::getFieldStdout(YAML::const_iterator & it)
{
	if (it->second[TM_FIELD_STDOUT])
		return TM_FIELD_STDOUT;
	std::string stream = it->second[TM_FIELD_STDOUT].as<std::string>();
	return stream;	
	// if (stdout == "allow" || stdout == "discard")
	// 	job.setStdout(stdout);
	// else if ()
}

uint					Taskmaster::getFieldStopSignal(YAML::const_iterator& it)
{
	if (!(it->second[TM_FIELD_STOPSIGNAL]))
		return TM_DEF_STOPSIGNAL;
	
	//TODO optimize this function
	uint		signal = TM_DEF_STOPSIGNAL;
	std::string	sigstr = it->second[TM_FIELD_STOPSIGNAL].as<std::string>();
	if (sigstr == "SIGHUP")
		signal = 1;
	else if (sigstr == "SIGINT")
		signal = 2;
	else if (sigstr == "SIGUSR1")
		signal = 30;
	else if (sigstr == "SIGUSR2")
		signal = 31;
	else
	{
		try {
			signal = std::stoi(sigstr);
			if (signal > 31 || signal < 1)
			{
				signal = TM_DEF_STOPSIGNAL;
				LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value '" << signal << "' for field '" TM_FIELD_STOPSIGNAL "' on job '" << it->first << "'. (Out of Range)")
			}
		}
		catch (std::exception & e)
		{
			LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" TM_FIELD_STOPSIGNAL "' on job '" << it->first << "'.")
		}
	}
	return signal;
}