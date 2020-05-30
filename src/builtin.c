/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include "errstr.h"
#include "builtin.h"
#include "string.h"
#include "history.h"
#include <unistd.h>

#define COUNTOFPARAM 16
#define SIZEOFCOMMAND 32

// Use array rather than hash table for reduce coding time. hardcoded few commands > hash table
static char BuiltinCommands[][SIZEOFCOMMAND] = {
	"history",
	"cd",
	"pwd",
	"exit"
};

bool IsBuiltinCommand(char* command) {
	static int size = sizeof(BuiltinCommands)/sizeof(BuiltinCommands[0]);
	for(int i = 0; i < size; ++i) {
		if (strcmp(BuiltinCommands[i], command) == 0) {
			return true;
		}
	}

	return false;
}

// hardcoded cause there's no hash map.
ssize_t RunBUiltinCommand(char* cmd, char** argv) {
	if (strcmp(cmd, BuiltinCommands[0]) == 0) {
		ShowHistory();
		return 0;
	} else if (strcmp(cmd, BuiltinCommands[1]) == 0) {
		if (chdir(argv[0]) == -1) {
			printf(ERRCMD_CD_NODIR, cmd[1]);
		}
		return 0;
	} else if (strcmp(cmd, BuiltinCommands[2]) == 0) {
		char* path = getcwd(NULL, 0);
		if (path == NULL)
			return -1;
		printf("%s\n", path);
		free(path);
		return 0;
	} else if (strcmp(cmd, BuiltinCommands[3]) == 0) {
		exit(0);
	}
	return -1;
}