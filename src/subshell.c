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

pid_t RunSubshellInstance(char* command, bool isBackground, char* input, char* output, int outputMode) {
	pid_t pid = vfork();
	if (pid < 0) {
		printf(ERRFORK);
		return -1;
	} else if (pid == 0) {
		// pipe here
		if (input != NULL) {
			RedirectInput(input);
		}
		if (output != NULL) {
			RedirectOutput(output, outputMode);
		}

		RunCommand(command);
		exit(0);
	} else {
		if (isBackground == false)
			waitpid(pid);
		return pid;
	}
}