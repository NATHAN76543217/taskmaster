#ifndef NTOS_HPP
# define NTOS_HPP

# include <string>
# include <sstream>

template<typename T>
std::string ntos ( T Number )
{
	std::ostringstream ss;
	ss << std::boolalpha << Number;
	return ss.str();
}

# endif		// NTOS_HPP