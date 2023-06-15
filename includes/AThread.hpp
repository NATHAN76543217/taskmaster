#ifndef ATHREAD_HPP
# define ATHREAD_HPP

# include <iostream>
# include <string>
# include <thread>
# include <atomic>
# include <mutex>
# include "Tintin_reporter.hpp"

template <typename T> 
class AThread
{
	private:
		std::thread		_thread;
		std::string		_name;


		void			_stopThread( void )
		{
			LOG_DEBUG(LOG_CATEGORY_THREAD, "Joining thread <" << this->_name << ">.")
			this->_thread.join();
			LOG_INFO(LOG_CATEGORY_THREAD, "Thread <" << this->_name<< "> joined")
		}


	protected:

		bool			_running;
		std::mutex		_internal_mutex;

		/* unique instance */ 
		static std::atomic<T*>	instance_;

		AThread( T & super, const std::string & name) : 
		_thread(&T::operator(), &super),
		_name(name),
		_running(true)
		{
			LOG_DEBUG(LOG_CATEGORY_THREAD, "AThread - Constructor <" << this->_name << ">.")
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
			LOG_CRITICAL(LOG_CATEGORY_THREAD, "AThread - Operator()")	
		}



		virtual	void			stop( void )
		{
			std::lock_guard<std::mutex> lk(this->_internal_mutex);
			this->_running = false;
		}

		const std::string &getName( void ) const
		{
			return this->_name;
		}


};
	/* unique instance initialization*/ 
	template<typename T>
		std::atomic<T*>	AThread<T>::instance_{nullptr};

	template<typename T>
		T&		AThread<T>::GetInstance( const std::string & name )
		{
			if(instance_.load() == nullptr)
			{
				LOG_INFO(LOG_CATEGORY_STDOUT, "Create a <" << name << "> thread : " << typeid(T).name() )
				instance_.store(new T(name));
			}
			return *(instance_.load());
		}

	template<typename T>
		void		AThread<T>::DestroyInstance( void )
		{
			if(instance_.load() == nullptr)
				return ;
			LOG_INFO(LOG_CATEGORY_THREAD, "Destroying <" << instance_.load()->getName() << "> on thread : " << std::this_thread::get_id())
			instance_.load()->stop();
			instance_.load()->_stopThread();
			delete instance_.load();
			instance_.store(nullptr);

		}


// template <typename T>
// constexpr std::atomic<T*> AThread<T>::instance_(nullptr);

// std::ostream &			operator<<( std::ostream & o, AThread const & i );

#endif /* ********************************************************* ATHREAD_H */