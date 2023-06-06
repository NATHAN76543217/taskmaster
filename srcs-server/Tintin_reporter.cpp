#include "Tintin_reporter.hpp"
//TODO Improve verbose: say 'new' when new category and say 'set' when relink category (do the same for files)
//TODO optimize between addCategpry and addDefaultCategory
Tintin_reporter *Tintin_reporter::_logManager = nullptr;

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Tintin_reporter::Tintin_reporter(const std::string &defaultFile) : _defaultCategory(LOG_CATEGORY_DEFAULT),
																   _timestamp_format(0),
																   _color_enabled(false)
{
	this->addDefaultCategory(defaultFile);
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
			LOG_DEBUG(LOG_CATEGORY_LOGGER, "Closing file '" + it->first + "'.")
			static_cast<std::ofstream &>(it->second.output).close();
			delete &(it->second.output);
		}
	}

	Tintin_reporter::log_destination &default_destination = this->_opened_files.at(this->_categories.at(this->_defaultCategory).filename);
	if (default_destination.is_file == true)
	{
		LOG_DEBUG(LOG_CATEGORY_LOGGER, "Closing file '" + this->_categories.at(this->_defaultCategory).filename + "'.")
		static_cast<std::ofstream &>(default_destination.output).close();
		delete &(default_destination.output);
	}
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

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
	log(LOG_LEVEL_DEBUG, LOG_CATEGORY_LOGGER, "New category added '" + this->_defaultCategory + "' pointing to '" + defaultOutputFile + "'.");
	log(LOG_LEVEL_DEBUG, LOG_CATEGORY_LOGGER, "New category added '" LOG_CATEGORY_LOGGER "' pointing to '" + defaultOutputFile + "'.");
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
				LOG_INFO(LOG_CATEGORY_LOGGER, "Closing file '" + cat_it->second.filename + "'")
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
		LOG_WARN(LOG_CATEGORY_LOGGER, "Set Category '" + CategoryName + "' point to default file (No filename provided).")
		return EXIT_SUCCESS;
	}

	new_category.filename = outfile;
	std::map<std::string, Tintin_reporter::log_destination>::iterator cat_dst = this->_opened_files.find(new_category.filename);
	if (cat_dst != this->_opened_files.end())
	{
		/* Destination file already exist, just link to it */
		cat_dst->second.nb_references++;
		LOG_DEBUG(LOG_CATEGORY_LOGGER, "New category '" + CategoryName + "' linked to '" + new_category.filename + "'.")
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
			LOG_ERROR(LOG_CATEGORY_LOGGER, "Failed to open file '" + outfile + "'.")
			LOG_WARN(LOG_CATEGORY_LOGGER, "Set Category '" + CategoryName + "' equal to '" + this->_categories.at(this->_defaultCategory).filename + "'.")
			this->_opened_files.erase(outfile);
			// this->_categories[Category].name = this->_categories[this->_defaultCategory].name;
			new_category.filename = this->_categories.at(this->_defaultCategory).filename;
		}
	}
	LOG_DEBUG(LOG_CATEGORY_LOGGER, "New category added '" + CategoryName + "' pointing to '" + new_category.filename + "'.")
	return EXIT_SUCCESS;
}



void Tintin_reporter::log(uint level, const std::string &categoryName, const std::string &message)
{

	std::string active_category = categoryName;

#if LOG_CATEGORY_AUTO == true

	/* If category don't exist : Create a new category */
	if (this->_categories.find(categoryName) == this->_categories.end() && this->addCategory(categoryName) == EXIT_FAILURE)
	{
		LOG_CRITICAL(LOG_CATEGORY_LOGGER, "Failed to add category '" + categoryName + "' !")
		active_category = this->_defaultCategory;
	}
#else
	/* If category don't exist : Use default category */
	if (this->_categories.find(categoryName) == this->_categories.end())
	{
		LOG_WARN(LOG_CATEGORY_LOGGER, "Use of unknown category '" + categoryName + "'.")
		active_category = this->_defaultCategory;
	}
#endif

	switch (level)
	{
	case LOG_LEVEL_CRITICAL:
		this->log_critical(active_category, message);
		break;
	case LOG_LEVEL_ERROR:
		this->log_error(active_category, message);
		break;
	case LOG_LEVEL_WARNING:
		this->log_warning(active_category, message);
		break;
	case LOG_LEVEL_INFO:
		this->log_info(active_category, message);
		break;
	case LOG_LEVEL_DEBUG:
	default:
		this->log_debug(active_category, message);
		break;
	}
}

/*
** --------------------------------- log methods ---------------------------------
*/

void Tintin_reporter::log_debug(const std::string &category, const std::string &message)
{
	if (this->_color_enabled)
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_DEBUG << message << RESET_ANSI << std::endl;
	else
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_DEBUG_NOCOLOR << message << RESET_ANSI << std::endl;
}

void Tintin_reporter::log_info(const std::string &category, const std::string &message)
{
	if (this->_color_enabled)
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_INFO << message << RESET_ANSI << std::endl;
	else
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_INFO_NOCOLOR<< message << RESET_ANSI << std::endl;
}

void Tintin_reporter::log_warning(const std::string &category, const std::string &message)
{
	if (this->_color_enabled)
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_WARNING << message << RESET_ANSI << std::endl;
	else
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_WARNING_NOCOLOR << message << RESET_ANSI << std::endl;
}

void Tintin_reporter::log_error(const std::string &category, const std::string &message)
{
	if (this->_color_enabled)
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_ERROR << message << RESET_ANSI << std::endl;
	else
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_ERROR_NOCOLOR<< message << RESET_ANSI << std::endl;
}

void Tintin_reporter::log_critical(const std::string &category, const std::string &message)
{
	if (this->_color_enabled)
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_CRITICAL << message << RESET_ANSI << std::endl;
	else
		this->_opened_files.at(this->_categories.at(category).filename).output << this->getLogHead(category) << LOG_LEVEL_PREFIX_CRITICAL_NOCOLOR << message << RESET_ANSI << std::endl;
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

const std::string Tintin_reporter::getLogHead(const std::string &category) const
{
	return this->getTimestamp() + " " + this->_categories.at(category).name;
}

std::string Tintin_reporter::getTimestamp(void) const
{
	struct timeval time_now;
	std::stringstream ss;

	gettimeofday(&time_now, nullptr);

	std::string ms(ntos(time_now.tv_usec));
	ms.resize(3);
	if (this->_timestamp_format == LOG_TIMESTAMP_DEFAULT)
	{
		if (this->_color_enabled)
			ss << "[" << LOG_TIMESTAMP_COLOR << timeToString(time_now) << ":" << ms << RESET_ANSI "]";
		else
			ss << "[" << timeToString(time_now) << ":" << ms << "]";
	}
	return ss.str();
}

std::string Tintin_reporter::timeToString(const struct timeval &time) const
{
	char buffer[80];
	struct tm *timeinfo;

	timeinfo = localtime(&time.tv_sec);
	buffer[strftime(buffer, sizeof(buffer) - 1, "%d-%m-%Y %H:%M:%S", timeinfo)] = '\0';
	return buffer;
}

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