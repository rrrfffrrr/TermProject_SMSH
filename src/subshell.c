/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include <string.h>
#include <unistd.h>

#include "errstr.h"
#include "type.h"
#include "subshell.h"
#include "command.h"
#include "redirect.h"
#include <stdlib.h>
#include <sys/wait.h>

pid_t RunSubshellInstance(char* command, bool isBackground, int* pipeReceiver, int* pipeSender) {
	pid_t pid = fork();
	if (pid < 0) {
		write(1, ERRFORK, strlen(ERRFORK));
		return -1;
	} else if (pid == 0) {

		RunCommand(command);
		exit(0);
	} else {
		if (isBackground == false)
			waitpid(pid, 0, 0);
		return pid;
	}
}