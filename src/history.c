/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include "history.h"
#include <stdio.h>
#include <string.h>

static char Commands[MAX_HISTORY_COUNT][MAX_COMMAND_LENGTH];
static size_t CurrentCommandCount = 0;

enum ParserFS {
	PFS_Ready,
	PFS_Block,
	PFS_Parse
};

bool ParseCommandToAddHistory(char* string, size_t size, size_t* error) {
	char* const cbegin = string;
	const char* cend = string + size;

	char buffer[MAX_COMMAND_LENGTH];
	char* cursor = cbegin;
	char* start = NULL;
	bool skipNext = false;
	bool isEvent = false;
	size_t hid = 0;
	enum ParserFS fsm = PFS_Ready;

	for(; cursor != cend; cursor++) {
		switch(fsm) {
			case PFS_Ready:
				switch(*cursor) {
					case '\\':
						fsm = PFS_Block;
					break;
					case '!':
						start = cursor;
						hid = 0;
						isEvent = false;
						fsm = PFS_Parse;
					break;
					default:
					break;
				}
			break;
			case PFS_Block:
				if (*cursor != '\\')
					fsm = PFS_Ready;
			break;
			case PFS_Parse:
				if ('0' <= *cursor && *cursor <= '9') {
					isEvent = true;
					hid = hid * 10 + (*cursor - '0');
				} else {
					fsm = PFS_Ready;
					if (isEvent == true) {
						if (hid <= 0 || hid > CurrentCommandCount) {
							if (error != NULL)
								*error = hid;
							return false;
						}
						GetHistory(hid - 1, buffer, MAX_COMMAND_LENGTH);
						size_t copySize = strlen(buffer);
						size_t interval = cursor - start;
						memmove(start + copySize, cursor, cend - cursor + (copySize - interval));
						cursor = start + copySize;
						memcpy(start, buffer, copySize);
					}
				}
			break;
		}
	}

	return true;
}

void AddHistory(char* cmd) {
	if (cmd == NULL || strlen(cmd) < 1 || (CurrentCommandCount > 0 && strcmp(cmd, Commands[CurrentCommandCount-1]) == 0)) {
		return;
	}
	size_t len = strlen(cmd)+1;
	if (len > MAX_COMMAND_LENGTH)
		len = MAX_COMMAND_LENGTH;
	memcpy(Commands[CurrentCommandCount++], cmd, len);
	Commands[CurrentCommandCount-1][len-1] = '\0';
}

void ShowHistory() {
	for(int i = 0; i < CurrentCommandCount; ++i) {
		printf("  %d  %s\n", i + 1, Commands[i]);
	}
}

size_t GetHistoryCount() {
	return CurrentCommandCount;
}

void GetHistory(size_t index, char* destination, size_t size) {
	if (destination == NULL || size < 1) {
		return;
	}
	if (index > CurrentCommandCount) {
		destination[0] = '\0';
		return;
	}
	size_t len = strlen(Commands[index]) + 1;
	if (len > size)
		len = size;
	memcpy(destination, Commands[index], size);
	destination[size-1] = '\0';
}

void FindHistory(char* toFind, char* destination, size_t size) {
	destination[0] = '\0';
}