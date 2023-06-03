#ifndef TINTIN_REPORTER_HPP
# define TINTIN_REPORTER_HPP

# include <iomanip>
# include <iostream>
# include <fstream>
# include <string>
# include <map>
# include <sys/time.h>
# include <ntos.hpp>

# define LOG_LEVEL_DEBUG	0
# define LOG_LEVEL_INFO		1
# define LOG_LEVEL_WARNING	2
# define LOG_LEVEL_ERROR	3
# define LOG_LEVEL_CRITICAL	4

// define basic ansi colors
# define RESET_ANSI		"\033[0m"		
# define BOLD_ANSI		"\033[1m"		
# define RED_ANSI		"\033[91m"		
# define YELLOW_ANSI	"\033[93m"		
# define BLUE_ANSI		"\033[96m"		
# define LGREY_ANSI		"\033[37m"		
# define GREEN_ANSI		"\033[92m"		
# define DBLUE_ANSI		"\033[94m"	

// define prefixes RGB's here
# define LOG_LEVEL_DEBUG_COLOR    "\033[38;2;180;210;255m\033[3m"
# define LOG_LEVEL_INFO_COLOR     "\033[38;2;150;250;105m"
# define LOG_LEVEL_WARNING_COLOR  "\033[38;2;250;230;15m\033[3m"
# define LOG_LEVEL_ERROR_COLOR    "\033[38;2;255;67;33m\033[1m"
# define LOG_LEVEL_CRITICAL_COLOR	"\033[48;2;250;100;20m\033[1m"

// define message output color here
# define LOG_LEVEL_DEBUG_MSG_COLOR    "\033[38;2;180;210;255m"
# define LOG_LEVEL_INFO_MSG_COLOR     "\033[38;2;230;230;210m"
# define LOG_LEVEL_WARNING_MSG_COLOR  YELLOW_ANSI "\033[38;2;250;230;40m"
# define LOG_LEVEL_ERROR_MSG_COLOR    "\033[38;2;255;77;50m"
# define LOG_LEVEL_CRITICAL_MSG_COLOR "\033[38;2;250;190;80m"

// define prefix structure here
# define LOG_LEVEL_DEBUG_MSG		"[" LOG_LEVEL_DEBUG_COLOR "DEBUG" RESET_ANSI "]  - " LOG_LEVEL_DEBUG_MSG_COLOR
# define LOG_LEVEL_INFO_MSG			"[" LOG_LEVEL_INFO_COLOR "INFO" RESET_ANSI "]   - " LOG_LEVEL_INFO_MSG_COLOR
# define LOG_LEVEL_WARNING_MSG		"[" LOG_LEVEL_WARNING_COLOR "WARN" RESET_ANSI "]   - " LOG_LEVEL_WARNING_MSG_COLOR
# define LOG_LEVEL_ERROR_MSG		"[" LOG_LEVEL_ERROR_COLOR "ERROR" RESET_ANSI "]  - " LOG_LEVEL_ERROR_MSG_COLOR
# define LOG_LEVEL_CRITICAL_MSG		"[" LOG_LEVEL_CRITICAL_COLOR "CRITIC" RESET_ANSI "] - " LOG_LEVEL_CRITICAL_MSG_COLOR


// define timestamp color here
# define LOG_TIMESTAMP_COLOR    "\033[38;2;100;100;120m"

# define LOG_LEVEL_NAME_MAXSIZE		8
# define LOG_CATEGORY_NAME_MAXSIZE	7

# define LOG_CATEGORY_DEFAULT	"DEFAULT"
# define LOG_CATEGORY_INIT		"INIT"
# define LOG_CATEGORY_SIGNAL	"SIGNAL"
# define LOG_CATEGORY_NETWORK	"NETWORKING"
# define LOG_CATEGORY_CONFIG	"CONFIG"

/* If enabled create automatically new categories without throwing an exception */
# define LOG_CATEGORY_AUTO		false

// # define LOG_MANUAL(level, category, file, log) getLogManager().level(level).category(category).tofile(file).log(log);
# define LOG_DEBUG(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::getLogManager().log(LOG_LEVEL_DEBUG, category, std::string(_s.str())); }
# define LOG_INFO(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::getLogManager().log(LOG_LEVEL_INFO, category, std::string(_s.str())); }
# define LOG_WARN(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::getLogManager().log(LOG_LEVEL_WARNING, category, std::string(_s.str())); }
# define LOG_ERROR(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::getLogManager().log(LOG_LEVEL_ERROR, category, std::string(_s.str())); }
# define LOG_CRITICAL(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::getLogManager().log(LOG_LEVEL_CRITICAL, category, std::string(_s.str())); }

# define LOG_TIMESTAMP_DEFAULT 0

class Tintin_reporter
{
	private:
		Tintin_reporter(const std::string & defaultFile);
		~Tintin_reporter();
		
		class Category
		{
			public:
				Category() {};
				~Category() {};
				std::string		name;
				std::string		filename;
			//add FD
		};

		static Tintin_reporter*					_logManager;
		std::map<std::string, std::ofstream*>	_opened_files;
		std::map<std::string, Category>			_categories;

		std::string					_defaultFile;
		uint						_timestamp_format;
		uint						_color_enabled;
		// uint						nb_categories;


	public:

		Tintin_reporter( Tintin_reporter & src ) = delete;
		Tintin_reporter( Tintin_reporter const & src ) = delete;

		Tintin_reporter &		operator=( Tintin_reporter const & rhs ) = delete;


		static Tintin_reporter& getLogManager(const std::string & defaultFile = "") {
			if (_logManager == nullptr) {
				_logManager = new Tintin_reporter(defaultFile);
			}
			return *_logManager;
		}

		static void	destroyLogManager( void )
		{
			if (_logManager != nullptr)
				delete _logManager;
		}


		void			setColor( bool enabled );
		std::string		getTimestamp( void ) const;
		std::string		timeToString( const struct timeval & time ) const;
		int				addCategory(const std::string & str, std::string outfile = "");

		void			log(uint level, const std::string & category, const std::string & message);
		void			log_critical(const std::string & category, const std::string & message);
		void			log_error(const std::string & category, const std::string & message);
		void			log_warning(const std::string & category, const std::string & message);
		void			log_info(const std::string & category, const std::string & message);
		void			log_debug(const std::string & category, const std::string & message);
		const std::string	getLogHead(const std::string & category) const;

	private:

};

std::ostream &			operator<<( std::ostream & o, Tintin_reporter const & i );

#endif /* ************************************************* TINTIN_REPORTER_H */