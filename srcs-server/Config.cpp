#include "Config.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Config::Config() : _pathConfigFile(""), _isReadOnly(true)
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

YAML::Node					Config::operator[]( const std::string & keyword )
{
	return this->_config[keyword];
}

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



int					Config::loadConfigFile( const std::string & path)
{
	this->_config = YAML::LoadFile(path);
	if (this->_config.size() == 0)
	{
		LOG_ERROR(LOG_CATEGORY_CONFIG, "Empty configuration file")
		return EXIT_FAILURE;
	}
	this->_pathConfigFile = path;
	LOG_INFO(LOG_CATEGORY_CONFIG, "Configuration file loaded.")

	LOG_DEBUG(LOG_CATEGORY_CONFIG, "Loaded config = \n" + ntos(this->_config))
	return EXIT_SUCCESS;
}


int						Config::reloadConfigFile( void )
{
	if (this->_pathConfigFile.empty())
	{
		LOG_ERROR(LOG_CATEGORY_CONFIG, "No config path loaded yet.")
		return EXIT_FAILURE;
	}	
	this->loadConfigFile(this->_pathConfigFile);
	return EXIT_SUCCESS;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

const std::string&		Config::getPath( void ) const
{
	return this->_pathConfigFile;
}


/* ************************************************************************** */