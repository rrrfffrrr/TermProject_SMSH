/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include "errstr.h"
#include "builtin.h"
#include "string.h"
#include "history.h"
#include "redirect.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define COUNTOFPARAM 16
#define SIZEOFCOMMAND 32

// Use array rather than hash table for reduce coding time. hardcoded few commands > hash table
static char BuiltinCommands[][SIZEOFCOMMAND] = {
	"history",
	"cd",
	"pwd",
	"exit",
	"set",
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
ssize_t RunBuiltinCommand(char** argv) {
	if (strcmp(argv[0], BuiltinCommands[0]) == 0) {
		ShowHistory();
		return 0;
	} else if (strcmp(argv[0], BuiltinCommands[1]) == 0) {
		if (chdir(argv[1]) == -1) {
			switch(errno) {
			case ENOENT:
				printf(ERRCMD_CD_NODIR, argv[1]);
			break;
			default:
				printf(ERRCMD_CD_DEFAULT, argv[1]);
			break;
			}
		}
		return 0;
	} else if (strcmp(argv[0], BuiltinCommands[2]) == 0) {
		char* path = getcwd(NULL, 0);
		if (path == NULL)
			return -1;
		printf("%s\n", path);
		free(path);
		return 0;
	} else if (strcmp(argv[0], BuiltinCommands[3]) == 0) {
		exit(0);
	} else if (strcmp(argv[0], BuiltinCommands[4]) == 0) {
		if (strcmp(argv[1], "+C") == 0)
			SetNoclobber(true);
		else if (strcmp(argv[1], "-C") == 0)
			SetNoclobber(false);
		else if (strcmp(argv[1], "+o"))
			if (strcmp(argv[2], "noclobber") == 0)
				SetNoclobber(true);
		else if (strcmp(argv[1], "-o"))
			if (strcmp(argv[2], "noclobber") == 0)
				SetNoclobber(false);
		return 0;
	}
	return -1;
}