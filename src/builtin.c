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
	char cmd[SIZEOFCOMMAND];
	for(int i = 0; i < SIZEOFCOMMAND; ++i) {
		switch(command[i]) {
			case ' ': case '	': case ';': case '|': case '&': case '<': case '>': case '\0':
				cmd[i] = '\0';
				i = SIZEOFCOMMAND;
			break;
			default:
				cmd[i] = command[i];
			break;
		}
	}

	static int size = sizeof(BuiltinCommands)/sizeof(BuiltinCommands[0]);
	for(int i = 0; i < size; ++i) {
		if (strcmp(BuiltinCommands[i], cmd) == 0) {
			return true;
		}
	}

	return false;
}

// hardcoded cause there's no hash map.
ssize_t RunBuiltinCommand(char* command) {
	char cmd[COUNTOFPARAM][SIZEOFCOMMAND];
	int iParam = 0;
	int iCursor = 0;
	int bIdentifier = 0;
	for(int i = 0; i < SIZEOFCOMMAND; ++i) {
		switch(command[i]) {
			case ';': case '|': case '&': case '<': case '>': case '\0':
				i = SIZEOFCOMMAND;
			break;
			case ' ': case '	':
				if (bIdentifier == 0)
					break;
				cmd[iParam][iCursor] = '\0';
				iParam++;
				iCursor = 0;
				bIdentifier = 0;
				if (iParam == COUNTOFPARAM)
					i = SIZEOFCOMMAND;
			break;
			default:
				bIdentifier = 1;
				cmd[iParam][iCursor++] = command[i];
			break;
		}
	}

	if (strcmp(cmd[0], BuiltinCommands[0]) == 0) {
		ShowHistory();
		return 0;
	} else if (strcmp(cmd[0], BuiltinCommands[1]) == 0) {
		if (chdir(cmd[1]) == -1) {
			printf(ERRCMD_CD_NODIR, cmd[1]);
		}
		return 0;
	} else if (strcmp(cmd[0], BuiltinCommands[2]) == 0) {
		char* path = getcwd(NULL, 0);
		if (path == NULL)
			return -1;
		printf("%s\n", path);
		free(path);
		return 0;
	} else if (strcmp(cmd[0], BuiltinCommands[3]) == 0) {
		exit(0);
	}
	return -1;
}