/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include "errstr.h"
#include "command.h"
#include "subshell.h"
#include "builtin.h"
#include "redirect.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum CheckCommandFS {
	CCFS_Ready,
	CCFS_Operator,
	CCFS_Identifier,
	CCFS_Background,
	CCFS_Separator,
	CCFS_SubshellStarter,
	CCFS_SubshellFinisher
};

bool CheckCommandSyntax(char* command) {
	const char* const cbegin = command;
	const char* const cend = &command[strlen(command)+1];
	enum CheckCommandFS fsm = CCFS_Ready;
	size_t subshellCounter = 0;

	for(char* cursor = (char*)cbegin;cursor != cend; ++cursor) {
		switch(fsm) {
			case CCFS_Ready: // first character
				switch(*cursor) {
					case '	': case ' ': break;
					case '>':
						if (*(cursor+1) == '|' || *(cursor+1) == '>') { // hard coding for fast complete
							++cursor;
						}
					case '<':
						fsm = CCFS_Operator;
					break;
					case ';': break;
					case '|':
					case '&':
						printf(ERRCMD_SYNTAX, *cursor);
						return false;
					break;
					case '(':
						fsm = CCFS_Ready;
						subshellCounter++;
					break;
					case ')':
						if (--subshellCounter < 0) {
							printf(ERRCMD_SYNTAX, *cursor);
							return false;
						}
						fsm = CCFS_Ready;
					break;
					default:
						fsm = CCFS_Identifier;
					break;
				}
			break;
			case CCFS_Identifier:
				switch(*cursor) {
					case ';':
						fsm = CCFS_Ready;
					break;
					case '>':
						if (*(cursor+1) == '|' || *(cursor+1) == '>') { // hard coding for fast complete
							++cursor;
						}
					case '|':
					case '<':
						fsm = CCFS_Operator;
					break;
					case '&':
						fsm = CCFS_Background;
					break;
					case '(': case ')':
						printf(ERRCMD_SYNTAX, *cursor);
						return false;
					default: break;
				}
			break;
			case CCFS_Background:
				switch(*cursor) {
					case ' ': case '	': break;
					case '(':
						fsm = CCFS_Ready;
						subshellCounter++;
					break;
					case ')':
						if (--subshellCounter < 0) {
							printf(ERRCMD_SYNTAX, *cursor);
							return false;
						}
						fsm = CCFS_Ready;
					break;
					case '|':
					case ';':
						printf(ERRCMD_SYNTAX, *cursor);
						return false;
					case '>':
						if (*(cursor+1) == '|' || *(cursor+1) == '>') { // hard coding for fast complete
							++cursor;
						}
					case '<':
					case '&':
						fsm = CCFS_Operator;
					break;
					default:
						fsm = CCFS_Identifier;
					break;
				}
			break;
			case CCFS_Operator:
				switch(*cursor) {
					case ' ': case '	': break;
					case ';': case '>': case '<': case '(': case ')':
						printf(ERRCMD_SYNTAX, *cursor);
						return false;
					default:
						fsm = CCFS_Identifier;
					break;
				}
			break;
			case CCFS_SubshellStarter: case CCFS_SubshellFinisher: case CCFS_Separator: // Deprecated for now...
			default:
				printf(ERRCMD_SYNTAX, *cursor);
				return false;
		}
	}

	return true;
}

void RunCommand(char* command) {
	const char* const cbegin = command;
	const char* const cend = &command[strlen(command)+1];

	char* cmdStart = (char*)cbegin;
	char* cmdEnd;
	bool isBackground = false;
	bool stop = false;

	for(char* cursor = cmdStart;cursor != cend && stop == false; ++cursor) {
		isBackground = false;
		// TRIM
		while(*cursor == ' ' || *cursor == '	')
			++cursor;

		// FIND LAST
		cmdStart = cursor;
		while(true) {
			if (*cursor == ';')
				break;
			if (cursor == cend) {
				stop = true;
				break;
			}
			if (*cursor != '&') {
				isBackground = true;
				break;
			}
			if (*cursor != ' ' && *cursor != '	')
				cmdEnd = cursor;
			++cursor;
		}

		ParsePipeCommand(cmdStart, cmdEnd, isBackground);
		++cursor;
	}
}

void ParsePipeCommand(const char* start, const char* end, bool background) {
	char* cursor = start;
	char* singleStart = start;
	char* singleEnd = start;
	bool isFirst = true;
	bool isLast = false;

	char cmd[MAX_COMMAND_LENGTH];
	pid_t pid;

	int pfd[2];
	int lastOut = STDOUT_FILENO;

	while(isLast != true && cursor != end) {
		while(*cursor == ' ' || *cursor == '	')
			++cursor;
		singleStart = cursor;
		while(true) {
			if (*cursor == '|')
				break
			if (cursor == end) {
				isLast = true;
				break
			}
			if (*cursor != ' ' && *cursor != '	')
				singleEnd = cursor;
			++cursor;
		}

		size_t len = singleEnd - singleStart;
		memcpy(cmd, singleStart, len);
		cmd[len] = '\0';

		pipe(pfd);
		if (IsBuiltinCommand(cmd) || (cmd[0] == 'c' && cmd[1] == 'd' && (cmd[2] == ' ' || cmd[2] == '	'))) { // hardcoding for just test
			RunBuiltinCommand(cmd);
		} else {
			switch(pid = fork()) {
			case -1:
				printf(ERRFORK);
				exit(1);
				break;
			case 0:
				if (isFirst != true) {
					close(STDIN_FILENO);
					dup(pfd[0]);
				}
				if (isLast != true) {
					close(STDOUT_FILENO);
					dup(pfd[1]);
				}
				close(pfd[0]);
				close(pfd[1]);
				RunSingleCommand(cmd);
				exit(0);
				break;
			default:
				break;
			}
		}

		close(pfd[0]);
		if (isFirst != true)
			close(lastOut);
		lastOut = pfd[1];
		if (isLast) {
			close(lastOut);
			if (isBackground)
				wait(pid);
		}
		isFirst = false;
	}
}

typedef struct _SCommandFrag {
	char* cmd;
	size_t size;
	struct _SCommandFrag* next;
} CFrag;

CFrag* SAllocCmdFrag(char* frag) {
	CFrag* node = (CFrag*)malloc(sizeof(CFrag));
	size_t ln = strlen(frag);
	node->size = ln + 1;
	node->next = NULL;
	node->cmd = (char*)malloc(sizeof(char) * (node->size));
	memcpy(node->cmd, frag, ln);
	node->cmd[ln+1] = '\0';
	return node;
}
size_t SAddCmdFrag(CFrag **top, char* frag) {
	if (*top == NULL) {
		*top = SAllocCmdFrag(frag);
		return 1;
	}
	CFrag *t = *top;
	while(t->next != NULL) {
		t = t->next;
	}

}
void SClearCmdFrag(CFrag **top) {
	CFrag *next, *t = *top;
	while(t != NULL) {
		next = t->next;

		free(t->cmd);
		free(t);

		t = next;
	}
	*top = NULL;
}
char** SCmdFragToArray(CFrag * top, size_t size) {
	char** list = (char**)malloc(sizeof(char*)*size);
	if (list == NULL)
		return NULL;
	int i = 0;
	while(top != NULL) {
		list[i++] = top->cmd;
		top = top->next;
		if (i == size)
			break;
	}
	return list;
}

void RunSingleCommand(char* command) {
	if (command == NULL || strlen(command) == 0)
		return;
	// redirect in out
//	run exec

	printf(ERRCMD_NOTFOUND, "command here");
}