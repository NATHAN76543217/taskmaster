#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <iostream>
# include <string>
# include "yaml-cpp/yaml.h"
# include "Tintin_reporter.hpp"

class Config
{

	public:

		Config();
		Config( Config const & src );
		~Config();

		YAML::Node		operator[]( const std::string & keyword );
		Config &		operator=( Config const & rhs );
		
		int					reloadConfigFile( void );
		int					loadConfigFile( const std::string & path = "");
		const std::string	&getPath( void ) const;

	private:
		YAML::Node		_config;
		std::string		_pathConfigFile;
		bool			_isReadOnly;

};

std::ostream &			operator<<( std::ostream & o, Config const & i );

#endif /* ********************************************************** CONFIG_H */