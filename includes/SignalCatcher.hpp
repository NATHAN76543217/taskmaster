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

	protected:
		/* Constructor */ 
		SignalCatcher(const std::string & name = "SignalCatcher") : 
			AThread<SignalCatcher>(*this, name),
			_sigSet(0)
		{
		};

	public:
		/* Logic loop for execution */
		void	operator()( void );

	public:

		/* Public methods */

	friend class AThread<SignalCatcher>;
};

std::ostream &			operator<<( std::ostream & o, SignalCatcher const & i );

#endif /* *************************************************** SIGNALCATCHER_H */