/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include <string.h>
#include <unistd.h>

#include "type.h"
#include "subshell.h"
#include "command.h"

pid_t RunSubshellInstance(char* command) {
	pid_t pid = vfork();
	if (pid < 0) {
		printf("Fail to fork.\n");
		return -1;
	} else if (pid == 0) {
		RunCommand(command);
		exit(0);
	} else {
		return pid;
	}
}