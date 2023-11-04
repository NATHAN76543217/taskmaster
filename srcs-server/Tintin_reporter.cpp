#include "Tintin_reporter.hpp"
//TODO Improve verbose: say 'new' when new category and say 'set' when relink category (do the same for files)

/*
** ------------------------------- Static VAR init --------------------------------
*/

std::mutex									Tintin_reporter::_mutexQueue;
std::condition_variable						Tintin_reporter::_hasUpdate;
std::queue<Tintin_reporter::log_message>	Tintin_reporter::_messageQueue = std::queue<Tintin_reporter::log_message>();

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Tintin_reporter::Tintin_reporter(const std::string &defaultFile) : 
																AThread<Tintin_reporter>(*this, "Tintin_reporter"),
																_defaultCategoryName(LOG_CATEGORY_DEFAULT),
																_timestamp_format(LOG_TIMESTAMP_CURRENT),
																_starttime(),
																_color_enabled(false),
																_mutexUpdate()
{
	this->addCategory(this->_defaultCategoryName, formatFilename(defaultFile));
	if (this->_categories.empty())
		throw Tintin_reporter::defaultFileException(defaultFile);
	this->_starttime = Tintin_reporter::getTimestamp();
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Tintin_reporter::~Tintin_reporter()
{
	std::unique_lock<std::mutex> lock(Tintin_reporter::static_mutex);

	for (std::map<std::string, Tintin_reporter::log_destination>::iterator it = this->_opened_files.begin();
		 it != this->_opened_files.end();
		 it++)
	{
		if (this->_categories.at(this->_defaultCategoryName).filename == it->first)
			continue;
		if (it->second.is_file == true)
		{
			// LOG_DEBUG(LOG_CATEGORY_LOGGER, "Closing file '" + it->first + "'.")
			static_cast<std::ofstream &>(it->second.output).close();
			delete &(it->second.output);
		}
	}

	Tintin_reporter::log_destination &default_destination = this->_opened_files.at(this->_categories.at(this->_defaultCategoryName).filename);
	if (default_destination.is_file == true)
	{
		// LOG_DEBUG(LOG_CATEGORY_LOGGER, "Closing file '" + this->_categories.at(this->_defaultCategoryName).filename + "'.")
		static_cast<std::ofstream &>(default_destination.output).close();
		delete &(default_destination.output);
	}
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

void				Tintin_reporter::operator()( void )
{
	std::this_thread::sleep_for(std::chrono::microseconds(10000));
	LOG_INFO(LOG_CATEGORY_LOGGER, "wait to start");
	{
		std::unique_lock<std::mutex> lock(Tintin_reporter::static_mutex);
		this->_ready.wait(lock);
	}

	LOG_INFO(LOG_CATEGORY_LOGGER, "Start.");

	std::unique_lock<std::mutex> update_lock(this->_mutexUpdate);

	do {
		{
			std::lock_guard<std::mutex> intern_lock(Tintin_reporter::static_mutex);
			{
				std::lock_guard<std::mutex> queue_lock(Tintin_reporter::_mutexQueue);

				while (!Tintin_reporter::_messageQueue.empty())
				{
					this->_log(this->_messageQueue.front());
					Tintin_reporter::_messageQueue.pop();
				}
			}
			if (this->_running == false)
			{
				this->_log(log_message(LOG_LEVEL_INFO, LOG_CATEGORY_LOGGER, std::this_thread::get_id(), Tintin_reporter::getTimestamp(), "End."));
				break;
			}
		}
		Tintin_reporter::_hasUpdate.wait(update_lock);
	}
	while (true);
	return ;
}


std::ostream &operator<<(std::ostream &o, Tintin_reporter const &i)
{
	o << " --- LOGGER --- " << std::endl;
	o << "color: " << std::boolalpha << i.getColor() << " | Categories: " << i.getNbCategories() << " | Files: " << i.getNbOpenfiles() << std::endl;
	o << "> Categories: (def: " << i.getDefaultCategory() << ")" << std::endl;
	for (std::map<std::string, Tintin_reporter::log_category>::const_iterator it = i.getCategoriesBegin(); it != i.getCategoriesEnd(); it++)
	{
		// o << "- " << std::setw(LOG_CATEGORY_NAME_MAXSIZE) << it->first << " to '" << it->second.filename << "'" << std::endl;
		o << " - " << std::setw(LOG_CATEGORY_NAME_MAXSIZE) << it->second.name << " to '" << it->second.filename << "'" << std::endl;
	}
	o << "> Opened files: " << std::endl;
	for (std::map<std::string, Tintin_reporter::log_destination>::const_iterator it = i.getOpenFilesBegin(); it != i.getOpenFilesEnd(); it++)
	{
		if (it->second.is_file)
			o << " - isFile: true  State: " << !it->second.output.fail() << " CatLinked: " << std::left << std::setw(3) << it->second.nb_references << " - '" << it->first << "'" << std::endl;
		else
			o << " - isFile: false State: " << !it->second.output.fail() << " CatLinked: " << std::left << std::setw(3) << it->second.nb_references << " - '" << it->first << "'" << std::endl;
	}
	return o;
}


/*
** --------------------------------- METHODS ----------------------------------
*/


void				Tintin_reporter::stop( void ) 
{
	AThread<Tintin_reporter>::stop();
	Tintin_reporter::_hasUpdate.notify_all();
	return ;
}

void				Tintin_reporter::addThreadName(const std::thread::id id, const std::string & name)
{
	this->_threadnames.insert(make_pair(id,name));
}


/*
	WARNING: This function allocate the field .output with `new`.
*/
Tintin_reporter::log_destination Tintin_reporter::newLogDestination(void) const
{
	Tintin_reporter::log_destination dst(true, 1, static_cast<std::ostream &>(*(new std::ofstream)));

	return dst;
}

Tintin_reporter::log_destination Tintin_reporter::newLogDestinationStdout(void) const
{
	return Tintin_reporter::log_destination(false, 1, std::cout);
}

Tintin_reporter::log_destination Tintin_reporter::newLogDestinationStderr(void) const
{
	return Tintin_reporter::log_destination(false, 1, std::cerr);
}

/*
	TODO: remove leading ./ and replace by ether cwd or logdir

*/
std::string Tintin_reporter::formatFilename(const std::string & filename) const
{
	if (filename.empty() || filename == LOG_STDOUT_MAGIC)
		return LOG_STDOUT_MAGIC;
	else if (filename == LOG_STDERR_MAGIC)
		return LOG_STDERR_MAGIC;

	std::string new_filename = filename;
	if (filename.at(0) != '/')
	{
		new_filename = this->getLogdir() + filename;
	}

	/*
		Remove /./ from path
	*/
	size_t pos = 0;
	while (( pos = new_filename.find("/./")) != std::string::npos)
	{
		new_filename.erase(new_filename.begin() + pos, new_filename.begin() + pos + 2);
	}


	// char pwd[PATH_MAX];
	// getcwd(pwd);
	// std::cout << "!!" << filename << " | " << path << std::endl;
	return std::string(new_filename);
	// else if (filename.at(0) == '.')
	// {
	// 	if (filename.at(1) == '.')
	// 	{
	// 	}
	// }
}


//TODO check if still needed after LOG_CATEGORY_NAME_MAXSIZE moved to _log
std::string Tintin_reporter::formatCategoryName(const std::string &categoryName) const
{
	std::string formatedName(categoryName);

	if (formatedName.empty())
		return LOG_CATEGORY_DEFAULT;
	if (categoryName.size() > LOG_CATEGORY_NAME_MAXSIZE)
		formatedName = categoryName.substr(0, LOG_CATEGORY_NAME_MAXSIZE);
	else
		formatedName.append(LOG_CATEGORY_NAME_MAXSIZE - categoryName.size(), ' ');
	return formatedName;
}

/* 
	Don't detach a previous attachment for now 
	Use category with standardized path/filename
	Thread: mono / need internal access
*/
void Tintin_reporter::attachCategoryToDestination(Tintin_reporter::log_category & category, const std::string & filename)
{
	std::map<std::string, Tintin_reporter::log_destination>::iterator dest_iterator = this->_opened_files.find(filename);
	if (dest_iterator != this->_opened_files.end())
	{
		/* Destination file already exist, just link to it */
		category.filename = filename;
		dest_iterator->second.nb_references++;
		// LOG_DEBUG(LOG_CATEGORY_LOGGER, "New category '" + category.name + "' linked to '" + category.filename + "'.")
		return ;
	}

	/* Not already exist*/
	if (filename.empty() || filename == LOG_STDOUT_MAGIC)
		this->_opened_files.insert(std::make_pair(filename, newLogDestinationStdout()));
	else if (filename == LOG_STDERR_MAGIC)
		this->_opened_files.insert(std::make_pair(filename, newLogDestinationStderr()));
	else
	{
		dest_iterator = this->_opened_files.insert(std::make_pair(filename, newLogDestination())).first;
		std::ofstream &destination_ofstream = static_cast<std::ofstream &>(dest_iterator->second.output);
		/* TODO directory creation */
		/* File creation here */
		destination_ofstream.open(filename, std::ofstream::out | std::ofstream::app);
		if (destination_ofstream.fail())
		{
			// std::cerr << "dest ofstream failed: |" <<  category.filename << "|" << category.name  << "|" << this->_defaultCategoryName << std::endl;
			// LOG_ERROR(LOG_CATEGORY_LOGGER, "Failed to open file '" + category.filename + "'.")
			// LOG_WARN(LOG_CATEGORY_LOGGER, "Set Category '" + CategoryName + "' equal to '" + this->_categories.at(this->_defaultCategoryName).filename + "'.")
			this->_opened_files.erase(category.filename);
			// this->_categories[Category].name = this->_categories[this->_defaultCategoryName].name;
			category.filename = this->_categories.at(this->_defaultCategoryName).filename;
			return ;
		}
	}
	category.filename = filename;
}

void Tintin_reporter::detachCategoryFromDestination(const Tintin_reporter::log_category & category)
{
	std::map<std::string, Tintin_reporter::log_destination>::iterator dest_iterator = this->_opened_files.find(category.filename);
	if (dest_iterator == this->_opened_files.end())
		return ;
	dest_iterator->second.nb_references--;
	//TODO clean unified file close
	if (dest_iterator->second.nb_references == 0)
	{
		if (dest_iterator->second.is_file)
		{
			static_cast<std::ofstream &>(dest_iterator->second.output).close();
			// LOG_INFO(LOG_CATEGORY_LOGGER, "Closing file '" + category->second.filename + "'")
		}
		this->_opened_files.erase(category.filename);
	}
}


/*

*/
int Tintin_reporter::addCategory(const std::string &CategoryName, const std::string &outfile)
{
	{
	std::lock_guard<std::mutex> lock(Tintin_reporter::static_mutex);

	/* Check if category already exist */
	std::map<std::string, Tintin_reporter::log_category>::iterator cat_it = this->_categories.find(CategoryName);
	if (cat_it != this->_categories.end())
	{
		/* Already existing category */
		detachCategoryFromDestination(cat_it->second);
		attachCategoryToDestination(cat_it->second, formatFilename(outfile));
	}
	else
	{
		/* New category */
		std::map<std::string, Tintin_reporter::log_category>::iterator new_category = this->_categories.insert(std::make_pair(CategoryName, Tintin_reporter::log_category())).first;
		new_category->second.name = formatCategoryName(CategoryName);
		attachCategoryToDestination(new_category->second, formatFilename(outfile));
	}

	// attachCategoryToDestination(*new_category, formatFilename(outfile));
	}

	LOG_DEBUG(LOG_CATEGORY_LOGGER, "New category added '" + formatCategoryName(CategoryName) + "' pointing to '" + formatFilename(outfile) + "'.")
	return EXIT_SUCCESS;
}





/*
	This function is not thread-safe.
	LOG_TIMESTAMP_CURRENT	: Return the standard timestamp.
	LOg_TIMESTAMP_ELAPSED	: Return the timestamp of the time elapsed since the logger start.
	LOg_TIMESTAMP_UTC		: Return the timestamp int the UTC time formart.
*/
std::string			Tintin_reporter::getTimestampStr(const CronType::time_point & timestamp) const
{

	std::stringstream ss;

	if (this->_color_enabled)
		ss << std::right << std::setfill('0') << "[" << LOG_TIMESTAMP_COLOR;
	else
		ss << std::right << std::setfill('0') << "[";

	if (this->_timestamp_format == LOG_TIMESTAMP_UTC)
	{
		char timeStringUTC[sizeof("yyyy-mm-dd hh:mm:ss")];
		std::time_t date = CronType::to_time_t(timestamp);
		// std::strftime(timeStringUTC, sizeof(timeStringUTC), "%FT%TZ", std::gmtime(&date));
		timeStringUTC[std::strftime(timeStringUTC, sizeof(timeStringUTC), "%F %T", std::gmtime(&date))] = '\0';
		ss << timeStringUTC;
	}
	else
	{
		int ms		= 0;
		int sec		= 0;
		int min		= 0;
		int hours	= 0;
		int days	= 0;

		if (this->_timestamp_format == LOG_TIMESTAMP_CURRENT)
		{
			char datestr[sizeof("dd-mm-yyyy")];
			std::time_t	tmpTime = CronType::to_time_t(timestamp);
			std::tm*	local = std::localtime(&tmpTime);
		
			datestr[strftime(datestr, sizeof(datestr), "%d-%m-%Y", local)] = '\0';
			
			long long time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count();
			ms = time_ms % 1000;
			sec = (time_ms / 1000) % 60;
			min =  (time_ms / 60000 ) % 60;
			hours = ((time_ms / 3600000 ) - LOG_TIMESTAMP_SUMMERTIME) % 24;

			ss << datestr << " ";
			ss << std::setw(2) << hours << ":" << std::setw(2) << min << ":" << std::setw(2) << sec << ":" << std::setw(3) << ms;
		}
		else if (this->_timestamp_format == LOG_TIMESTAMP_ELAPSED)
		{
			long long elapsed_ms	= std::chrono::duration_cast<std::chrono::milliseconds>((timestamp - this->_starttime)).count();
			long long elapsed_hours = std::chrono::duration_cast<std::chrono::hours>((timestamp - this->_starttime)).count();
			ms		= elapsed_ms % 1000;
			sec		= (elapsed_ms / 1000) % 60;
			min		=  (elapsed_ms / 60000 ) % 60;
			hours	= elapsed_hours % 24;
			days	= (elapsed_hours / 24);
			ss << std::setw(2) << days << ":" << std::setw(2) << hours << ":";
			ss << std::setw(2) << min  << ":" << std::setw(2) << sec << ":" << std::setw(3) << ms;	
		}
		else
			LOG_CRITICAL(LOG_CATEGORY_LOGGER, "Timestamp format not implemented.")
	}
	if (this->_color_enabled)
		ss << RESET_ANSI << "]";
	else
		ss << "]";
	return ss.str();
}

/*
** --------------------------------- log methods ---------------------------------
*/

/*
	This method is not thtread-safe.
*/
void Tintin_reporter::_log(const log_message & message)
{
	// std::cout << "(" << message.message << ")" << std::endl; // <=== good logger debug
	std::string active_category = message.category;

#if LOG_CATEGORY_AUTO == true

	/* If category don't exist : Create a new category */
	if (this->_categories.find(active_category) == this->_categories.end() && this->addCategory(active_category) == EXIT_FAILURE)
	{
		LOG_CRITICAL(LOG_CATEGORY_LOGGER, "Failed to add category '" + active_category + "' !")
		active_category = this->_defaultCategoryName;
	}
#else
	/* If category don't exist : Use default category */
	if (this->_categories.find(active_category) == this->_categories.end())
	{
		// LOG_WARN(LOG_CATEGORY_LOGGER, "Use of unknown category '" + categoryName + "'.")
		active_category = this->_defaultCategoryName;
	}
#endif

	const char* levelPrefix = NULL;

	switch (message.level)
	{
		case LOG_LEVEL_CRITICAL:
			levelPrefix = this->_color_enabled ? LOG_LEVEL_PREFIX_CRITICAL : LOG_LEVEL_PREFIX_CRITICAL_RAW;
			break;
		case LOG_LEVEL_ERROR:
			levelPrefix = this->_color_enabled ? LOG_LEVEL_PREFIX_ERROR : LOG_LEVEL_PREFIX_ERROR_RAW;
			break;
		case LOG_LEVEL_WARNING:
			levelPrefix = this->_color_enabled ? LOG_LEVEL_PREFIX_WARNING : LOG_LEVEL_PREFIX_WARNING_RAW;
			break;
		case LOG_LEVEL_INFO:
			levelPrefix = this->_color_enabled ? LOG_LEVEL_PREFIX_INFO : LOG_LEVEL_PREFIX_INFO_RAW;
			break;
		case LOG_LEVEL_DEBUG:
		default:
			levelPrefix = this->_color_enabled ? LOG_LEVEL_PREFIX_DEBUG : LOG_LEVEL_PREFIX_DEBUG_RAW;
	}
	
	this->_opened_files.at(this->_categories.at(active_category).filename).output 
		<< this->getTimestampStr(message.timestamp) << "  " 
		<< std::setw(LOG_CATEGORY_NAME_MAXSIZE) << this->_categories.at(active_category).name << "  "
		<< std::setw(LOG_THREAD_NAME_MAXSIZE) << ( "[" + this->_threadnames[message.thread] + "]")
		<< " " << levelPrefix
		<< " - " << message.message
		<< RESET_ANSI << std::endl;
	return ;
}

/*
** --------------------------------- SETTERS ---------------------------------
*/

void Tintin_reporter::setColor(bool enabled)
{
	{
		std::lock_guard<std::mutex> lock(Tintin_reporter::static_mutex);
		this->_color_enabled = enabled;
	}
	return ;
}

/*
** --------------------------------- GETTERS ---------------------------------
*/


const std::map<std::string, Tintin_reporter::log_destination>::const_iterator Tintin_reporter::getOpenFilesBegin(void) const
{
	return this->_opened_files.begin();
}

const std::map<std::string, Tintin_reporter::log_destination>::const_iterator Tintin_reporter::getOpenFilesEnd(void) const
{
	return this->_opened_files.end();
}

const std::map<std::string, Tintin_reporter::log_category>::const_iterator Tintin_reporter::getCategoriesBegin(void) const
{
	return this->_categories.begin();
}

const std::map<std::string, Tintin_reporter::log_category>::const_iterator Tintin_reporter::getCategoriesEnd(void) const
{
	return this->_categories.end();
}

uint Tintin_reporter::getNbOpenfiles(void) const
{
	return this->_opened_files.size();
}

uint Tintin_reporter::getNbCategories(void) const
{
	return this->_categories.size();
}

const std::string &Tintin_reporter::getDefaultCategory(void) const
{
	return this->_defaultCategoryName;
}

bool Tintin_reporter::getColor(void) const
{
	{
		std::lock_guard<std::mutex>(const_cast<std::mutex&>(Tintin_reporter::static_mutex));
		return this->_color_enabled;
	}
}


const std::string & Tintin_reporter::getLogdir( void ) const
{
	return this->_logDirectory;
}

void				Tintin_reporter::setLogdir( const std::string & path )
{
	{
		std::lock_guard<std::mutex> lock(Tintin_reporter::static_mutex);
		this->_logDirectory = path;
	}	
}



/* ************************************************************************** */