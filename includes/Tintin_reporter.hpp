#ifndef TINTIN_REPORTER_HPP
# define TINTIN_REPORTER_HPP

# include <iomanip>
# include <iostream>
# include <ostream>
# include <fstream>
# include <string>
# include <queue>
# include <map>
# include <chrono>

# include <sys/time.h>
# include <ntos.hpp>

class Tintin_reporter;

# include "tm_values.hpp"
# include "AThread.hpp"

/* If enabled create automatically new categories without using default category */
//TODO Change LOG_CATEGORY_AUTO with a variable
# define LOG_CATEGORY_AUTO		false


# define LOG_DEBUG(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::log(LOG_LEVEL_DEBUG, category, std::string(_s.str())); }
# define LOG_INFO(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::log(LOG_LEVEL_INFO, category, std::string(_s.str())); }
# define LOG_WARN(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::log(LOG_LEVEL_WARNING, category, std::string(_s.str())); }
# define LOG_ERROR(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::log(LOG_LEVEL_ERROR, category, std::string(_s.str())); }
# define LOG_CRITICAL(category, log_message) \
	{ std::ostringstream _s; _s << log_message; Tintin_reporter::log(LOG_LEVEL_CRITICAL, category, std::string(_s.str())); }


class Tintin_reporter : public virtual AThread<Tintin_reporter>
{
	using CronType = std::chrono::system_clock;

	public:
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

		struct log_message
		{
			log_message(uint arg_level, const char* arg_category, const char* arg_thread, CronType::time_point  arg_timestamp, const std::string & arg_message) : 
				level(arg_level), category(arg_category), thread(arg_thread), timestamp(arg_timestamp), message(arg_message)
			{}
			log_message(uint arg_level, const char* arg_category, const char* arg_thread, CronType::time_point arg_timestamp, const char * arg_message) : 
				level(arg_level), category(arg_category), thread(arg_thread), timestamp(arg_timestamp),message(arg_message)
			{}
			log_message(const Tintin_reporter::log_message & src) : 
				level(src.level), category(src.category), thread(src.thread), timestamp(src.timestamp), message(src.message)
			{}

			uint						level;
			const char*					category;
			const char*					thread; //TODO implement
			CronType::time_point		timestamp;
			std::string					message;
		};

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

			bool			is_file;
			uint			nb_references;
			std::ostream &	output;
		};

	private:
		Tintin_reporter(const std::string & defaultFile = "default.log");
		~Tintin_reporter();

		std::map<std::string, log_destination>	_opened_files;
		std::map<std::string, log_category>		_categories;
		std::string								_defaultCategory;
		uint									_timestamp_format;
		CronType::time_point					_starttime;
		uint									_color_enabled;
		static std::condition_variable			_hasUpdate;
		static std::mutex						_mutexUpdate;
		static std::queue<log_message>			_messageQueue;

	protected:

		void						_log(const log_message & message);
		void						addDefaultCategory(const std::string & outfile);

		Tintin_reporter::log_destination	newLogDestination( void ) const;
		Tintin_reporter::log_destination	newLogDestinationStdout( void ) const;
		Tintin_reporter::log_destination	newLogDestinationStderr( void ) const;

	public:

		/* 
			Return number of miliseconds elapsed since epoch Unix
		*/
		static CronType::time_point		getTimestamp( void )
		{
			return CronType::now();
		}


		static	void			log(uint level, const char* category, const std::string & message)
		{
			{
				std::lock_guard<std::mutex> lock(Tintin_reporter::_mutexUpdate);
				Tintin_reporter::_messageQueue.push(log_message(level, category, "", Tintin_reporter::getTimestamp(), message ));
			}
			Tintin_reporter::_hasUpdate.notify_all();
		}

		void				operator()( void );
		void				stop( void );

		void				setColor( bool enabled );
		bool				getColor( void ) const;

		uint				getNbCategories( void ) const;
		uint				getNbOpenfiles( void ) const;
		std::string			getTimestampStr(const CronType::time_point & timestamp) const;
		const std::string&	getDefaultCategory( void ) const;
		std::string			formatCategoryName( const std::string & categoryName) const;
		int					addCategory(const std::string & str, const std::string & outfile = "");

		const std::map<std::string, log_category >::const_iterator			getCategoriesBegin( void ) const;
		const std::map<std::string, log_category >::const_iterator			getCategoriesEnd( void ) const;
		const std::map<std::string, log_destination>::const_iterator		getOpenFilesBegin( void ) const;
		const std::map<std::string, log_destination>::const_iterator		getOpenFilesEnd( void ) const;

	friend class AThread<Tintin_reporter>;
};


std::ostream &			operator<<( std::ostream & o, Tintin_reporter const & i );

#endif /* ************************************************* TINTIN_REPORTER_H */