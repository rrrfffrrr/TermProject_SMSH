/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include "builtin.h"
#include "string.h"
#include "history.h"

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
ssize_t RunBuiltinCommand(char* command) {
	if (strcmp(command, BuiltinCommands[0]) == 0) {
		ShowHistory();
		return 0;
	} else if (strcmp(command, BuiltinCommands[1]) == 0) {
		// cd here		
		return 0;
	} else if (strcmp(command, BuiltinCommands[2]) == 0) {
		// pwd here
		return 0;
	} else if (strcmp(command, BuiltinCommands[3]) == 0) {
		exit(0);
	}
	return -1;
}