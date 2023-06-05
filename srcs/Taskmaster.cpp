# include "Taskmaster.hpp"

/*
	INIT
*/

bool		Taskmaster::isRunningRootPermissions( void ) const
{
	uid_t euid = geteuid();
	if (euid != 0)
		return false;
	return true;	
}

// void		Taskmaster::initCategories( void ) const
// {
// 	Tintin_reporter::getLogManager("./default.log").addCategory(LOG_CATEGORY_LOGGER);
// 	LOG_INFO(LOG_CATEGORY_DEFAULT, "FIRST WRITE TO DEFAULT")
// 	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_INIT, "init.log");
// 	LOG_INFO(LOG_CATEGORY_DEFAULT, "SECOND WRITE TO DEFAULT")
// 	LOG_INFO(LOG_CATEGORY_INIT, "FIRST WRITE TO INIT")
// 	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_INIT);
// 	LOG_INFO(LOG_CATEGORY_INIT, "INIT NOW point to default")
// 	LOG_INFO("BLABLA", "Unknown to default")
// 	const_cast<Taskmaster*>(this)->exitProperly();
// 	exit(0);
// }

void		Taskmaster::initCategories( void ) const
{
	Tintin_reporter::getLogManager("./default.log").addCategory(LOG_CATEGORY_DEFAULT);
	// Tintin_reporter::getLogManager(LOG_STDOUT_MAGIC).addCategory(LOG_CATEGORY_DEFAULT);
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_LOGGER, LOG_STDOUT_MAGIC);
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_INIT);
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_NETWORK);
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_SIGNAL, "./signal.log");
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_CONFIG);
	Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_JOB, LOG_STDOUT_MAGIC);
}


/* 
	Methods implementation
*/

int			Taskmaster::parse_arguments( void )
{
	return EXIT_SUCCESS;
}



int			Taskmaster::exitProperly( void )
{
	LOG_INFO(LOG_CATEGORY_INIT, "Exiting properly.")
	this->freeLockFile();
	LOG_INFO(LOG_CATEGORY_INIT, "Lock file '" + this->_lockpath + "' successfuly released.")
	LOG_INFO(LOG_CATEGORY_INIT, "Destroying logger.")
	Tintin_reporter::destroyLogManager();
	return EXIT_SUCCESS;
}


/*
	CONFIG
*/
//TODO Clean the code of this function

int			Taskmaster::_parseConfigPrograms( void )
{
	std::list<Job> newJoblist ;
	Job job;
	bool isInvalid = false;

	YAML::Node topnode = this->_config[TM_FIELD_PROGRAMS];
	
	for(YAML::const_iterator it=topnode.begin();it!=topnode.end();++it) {
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
		{
			job.setWorkingdir(it->second[TM_FIELD_WORKINGDIR].as<std::string>());
		}
		else
		{
			// TODO set pwd equal to main pwd
		}

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


		if (it->second[TM_FIELD_STOPSIGNAL])
		{
			//TODO optimize this function
			uint signal = TM_DEF_STOPSIGNAL;
			std::string sigstr = it->second[TM_FIELD_STOPSIGNAL].as<std::string>();
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
						LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_STOPSIGNAL) + "' on job '" + job.getName() + "'. (Out of Range)")
					}
				}
				catch (std::exception & e)
				{
					LOG_WARN(LOG_CATEGORY_CONFIG, "Invalid value for field '" + ntos(TM_FIELD_STOPSIGNAL) + "' on job '" + job.getName() + "'.")
				}
			}
			job.setStopSignal(signal);
		}


//REVIEW add a switch to check at config load if stdout/err file is
		if (it->second[TM_FIELD_STDOUT])
		{
			std::string stdout = it->second[TM_FIELD_STDOUT].as<std::string>();
			// if (stdout == "allow" || stdout == "discard")
			// 	job.setStdout(stdout);
			// else if ()
				job.setStdout(stdout);

		}
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
	if (this->_config[TM_FIELD_SERVER][TM_FIELD_LOGCOLOR])
	{
		this->_logcolor = this->_config[TM_FIELD_SERVER][TM_FIELD_LOGCOLOR].as<bool>();
		Tintin_reporter::getLogManager().setColor(this->_logcolor);
	}
	return EXIT_SUCCESS;
}


int			Taskmaster::loadConfigFile(const std::string & path)
{
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
	if (this->_config.reloadConfigFile())
	{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


/*
	LOCK FILE
*/

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

/*
	UTILS
*/

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
