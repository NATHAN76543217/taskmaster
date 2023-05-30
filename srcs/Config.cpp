#include "Config.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Config::Config()
{
}

Config::Config( const Config & src ) : _isReadOnly(src._isReadOnly)
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Config::~Config()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Config &				Config::operator=( Config const & rhs )
{
	(void) rhs;
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, Config const & i )
{
	(void) o;
	(void) i;

	//o << "Value = " << i.getValue();
	return o;
}


/*
** --------------------------------- METHODS ----------------------------------
*/

void					Config::loadConfigFile()
{

}

void					Config::loadConfigFile( const std::string & path)
{
	YAML::Node config = YAML::LoadFile(path);
	if (config.size() == 0)
	{
		LOG_ERROR(LOG_CATEGORY_CONFIG, "Empty configuration file")
		return ;
	}
	LOG_INFO(LOG_CATEGORY_CONFIG, "Configuration file loaded.")

	std::cout << config << std::endl;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */