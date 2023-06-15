#ifndef TM_UTILS_HPP
# define TM_UTILS_HPP

# include <unistd.h>
# include <errno.h>
# include <fcntl.h>
# include <cstdlib>
# include <string>
# include <iostream>

bool		isRunningRootPermissions( void );
int			takeLockFile( const std::string & path );
void		freeLockFile( const std::string & path );

#endif //TM_UTILS_HPP
