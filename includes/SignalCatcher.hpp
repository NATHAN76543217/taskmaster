#ifndef SIGNALCATCHER_HPP
# define SIGNALCATCHER_HPP

class SignalCatcher;

# include "AThread.hpp"
# include "Job.hpp"
# include "Tintin_reporter.hpp"
# include "Taskmaster.hpp"

class SignalCatcher : public virtual AThread<SignalCatcher>
{
	private:
		/* Private variables */
		sigset_t		_sigSet;
		int				_sigNum;

	protected:
		/* Constructor */ 
		SignalCatcher(const std::string & name = "SignalCatcher") : 
			AThread<SignalCatcher>(*this, name),
			_sigSet(0),
			_sigNum(0)
		{
			LOG_DEBUG(LOG_CATEGORY_THREAD, "SignalCatcher - Constructor" )
		};

	public:
		/* Logic loop for execution */
		void	operator()( void );

	public:

		/* Public methods */
		static SignalCatcher&	GetInstance( const std::string & name = "SignalCatcher" )
		{
			return AThread<SignalCatcher>::GetInstance(name);
		}


	friend class AThread<SignalCatcher>;
};

std::ostream &			operator<<( std::ostream & o, SignalCatcher const & i );

#endif /* *************************************************** SIGNALCATCHER_H */