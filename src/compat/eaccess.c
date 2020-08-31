#include <unistd.h>

int
eaccess(const char *path, int mode)
{
	return access(path, mode);
}
