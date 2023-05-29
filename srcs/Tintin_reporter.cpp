#include "Tintin_reporter.hpp"

Tintin_reporter*			Tintin_reporter::_logManager = nullptr;

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Tintin_reporter::Tintin_reporter() : _timestamp_format(0), _color_enabled(true)
{
	this->addCategory(LOG_CATEGORY_DEFAULT);
}

Tintin_reporter::Tintin_reporter( const Tintin_reporter & src ) : 
_timestamp_format(src._timestamp_format),
_color_enabled(src._color_enabled)
{
	(void) src;
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Tintin_reporter::~Tintin_reporter()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Tintin_reporter &				Tintin_reporter::operator=( Tintin_reporter const & rhs )
{
	(void) rhs;
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, Tintin_reporter const & i )
{
	(void)i;
	//o << "Value = " << i.getValue();
	return o;
}


/*
** --------------------------------- METHODS ----------------------------------
*/

void	Tintin_reporter::addCategory(const std::string & CategoryName)
{
	if (CategoryName.size() > LOG_CATEGORY_NAME_MAXSIZE)
		this->_categories[CategoryName].name = CategoryName.substr(0, LOG_CATEGORY_NAME_MAXSIZE);
	else
		this->_categories[CategoryName].name = CategoryName;
}

void	Tintin_reporter::log(uint level, const std::string & category, const std::string & message)
{
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
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " " <<  LOG_LEVEL_DEBUG_MSG << " " << message << RESET_ANSI << std::endl;
	else
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " [DEBUG] " << message << RESET_ANSI << std::endl;
}

void			Tintin_reporter::log_info(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " " <<  LOG_LEVEL_INFO_MSG << " " << message << RESET_ANSI << std::endl;
	else
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " [INFO] " << message << RESET_ANSI << std::endl;
}

void			Tintin_reporter::log_warning(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " " <<  LOG_LEVEL_WARNING_MSG << " " << message << RESET_ANSI << std::endl;
	else
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " [WARNING] " << message << RESET_ANSI << std::endl;
}


void			Tintin_reporter::log_error(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " " <<  LOG_LEVEL_ERROR_MSG << " " << message << RESET_ANSI << std::endl;
	else
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " [ERROR] " << message << RESET_ANSI << std::endl;
}

void			Tintin_reporter::log_critical(const std::string & category, const std::string & message)
{
	if (this->_color_enabled)
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " " <<  LOG_LEVEL_CRITICAL_MSG << " " << message << RESET_ANSI << std::endl;
	else
		std::cout << this->getTimestamp() << " " << this->_categories.at(category).name << " [CRITICAL] " << message << RESET_ANSI << std::endl;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

std::string	Tintin_reporter::getTimestamp( void )
{
    struct timeval		time_now;
	std::stringstream	ss;

    gettimeofday(&time_now, nullptr);

	std::string ms(ntos(time_now.tv_usec));
	ms.resize(3);
	if (this->_color_enabled)
		ss << "[" << LOG_TIMESTAMP_COLOR << timeToString(time_now) << ":" << ms << RESET_ANSI"]";
	else
		ss << "[" << timeToString(time_now) << ":" << ms << "]";
	return ss.str();

}


std::string				Tintin_reporter::timeToString( const struct timeval & time )
{
	char		buffer[80];
	struct tm*	timeinfo;

	timeinfo = localtime(&time.tv_sec);
	buffer[strftime(buffer, sizeof(buffer) -1, "%d-%m-%Y %H:%M:%S", timeinfo)] = '\0';
	return buffer;
}

/* ************************************************************************** */