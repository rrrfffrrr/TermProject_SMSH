/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "type.h"
#include "history.h"
#include "command.h"
#include "errstr.h"

// string
const char const PrefixFormat[256] = "\033[1;32m%s\033[0m:\033[1;94m%s\033[0m$ ";
char UserName[MAX_USERNAME_LENGTH] = "SMSH";

int main() {
	char command[MAX_COMMAND_LENGTH];
	int commandSize;
	char cwd[MAX_CWD_LENGTH];
	getcwd(cwd, MAX_CWD_LENGTH);
	getlogin_r(UserName, MAX_USERNAME_LENGTH);
	
	while(true) {
		{ // get input
			commandSize = snprintf(command, MAX_COMMAND_LENGTH, PrefixFormat, UserName, cwd);
			write(STDOUT_FILENO, command, commandSize);
			commandSize = read(STDIN_FILENO, command, MAX_COMMAND_LENGTH);
			command[commandSize-1] = '\0'; // replace \n to \0
		}
		{ // perform history
			size_t hid;
			if (ParseCommandToAddHistory(command, MAX_COMMAND_LENGTH, &hid) == false) {
				commandSize = snprintf(command, MAX_COMMAND_LENGTH, ERREVENT_NOFOUND, hid);
				write(STDOUT_FILENO, command, commandSize);
				goto ENDLOOP;
			}
			AddHistory(command);
		}
		if (strlen(command) < 1) // no command
			goto ENDLOOP;
		
		{ // parse command
			
		}
		
ENDLOOP:;
	}
	return 0;
}