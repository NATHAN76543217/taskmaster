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

		Config &		operator=( Config const & rhs );
		
		void			loadConfigFile( void );
		void			loadConfigFile( const std::string & path = "");

	private:
		std::string		_pathConfigFile;
		std::string		_isReadOnly;

};

std::ostream &			operator<<( std::ostream & o, Config const & i );

#endif /* ********************************************************** CONFIG_H */