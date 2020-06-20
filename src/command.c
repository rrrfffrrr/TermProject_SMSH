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

struct CommandData {
	char** args;
	char* input;
	char* output;
	bool appendOutput;
}

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

		struct CommandData* cData = ParseCommand(cmd);

		pipe(pfd);
		if (IsBuiltinCommand(cmd)) {
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
				FreeCommandData(cData);
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

void RunSingleCommand(char** args) {
	if (command == NULL || strlen(command) == 0)
		return;
	execvp(args[0], args);
	char err[ERR_MAX_LEN];
	snprintf(err, ERR_MAX_LEN, ERRCMD_NOTFOUND, "command here");
	write(1, err, strlen(err));
}

/// Parsing and return structured data
struct DataNode {
	char* data;
	struct DataNode* next;
}
struct DataNode* CreateDataNode(char* data) {
	struct DataNode* node = (struct DataNode*)malloc(sizeof(struct DataNode));
	node->data = (char*)malloc(sizeof(char) * (strlen(data) + 1));
	memcpy(node->data, data, strlen(data) + 1)
	node->next = NULL;
	return node;
}
void RemoveDataNode(struct DataNode** node) {
	struct DataNode* n = *node, *temp;
	*node = NULL;
	while(n != NULL) {
		if (n->data != NULL)
			free(n->data);
		temp = n;
		n = n->next;
		free(temp);
	}
}
char** BuildDataNodeString(struct DataNode* list) {
	char** ret;
	size_t num = 0;
	for(struct DataNode* n = list;n != NULL; n = n->next) {
		++num;
	}

	ret = (char**)malloc(sizeof(char*) * (num + 1));

	for(struct DataNode* n = list, int i = 0;n != NULL; n = n->next, ++i) {
		size_t sl = strlen(n->data);
		ret[i] = (char*)malloc(sizeof(char) * (sl + 1));
		memcpy(ret[i], n->data, sl);
		ret[i][sl] = '\0';
	}	

	ret[num] = NULL;
	return ret;
}

// methods
struct CommandData* ParseCommand(char* command) {
	struct CommandData* data = (struct CommandData*)malloc(sizeof(struct CommandData));
	if (data == NULL)
		return NULL;
	data->args = NULL;
	data->input = NULL;
	data->output = NULL;
	data->appendOutput = false;

	struct DataNode* clist = CreateDataNode(NULL);

	data->args = BuildDataNodeString(clist->next);
	RemoveDataNode(&clist);
	return data;
}

void FreeCommandData(struct CommandData* data) {
	if (data == NULL) return;
	char* data;
	for(int i = 0; data[i] != NULL; ++i)
		free(data[i]);
	free(data[i]);
	if (input != NULL) free(input);
	if (output != NULL) free(output);
}