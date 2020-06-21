#include "redirect.h"
#include <unistd.h>
#include <fcntl.h>

bool NoClobber = true;

void SetNoclobber(bool enable) {
	NoClobber = enable;
}
bool GetNoclobber() {
	return NoClobber;
}

bool RedirectInput(char* file) {
	int fd = open(file, O_RDONLY);
	if (fd == -1) {
		return false;
	}
	close(0);
	dup(fd);
	close(fd);
	return true;
}

bool RedirectOutput(char* file, int mode) {
	int flags = O_CREAT|O_WRONLY;
	if (mode == RDO_Append)
		flags |= O_APPEND;
	else if (mode == RDO_ForceOverride)
		flags |= O_TRUNC;
	else if (NoClobber == false)
		flags |= O_TRUNC;

	int fd = open(file, flags, 0666);
	if (fd == -1) {
		return false;
	}
	close(1);
	dup(fd);
	close(fd);
	return true;
}

// Deprecated
bool RedirectError(char* file) {
	return true;
}