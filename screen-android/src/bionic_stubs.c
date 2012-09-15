#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define	DEV	"/dev/pts/"	/* device directory */
#define	DEVLEN	sizeof(DEV)-1	/* sizeof includes null at end */

char *crypt(char *key, char *salt) { char * ret ; ret = malloc(14); strcpy (ret, "ABCDEFGHIJKLM"); return ret; }
char *getpass(char *key) { return 0; }

#define _POSIX_PATH_MAX 256

char *
ttyname(int fd)
{
	struct stat		fdstat, devstat;
	DIR				*dp;
	struct dirent	*dirp;
	static char		pathname[_POSIX_PATH_MAX + 1];
	char			*rval;

	if (isatty(fd) == 0)
		return(NULL);
	if (fstat(fd, &fdstat) < 0)
		return(NULL);
	if (S_ISCHR(fdstat.st_mode) == 0)
		return(NULL);

	strcpy(pathname, DEV);
	if ( (dp = opendir(DEV)) == NULL)
		return(NULL);
	rval = NULL;
	while ( (dirp = readdir(dp)) != NULL) {
		if (dirp->d_ino != fdstat.st_ino)
			continue;		/* fast test to skip most entries */

		strncpy(pathname + DEVLEN, dirp->d_name, _POSIX_PATH_MAX - DEVLEN);
		if (stat(pathname, &devstat) < 0)
			continue;
		if (devstat.st_ino == fdstat.st_ino &&
			devstat.st_dev == fdstat.st_dev) {	/* found a match */
				rval = pathname;
				break;
		}
	}
	closedir(dp);
	return(rval);
}

