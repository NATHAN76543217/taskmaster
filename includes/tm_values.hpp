# ifndef TM_VALUES_HPP
# define TM_VALUES_HPP

# include <signal.h>

// #ifdef WINDOWS
// # include <direct.h>
// # define GetCurrentDir _getcwd
// #else
// # include <unistd.h>
// # define GetCurrentDir getcwd
// #endif

// std::string get_current_dir() {
//    char buff[FILENAME_MAX]; //create string buffer to hold path
//    GetCurrentDir( buff, FILENAME_MAX );
//    return std::string(buff);
// }

enum job_policy {
	never,
	always,
	onfailure
};

enum job_status {
	not_started,	//0 All processes are not started yet
	starting,		//1 All processes should start
	running,		//2 All processes are running
	stopped,		//3 All processes stoped by a SIGSTOP
	suspended,		//4 All processes stopped by a SIGTSTP
	incomplete,		//5 All processes should be running but at least one is not
	exited,			//6 All processes terminated and at least one with a bad exit value
	terminated,		//7 All processes of job terminated with success
	killed			//8 All processes terminated and at least one by a signal
};

//TODO create a class/struct with a child_status and a timestamp since the last status changes
enum child_status {
	child_not_started,	// The process is not started yet
	child_starting,			// The process is starting
	child_running,		// The process is running
	child_stopped,		// The process is stopped by a SIGSTOP
	child_suspended,	// The process is stopped by a SIGTSTP
	child_terminating,	// To have the same number of values than job_status
	child_exited,		// The process terminate with a bad exit value
	child_terminated,	// The process terminate with a success exit value
	child_killed		// The process is terminated by a signal
};

# define TM_DEF_CONFIGPATH	"/Users/sebastienlecaille/programmation/101/mygithub/taskmaster/config_test.yaml"
// # define TM_DEF_CONFIGPATH "./config_template.yaml"
# define TM_DEF_LOCKPATH	"/var/lock/matt_daemon.lock"

/* Server default values */
# define TM_DEF_MAX_CONNECTIONS 3
# define TM_DEF_LOGCOLOR	false
# define TM_DEF_LOGDIR		"/var/log/matt_daemon/"
# define TM_DEF_PWD			"/var"

/* Programs default values */

# define TM_DEF_NBPROCS		1
# define TM_DEF_WORKDIR 	""
# define TM_DEF_UMASK		022
# define TM_DEF_AUTOSTART	false
# define TM_DEF_RESTARTPOLICY never
# define TM_DEF_NBRETRYMAX	0
# define TM_DEF_EXITCODE	0
# define TM_DEF_STARTTIME	1
# define TM_DEF_STOPTIME	1

# define TM_DEF_STOPSIGNAL	SIGHUP

# define TM_DEF_STDOUT		"allow"
# define TM_DEF_STDERR		"allow"

# define TM_FIELD_SERVER		"server"
# define TM_FIELD_LOGCOLOR		"logcolor"
# define TM_FIELD_LOGDIR		"logdir"
# define TM_FIELD_PROGRAMS		"programs"
# define TM_FIELD_CMD			"cmd"
# define TM_FIELD_NBPROCS		"nbprocs"
# define TM_FIELD_UMASK			"umask"
# define TM_FIELD_WORKINGDIR	"workingdir"
# define TM_FIELD_AUTOSTART		"autostart"
# define TM_FIELD_RESTARTPOLICY	"restartpolicy"
# define TM_FIELD_NBRETRYMAX	"nbretrymax"
# define TM_FIELD_EXITCODES		"exitcodes"
# define TM_FIELD_STARTTIME		"starttime"
# define TM_FIELD_STOPTIME		"stoptime"
# define TM_FIELD_STOPSIGNAL	"stopsignal"
# define TM_FIELD_STDOUT		"stdout"
# define TM_FIELD_STDERR		"stderr"
# define TM_DEF_ENVFROMPARENT	"envfromparent"
# define TM_FIELD_ENV			"env"

//TODO define all def values


/* LOGGER */


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
# define LOG_LEVEL_PREFIX_DEBUG				"[" LOG_LEVEL_DEBUG_COLOR "DEBUG" RESET_ANSI "] " LOG_LEVEL_DEBUG_MSG_COLOR
# define LOG_LEVEL_PREFIX_INFO				"[" LOG_LEVEL_INFO_COLOR "INFO" RESET_ANSI "]  " LOG_LEVEL_INFO_MSG_COLOR
# define LOG_LEVEL_PREFIX_WARNING			"[" LOG_LEVEL_WARNING_COLOR "WARN" RESET_ANSI "]  " LOG_LEVEL_WARNING_MSG_COLOR
# define LOG_LEVEL_PREFIX_ERROR				"[" LOG_LEVEL_ERROR_COLOR "ERROR" RESET_ANSI "] " LOG_LEVEL_ERROR_MSG_COLOR
# define LOG_LEVEL_PREFIX_CRITICAL			"[" LOG_LEVEL_CRITICAL_COLOR "CRITIC" RESET_ANSI "]" LOG_LEVEL_CRITICAL_MSG_COLOR
# define LOG_LEVEL_PREFIX_DEBUG_RAW			"[DEBUG] "
# define LOG_LEVEL_PREFIX_INFO_RAW			"[INFO]  "
# define LOG_LEVEL_PREFIX_WARNING_RAW		"[WARN]  "
# define LOG_LEVEL_PREFIX_ERROR_RAW			"[ERROR] "
# define LOG_LEVEL_PREFIX_CRITICAL_RAW		"[CRITIC]"

// define timestamp color here
# define LOG_TIMESTAMP_COLOR    "\033[38;2;100;100;120m"

# define LOG_LEVEL_NAME_MAXSIZE		8
# define LOG_CATEGORY_NAME_MAXSIZE	7
# define LOG_THREAD_NAME_MAXSIZE	16

# define LOG_STDOUT_MAGIC 		"STDOUT" // "filename" for STD's
# define LOG_STDERR_MAGIC 		"STDERR"

# define LOG_CATEGORY_DEFAULT	"DEFAULT"
# define LOG_CATEGORY_INIT		"INIT"
# define LOG_CATEGORY_MAIN		"MAIN"
# define LOG_CATEGORY_SIGNAL	"SIGNAL"
# define LOG_CATEGORY_NETWORK	"NETWORKING"
# define LOG_CATEGORY_CONFIG	"CONFIG"
# define LOG_CATEGORY_JOB		"JOB"
# define LOG_CATEGORY_JM		"JOBMNGR"
# define LOG_CATEGORY_LOGGER	"LOGGER"
# define LOG_CATEGORY_STDOUT	LOG_STDOUT_MAGIC// category name for STD's
# define LOG_CATEGORY_STDERR	LOG_STDERR_MAGIC


# define LOG_TIMESTAMP_CURRENT	0
# define LOG_TIMESTAMP_ELAPSED	1
# define LOG_TIMESTAMP_UTC		2
# define LOG_TIMESTAMP_SUMMERTIME 1


# define THREADTAG_SIGNALCATCHER "SignalCatcher"
# define THREADTAG_LOGGER		 "Logger"
# define THREADTAG_JOBMANAGER	 "JobManager"
# define THREADTAG_MAIN			 "Main"
# define THREADTAG_TASKMASTER	 "Taskmaster"


# endif //TMP_VALUES_HPP