# ifndef TM_VALUES_HPP
# define TM_VALUES_HPP

# include <signal.h>

enum job_policy {
	never,
	always,
	onfailure
};

enum job_status {
	not_started,
	starting,
	running,
	stopped,
	suspended,
	incomplete,
	exited,
	terminated
};

# define TM_DEF_MAX_CONNECTIONS 3
# define TM_DEF_CONFIGPATH "./config_test.yaml"
// # define TM_DEF_CONFIGPATH "./config_template.yaml"
# define TM_DEF_LOCKPATH "./test.lock"
// # define TM_DEF_LOCKPATH "/var/lock/taskmaster.lock"
# define TM_DEF_LOGCOLOR false
# define TM_DEF_LOGPATH "/var/log/taskmaster/taskmaster.log"
# define TM_DEF_PWD "/var/log/taskmaster/taskmaster.log"
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
# define LOG_CATEGORY_MAIN		"MAIN"
# define LOG_CATEGORY_SIGNAL	"SIGNAL"
# define LOG_CATEGORY_NETWORK	"NETWORKING"
# define LOG_CATEGORY_CONFIG	"CONFIG"
# define LOG_CATEGORY_JOB		"JOB"
# define LOG_CATEGORY_JM		"JOBMNGR"
# define LOG_CATEGORY_THREAD	"THREAD"
# define LOG_CATEGORY_LOGGER	"LOGGER"
# define LOG_CATEGORY_STDOUT	LOG_STDOUT_MAGIC
# define LOG_CATEGORY_STDERR	LOG_STDERR_MAGIC


# define LOG_TIMESTAMP_DEFAULT 0

# define THREADTAG_SIGNALCATCHER	"[SignalCatcher] "
# define THREADTAG_LOGGER			"[Logger] "
# define THREADTAG_JOBMANAGER		"[JobManager] "
# define THREADTAG_MAIN				"[Main] "
# define THREADTAG_TASKMASTER		"[Taskmaster] "


# endif //TMP_VALUES_HPP