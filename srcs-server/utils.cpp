# include "utils.hpp"

bool		isRunningRootPermissions( void )
{
	return (::geteuid() == 0);
}

int			takeLockFile( const std::string & path )
{
	int fd = 0;

	if ((fd = open(path.c_str(), O_CREAT|O_EXCL )) == -1) {
		if (errno == EEXIST)
		{
			std::cerr << "The lock file already exist. Impossible to start a second instance of this program" << std::endl;
		}
		else
		{
			std::cerr << "Failed to open lock file '" << path << "' : " << strerror(errno) << std::endl;
		}
		return EXIT_FAILURE;
	}
	std::cerr << "Lock file '" << path << "' successfuly taken." << std::endl;
	close(fd);
	return EXIT_SUCCESS;
}


void		freeLockFile( const std::string & path ) 
{
	std::remove(path.c_str());
}