#ifndef TINTIN_REPORTER_HPP
# define TINTIN_REPORTER_HPP

# include <iomanip>
# include <iostream>
# include <ostream>
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
# define LOG_LEVEL_PREFIX_DEBUG				" [" LOG_LEVEL_DEBUG_COLOR "DEBUG" RESET_ANSI "]  - " LOG_LEVEL_DEBUG_MSG_COLOR
# define LOG_LEVEL_PREFIX_INFO				" [" LOG_LEVEL_INFO_COLOR "INFO" RESET_ANSI "]   - " LOG_LEVEL_INFO_MSG_COLOR
# define LOG_LEVEL_PREFIX_WARNING			" [" LOG_LEVEL_WARNING_COLOR "WARN" RESET_ANSI "]   - " LOG_LEVEL_WARNING_MSG_COLOR
# define LOG_LEVEL_PREFIX_ERROR				" [" LOG_LEVEL_ERROR_COLOR "ERROR" RESET_ANSI "]  - " LOG_LEVEL_ERROR_MSG_COLOR
# define LOG_LEVEL_PREFIX_CRITICAL			" [" LOG_LEVEL_CRITICAL_COLOR "CRITIC" RESET_ANSI "] - " LOG_LEVEL_CRITICAL_MSG_COLOR
# define LOG_LEVEL_PREFIX_DEBUG_NOCOLOR		" [DEBUG]  - "
# define LOG_LEVEL_PREFIX_INFO_NOCOLOR		" [INFO]   - "
# define LOG_LEVEL_PREFIX_WARNING_NOCOLOR	" [WARN]   - "
# define LOG_LEVEL_PREFIX_ERROR_NOCOLOR		" [ERROR]  - "
# define LOG_LEVEL_PREFIX_CRITICAL_NOCOLOR	" [CRITIC] - "


// define timestamp color here
# define LOG_TIMESTAMP_COLOR    "\033[38;2;100;100;120m"

# define LOG_LEVEL_NAME_MAXSIZE		8
# define LOG_CATEGORY_NAME_MAXSIZE	7

# define LOG_STDOUT_MAGIC 		"STDOUT"
# define LOG_STDERR_MAGIC 		"STDERR"

# define LOG_CATEGORY_DEFAULT	"DEFAULT"
# define LOG_CATEGORY_INIT		"INIT"
# define LOG_CATEGORY_SIGNAL	"SIGNAL"
# define LOG_CATEGORY_NETWORK	"NETWORKING"
# define LOG_CATEGORY_CONFIG	"CONFIG"
# define LOG_CATEGORY_JOB		"JOB"
# define LOG_CATEGORY_JM		"JOBMNGR"
# define LOG_CATEGORY_THREAD	"THREAD"
# define LOG_CATEGORY_LOGGER	"LOGGER"
# define LOG_CATEGORY_STDOUT	LOG_STDOUT_MAGIC
# define LOG_CATEGORY_STDERR	LOG_STDERR_MAGIC

/* If enabled create automatically new categories without using default category */
//TODO Change LOG_CATEGORY_AUTO with a variable
# define LOG_CATEGORY_AUTO		false

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
	public:
		struct log_category
		{
			std::string		name;
			std::string		filename;
		};

		struct log_destination
		{
			log_destination(bool init_isfile, uint init_nbref, std::ostream & init_out) : 
				is_file(init_isfile), nb_references(init_nbref), output(init_out)
			{}
			log_destination(const Tintin_reporter::log_destination & src) : 
				is_file(src.is_file), nb_references(src.nb_references), output(src.output)
			{}

			bool	is_file;
			uint	nb_references;
			std::ostream & output;
		};

	private:
		Tintin_reporter(const std::string & defaultFile);
		~Tintin_reporter();

		static Tintin_reporter*			_logManager;


		std::map<std::string, log_destination>	_opened_files;
		std::map<std::string, log_category>		_categories;
		std::string								_defaultCategory;
		uint									_timestamp_format;
		uint									_color_enabled;

	protected:

		const std::string	getLogHead(const std::string & category) const;
		void				log_critical(const std::string & category, const std::string & message);
		void				log_error(const std::string & category, const std::string & message);
		void				log_warning(const std::string & category, const std::string & message);
		void				log_info(const std::string & category, const std::string & message);
		void				log_debug(const std::string & category, const std::string & message);
		void				addDefaultCategory(const std::string & outfile);

		Tintin_reporter::log_destination	newLogDestination( void ) const;
		Tintin_reporter::log_destination	newLogDestinationStdout( void ) const;
		Tintin_reporter::log_destination	newLogDestinationStderr( void ) const;

	public:

		Tintin_reporter( Tintin_reporter & src )							= delete;
		Tintin_reporter( Tintin_reporter const & src )						= delete;
		Tintin_reporter &		operator=( Tintin_reporter const & rhs )	= delete;

		class emptyInitException : public std::exception
		{
			public:

				virtual const char* what() const noexcept
				{
					return "No default filename provided for logger.";
				}
		};

		class defaultFileException : public std::exception
		{
			private:
				std::string _filename;

			public:
				defaultFileException(const std::string filename = "")
				{
					this->_filename = filename;
				}

				virtual const char* what() const noexcept
				{
					return std::string("Impossible to open default file '" + this->_filename + "'.").c_str();
				}
		};



		static Tintin_reporter& getLogManager(const std::string & defaultFile = "") {
			if (_logManager == nullptr) {
				if (defaultFile.empty())
					throw Tintin_reporter::emptyInitException();
				_logManager = new Tintin_reporter(defaultFile);
			}
			return *_logManager;
		}

		static void				destroyLogManager( void )
		{
			if (_logManager != nullptr)
			{
				delete _logManager;
				_logManager = nullptr;
			}
		}

		void				setColor( bool enabled );
		bool				getColor( void ) const;
		const std::string&	getDefaultCategory( void ) const;
		uint				getNbCategories( void ) const;
		uint				getNbOpenfiles( void ) const;
		std::string			getTimestamp( void ) const;
		std::string			timeToString( const struct timeval & time ) const;
		std::string			formatCategoryName( const std::string & categoryName) const;
		int					addCategory(const std::string & str, const std::string & outfile = "");
		void				log(uint level, const std::string & category, const std::string & message);

		const std::map<std::string, log_category >::const_iterator			getCategoriesBegin( void ) const;
		const std::map<std::string, log_category >::const_iterator			getCategoriesEnd( void ) const;
		const std::map<std::string, log_destination>::const_iterator		getOpenFilesBegin( void ) const;
		const std::map<std::string, log_destination>::const_iterator		getOpenFilesEnd( void ) const;

};

std::ostream &			operator<<( std::ostream & o, Tintin_reporter const & i );

#endif /* ************************************************* TINTIN_REPORTER_H */