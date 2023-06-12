# ifndef TM_VALUES_HPP
# define TM_VALUES_HPP

# include <signal.h>

enum job_policy {
	never,
	always,
	onfailure
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


# endif //TMP_VALUES_HPP