#ifndef ATHREAD_HPP
# define ATHREAD_HPP

# include <iostream>
# include <string>
# include <thread>
# include <atomic>
# include <mutex>

# include "tm_values.hpp"

template <typename T> 
class AThread
{
	private:
		std::thread		_thread;
		std::string		_name;

	public:
		static std::mutex			static_mutex;
	protected:
		bool						_running;
		std::mutex					_stop_mutex; //to stop thread REVIEW change to static as start variable or not? 
		std::condition_variable		_ready; //to start thread
		std::condition_variable		_stop; //to stop thread

		/* unique instance */ 
		static std::atomic<T*>		instance_;

		AThread( T & super, const std::string & name) : 
		_thread(&T::operator(), &super),
		_name(name),
		_running(true),
		_stop_mutex(),
		_ready(),
		_stop()
		{
		}

		// AThread( const std::string & name) : 
		// _thread(),
		// _name(name),
		// _running(true)
		// {
		// 	LOG_DEBUG(LOG_CATEGORY_THREAD, "AThread - Constructor <" << this->_name << "> - nothread.")
		// }

	public:

		static T&		GetInstance( const std::string & name = "THREAD_NAME" );
		static void		DestroyInstance( void );
		virtual			~AThread() {}

		/* Should not be cloneable. */
		AThread(AThread &other)			= delete;
		AThread(const AThread &other)	= delete;
	
		/* should not be assignable. */
		void	operator=(const AThread &) = delete;

		virtual void			operator()( void )
		{
			std::cerr << "CRITICALL" << std::endl;
			std::cerr << std::flush;
		}


		virtual	void			start( void )
		{
			{
				std::lock_guard<std::mutex> lock(AThread<T>::static_mutex);
				this->_running = true;
			}
			this->_ready.notify_all();
		}

		virtual	void			waitEnd( void )
		{
			std::unique_lock<std::mutex> lock(this->_stop_mutex);
			this->_stop.wait(lock);
			return ;
		}

		virtual	void			stop( void )
		{
			{
				std::lock_guard<std::mutex> lock(AThread<T>::static_mutex);
				this->_running = false;
			}
			this->_stop.notify_all();
		}

		const std::string &getName( void ) const
		{
			return this->_name;
		}

		std::thread::id		getThreadID( void ) const
		{
			return this->_thread.get_id();
		}
};

	/* unique instance initialization*/ 

	template<typename T>
		std::atomic<T*>	AThread<T>::instance_{nullptr};

	template<typename T>
		std::mutex		AThread<T>::static_mutex;

	template<typename T>
		T&		AThread<T>::GetInstance( const std::string & name )
		{
			if(instance_.load() == nullptr)
				instance_.store(new T(name));
			return *(instance_.load());
		}


	template<typename T>
		void		AThread<T>::DestroyInstance( void )
		{
			if(instance_.load() == nullptr)
				return ;
			instance_.load()->stop();
			instance_.load()->_thread.join();
			delete instance_.load();
			instance_.store(nullptr);
		}


// std::ostream &			operator<<( std::ostream & o, AThread const & i );

#endif /* ********************************************************* ATHREAD_H */