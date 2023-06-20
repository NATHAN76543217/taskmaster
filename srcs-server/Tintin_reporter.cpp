#include "Tintin_reporter.hpp"
//TODO Improve verbose: say 'new' when new category and say 'set' when relink category (do the same for files)
//TODO optimize between addCategpry and addDefaultCategory

/*
** ------------------------------- Static VAR init --------------------------------
*/
std::condition_variable						Tintin_reporter::_hasUpdate;
std::mutex									Tintin_reporter::_mutexUpdate;
std::queue<Tintin_reporter::log_message>	Tintin_reporter::_messageQueue = std::queue<Tintin_reporter::log_message>();


/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Tintin_reporter::Tintin_reporter(const std::string &defaultFile) : 
																AThread<Tintin_reporter>(*this, defaultFile),
																_defaultCategory(LOG_CATEGORY_DEFAULT),
																_timestamp_format(LOG_TIMESTAMP_CURRENT),
																_starttime(),
																_color_enabled(false)
{
	this->addDefaultCategory(defaultFile);
	this->_starttime = CronType::now();
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Tintin_reporter::~Tintin_reporter()
{
	for (std::map<std::string, Tintin_reporter::log_destination>::iterator it = this->_opened_files.begin();
		 it != this->_opened_files.end();
		 it++)
	{
		if (this->_categories.at(this->_defaultCategory).filename == it->first)
			continue;
		if (it->second.is_file == true)
		{
			// LOG_DEBUG(LOG_CATEGORY_LOGGER, "Closing file '" + it->first + "'.")
			static_cast<std::ofstream &>(it->second.output).close();
			delete &(it->second.output);
		}
	}

	Tintin_reporter::log_destination &default_destination = this->_opened_files.at(this->_categories.at(this->_defaultCategory).filename);
	if (default_destination.is_file == true)
	{
		// LOG_DEBUG(LOG_CATEGORY_LOGGER, "Closing file '" + this->_categories.at(this->_defaultCategory).filename + "'.")
		static_cast<std::ofstream &>(default_destination.output).close();
		delete &(default_destination.output);
	}
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

void				Tintin_reporter::operator()( void )
{
	LOG_INFO(LOG_CATEGORY_DEFAULT, "Logger constructed - wait to start");
	{
		std::unique_lock<std::mutex> lock(this->_internal_mutex);
		this->_ready.wait(lock);
	}

	LOG_INFO(LOG_CATEGORY_DEFAULT, "Logger thread start.");

	std::unique_lock<std::mutex> update_lock(Tintin_reporter::_mutexUpdate);
	std::unique_lock<std::mutex> intern_lock(Tintin_reporter::_internal_mutex);
	
	intern_lock.unlock();
	do {
		intern_lock.lock();
		while (!Tintin_reporter::_messageQueue.empty())
		{
			this->_log(this->_messageQueue.front());
			Tintin_reporter::_messageQueue.pop();
		}
		{
			std::lock_guard<std::mutex> lk(this->_internal_mutex);
			if (this->_running == false)
				break;
		}
		intern_lock.unlock();
		Tintin_reporter::_hasUpdate.wait(update_lock);
	}
	while (true);
	LOG_INFO(LOG_CATEGORY_DEFAULT, "Logger thread end.")
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
	Tintin_reporter::log_destination dst(false, 1, std::cout);

	return dst;
}

Tintin_reporter::log_destination Tintin_reporter::newLogDestinationStderr(void) const
{
	Tintin_reporter::log_destination dst(false, 1, std::cerr);

	return dst;
}

std::string Tintin_reporter::formatCategoryName(const std::string &categoryName) const
{
	std::string formatedName(categoryName);

	if (categoryName.size() > LOG_CATEGORY_NAME_MAXSIZE)
		formatedName = categoryName.substr(0, LOG_CATEGORY_NAME_MAXSIZE);
	else
		formatedName.append(LOG_CATEGORY_NAME_MAXSIZE - categoryName.size(), ' ');
	return formatedName;
}

void Tintin_reporter::addDefaultCategory(const std::string &defaultOutputFile)
{
	if (this->_categories.empty() == false)
		return;

	/* Add default category */

	Tintin_reporter::log_category category;
	category.name = formatCategoryName(this->_defaultCategory);
	category.filename = defaultOutputFile;

	this->_categories.insert(std::make_pair(this->_defaultCategory, category));

	/* Open default file */
	std::map<std::string, Tintin_reporter::log_destination>::iterator def_logdest;

	if (category.filename == LOG_STDOUT_MAGIC)
		def_logdest = this->_opened_files.insert(std::make_pair(category.filename, newLogDestinationStdout())).first;
	else if (category.filename == LOG_STDERR_MAGIC)
		def_logdest = this->_opened_files.insert(std::make_pair(category.filename, newLogDestinationStderr())).first;
	else
	{
		def_logdest = this->_opened_files.insert(std::make_pair(defaultOutputFile, newLogDestination())).first;
		std::ofstream &dst_of = static_cast<std::ofstream &>(def_logdest->second.output);
		dst_of.open(defaultOutputFile, std::ofstream::app);
		if (dst_of.fail())
		{
			this->_categories.erase(this->_defaultCategory);
			this->_opened_files.erase(defaultOutputFile);
			throw Tintin_reporter::defaultFileException(defaultOutputFile);
		}
	}
	/* Add category 'LOGGER' */

	std::string loggerName = formatCategoryName(LOG_CATEGORY_LOGGER);
	this->_categories[LOG_CATEGORY_LOGGER].name = loggerName;
	this->_categories[LOG_CATEGORY_LOGGER].filename = defaultOutputFile;
	def_logdest->second.nb_references++;
	// log(LOG_LEVEL_DEBUG, LOG_CATEGORY_LOGGER, "New category added '" + this->_defaultCategory + "' pointing to '" + defaultOutputFile + "'.");
	// log(LOG_LEVEL_DEBUG, LOG_CATEGORY_LOGGER, "New category added '" LOG_CATEGORY_LOGGER "' pointing to '" + defaultOutputFile + "'.");
}


int Tintin_reporter::addCategory(const std::string &CategoryName, const std::string &outfile)
{
	/* Check if category already exist */
	std::map<std::string, Tintin_reporter::log_category>::iterator cat_it = this->_categories.find(CategoryName);
	if (cat_it != this->_categories.end())
	{
		log_destination &old_dst = this->_opened_files.at(cat_it->second.filename);
		old_dst.nb_references--;
		if (old_dst.nb_references == 0)
		{
			if (old_dst.is_file)
			{
				static_cast<std::ofstream &>(old_dst.output).close();
				// LOG_INFO(LOG_CATEGORY_LOGGER, "Closing file '" + cat_it->second.filename + "'")
			}
			this->_opened_files.erase(cat_it->second.filename);
		}
	}

	Tintin_reporter::log_category & new_category = this->_categories.insert(std::make_pair(CategoryName, Tintin_reporter::log_category())).first->second;
	new_category.name = formatCategoryName(CategoryName);
	if (outfile.empty())
	{
		/* If no filename provided, log to default file */
		new_category.filename = this->_categories.at(this->_defaultCategory).filename;
		this->_opened_files.at(new_category.filename).nb_references++;
		// LOG_WARN(LOG_CATEGORY_LOGGER, "Set Category '" + CategoryName + "' point to default file (No filename provided).")
		return EXIT_SUCCESS;
	}

	new_category.filename = outfile;
	std::map<std::string, Tintin_reporter::log_destination>::iterator cat_dst = this->_opened_files.find(new_category.filename);
	if (cat_dst != this->_opened_files.end())
	{
		/* Destination file already exist, just link to it */
		cat_dst->second.nb_references++;
		// LOG_DEBUG(LOG_CATEGORY_LOGGER, "New category '" + CategoryName + "' linked to '" + new_category.filename + "'.")
		return EXIT_SUCCESS;
	}

	/* Not already exist*/
	if (new_category.filename == LOG_STDOUT_MAGIC)
	{
		this->_opened_files.insert(std::make_pair(new_category.filename, newLogDestinationStdout()));
	}
	else if (new_category.filename == LOG_STDERR_MAGIC)
	{
		this->_opened_files.insert(std::make_pair(new_category.filename, newLogDestinationStderr()));
	}
	else
	{
		cat_dst = this->_opened_files.insert(std::make_pair(new_category.filename, newLogDestination())).first;
		std::ofstream &destination_ofstream = static_cast<std::ofstream &>(cat_dst->second.output);
		destination_ofstream.open(outfile, std::ofstream::app);
		if (destination_ofstream.fail())
		{
			// LOG_ERROR(LOG_CATEGORY_LOGGER, "Failed to open file '" + outfile + "'.")
			// LOG_WARN(LOG_CATEGORY_LOGGER, "Set Category '" + CategoryName + "' equal to '" + this->_categories.at(this->_defaultCategory).filename + "'.")
			this->_opened_files.erase(outfile);
			// this->_categories[Category].name = this->_categories[this->_defaultCategory].name;
			new_category.filename = this->_categories.at(this->_defaultCategory).filename;
		}
	}
	LOG_DEBUG(LOG_CATEGORY_LOGGER, "New category added '" + CategoryName + "' pointing to '" + new_category.filename + "'.")
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
	std::string active_category = message.category;

#if LOG_CATEGORY_AUTO == true

	/* If category don't exist : Create a new category */
	if (this->_categories.find(active_category) == this->_categories.end() && this->addCategory(active_category) == EXIT_FAILURE)
	{
		LOG_CRITICAL(LOG_CATEGORY_LOGGER, "Failed to add category '" + active_category + "' !")
		active_category = this->_defaultCategory;
	}
#else
	/* If category don't exist : Use default category */
	if (this->_categories.find(active_category) == this->_categories.end())
	{
		// LOG_WARN(LOG_CATEGORY_LOGGER, "Use of unknown category '" + categoryName + "'.")
		active_category = this->_defaultCategory;
	}
#endif

	const char* levelPrefix = NULL;

	if (this->_color_enabled)
	{
		switch (message.level)
		{
			case LOG_LEVEL_CRITICAL:
				levelPrefix = LOG_LEVEL_PREFIX_CRITICAL;
				break;
			case LOG_LEVEL_ERROR:
				levelPrefix = LOG_LEVEL_PREFIX_ERROR;
				break;
			case LOG_LEVEL_WARNING:
				levelPrefix = LOG_LEVEL_PREFIX_WARNING;
				break;
			case LOG_LEVEL_INFO:
				levelPrefix = LOG_LEVEL_PREFIX_INFO;
				break;
			case LOG_LEVEL_DEBUG:
			default:
				levelPrefix = LOG_LEVEL_PREFIX_DEBUG;
		}
	}
	else
	{
		switch (message.level)
		{
			case LOG_LEVEL_CRITICAL:
				levelPrefix = LOG_LEVEL_PREFIX_CRITICAL_NOCOLOR;
				break;
			case LOG_LEVEL_ERROR:
				levelPrefix = LOG_LEVEL_PREFIX_ERROR_NOCOLOR;
				break;
			case LOG_LEVEL_WARNING:
				levelPrefix = LOG_LEVEL_PREFIX_WARNING_NOCOLOR;
				break;
			case LOG_LEVEL_INFO:
				levelPrefix = LOG_LEVEL_PREFIX_INFO_NOCOLOR;
				break;
			case LOG_LEVEL_DEBUG:
			default:
				levelPrefix = LOG_LEVEL_PREFIX_DEBUG_NOCOLOR;
		}
	}
	
	this->_opened_files.at(this->_categories.at(active_category).filename).output 
		<< this->getTimestampStr(message.timestamp) << " " 
		<< this->_categories.at(active_category).name 
		<< levelPrefix
		<< message.message
		<< RESET_ANSI << std::endl;

	return ;
}

/*
** --------------------------------- SETTERS ---------------------------------
*/

void Tintin_reporter::setColor(bool enabled)
{
	this->_color_enabled = enabled;
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
	return this->_defaultCategory;
}

bool Tintin_reporter::getColor(void) const
{
	return this->_color_enabled;
}


/* ************************************************************************** */