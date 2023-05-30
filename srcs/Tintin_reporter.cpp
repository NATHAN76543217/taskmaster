#include "Tintin_reporter.hpp"

Tintin_reporter*			Tintin_reporter::_logManager = nullptr;

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Tintin_reporter::Tintin_reporter(const std::string & defaultFile) : 
_defaultFile(defaultFile), 
_timestamp_format(0), 
_color_enabled(true)
{
	this->addCategory(LOG_CATEGORY_DEFAULT, defaultFile);
}

// Tintin_reporter::Tintin_reporter( const Tintin_reporter & src ) : 
// _timestamp_format(src._timestamp_format),
// _color_enabled(src._color_enabled)
// {
// 	(void) src;
// }


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Tintin_reporter::~Tintin_reporter()
{
	LOG_DEBUG(LOG_CATEGORY_DEFAULT, "Closing all output files.")
	for (std::map<std::string, std::ofstream*>::iterator it = this->_opened_files.begin();
		it != this->_opened_files.end();
		it++) 
	{
		it->second->close();
		delete it->second;
	}
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

// Tintin_reporter &				Tintin_reporter::operator=( Tintin_reporter const & rhs )
// {
// 	(void) rhs;
// 	//if ( this != &rhs )
// 	//{
// 		//this->_value = rhs.getValue();
// 	//}
// 	return *this;
// }

std::ostream &			operator<<( std::ostream & o, Tintin_reporter const & i )
{
	(void)i;
	//o << "Value = " << i.getValue();
	return o;
}


/*
** --------------------------------- METHODS ----------------------------------
*/

int		Tintin_reporter::addCategory(const std::string & CategoryName, std::string outfile)
{
	if (CategoryName.size() > LOG_CATEGORY_NAME_MAXSIZE)
		this->_categories[CategoryName].name = CategoryName.substr(0, LOG_CATEGORY_NAME_MAXSIZE);
	else
	{
		std::string newCategoryName(CategoryName);
		
		newCategoryName.append(LOG_CATEGORY_NAME_MAXSIZE - CategoryName.size(), ' ');
		this->_categories[CategoryName].name = newCategoryName;
	}

	if (outfile.empty())
		outfile = this->_defaultFile;

	this->_categories[CategoryName].filename = outfile;
	
	std::map<std::string, std::ofstream*>::iterator of = this->_opened_files.find(outfile);
	if ( of == this->_opened_files.end() )
	{
		/* Output file don't exist*/
		// this->_opened_files.insert( { outfile, outfile } );
		of = this->_opened_files.insert(std::make_pair(outfile, new std::ofstream)).first;

		// dprintf(1, "Create a new ofstream linked to nothing\n");
	}

	if (of->second->is_open() == false)
	{
		/* ofstream exist but is closed */
		of->second->open(outfile, std::ofstream::app);
		if (of->second->fail() )
		{
			/* Fail to open file */
			// LOG_ERROR(LOG_CATEGORY_DEFAULT, "Failed to open file '" + outfile + "'.")
			return EXIT_FAILURE;
		}
		// LOG_DEBUG(LOG_CATEGORY_DEFAULT, "File '" + outfile + "' opened successfuly.")
	}
	return EXIT_SUCCESS;
}

void	Tintin_reporter::log(uint level, const std::string & category, const std::string & message)
{

#if LOG_CATEGORY_AUTO == true

	/* Create the category if don't already exist */
	if (this->_categories.find(category) == this->_categories.end())
		this->addCategory(category);

#endif

	switch (level)
    {
        case LOG_LEVEL_CRITICAL:
            this->log_critical(category, message);
            break;
        case LOG_LEVEL_ERROR:
            this->log_error(category, message);
            break;
		case LOG_LEVEL_WARNING:
            this->log_warning(category, message);
            break;
        case LOG_LEVEL_INFO:
            this->log_info(category, message);
            break;
		case LOG_LEVEL_DEBUG:
        default:
            this->log_debug(category, message);
            break;
    }

}

/*
** --------------------------------- log methods ---------------------------------
*/


void			Tintin_reporter::log_debug(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " " << LOG_LEVEL_DEBUG_MSG << message << RESET_ANSI << std::endl;
	else
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " [DEBUG] " << message << RESET_ANSI << std::endl;
}

void			Tintin_reporter::log_info(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " " << LOG_LEVEL_INFO_MSG << message << RESET_ANSI << std::endl;
	else
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " [INFO] " << message << RESET_ANSI << std::endl;
}

void			Tintin_reporter::log_warning(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " " << LOG_LEVEL_WARNING_MSG << message << RESET_ANSI << std::endl;
	else
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " [WARNING] " << message << RESET_ANSI << std::endl;
}


void			Tintin_reporter::log_error(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " " << LOG_LEVEL_ERROR_MSG << message << RESET_ANSI << std::endl;
	else
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " [ERROR] " << message << RESET_ANSI << std::endl;
}

void			Tintin_reporter::log_critical(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " " << LOG_LEVEL_CRITICAL_MSG << message << RESET_ANSI << std::endl;
	else
		*(this->_opened_files[this->_categories[category].filename]) << this->getLogHead(category) << " [CRITICAL] " << message << RESET_ANSI << std::endl;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

const std::string Tintin_reporter::getLogHead(const std::string & category) const
{
	return this->getTimestamp() + " " + this->_categories.at(category).name;
}


std::string	Tintin_reporter::getTimestamp( void ) const
{
    struct timeval		time_now;
	std::stringstream	ss;

    gettimeofday(&time_now, nullptr);

	std::string ms(ntos(time_now.tv_usec));
	ms.resize(3);
	if (this->_timestamp_format == LOG_TIMESTAMP_DEFAULT)
	{
		if (this->_color_enabled)
			ss << "[" << LOG_TIMESTAMP_COLOR << timeToString(time_now) << ":" << ms << RESET_ANSI"]";
		else
			ss << "[" << timeToString(time_now) << ":" << ms << "]";
	}
	return ss.str();

}


std::string				Tintin_reporter::timeToString( const struct timeval & time ) const
{
	char		buffer[80];
	struct tm*	timeinfo;

	timeinfo = localtime(&time.tv_sec);
	buffer[strftime(buffer, sizeof(buffer) -1, "%d-%m-%Y %H:%M:%S", timeinfo)] = '\0';
	return buffer;
}

/* ************************************************************************** */