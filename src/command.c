/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include "errstr.h"
#include "command.h"
#include "subshell.h"
#include "builtin.h"
#include "redirect.h"
#include "type.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

struct CommandData {
	char** args;
	char* input;
	char* output;
	bool ignoreClobber;
	bool appendOutput;
};

enum CheckCommandFS {
	CCFS_Ready,
	CCFS_Operator,
	CCFS_Identifier,
	CCFS_Background,
	CCFS_Separator,
	CCFS_SubshellStarter,
	CCFS_SubshellFinisher
};

void FreeCommandData(struct CommandData* );
struct CommandData* ParseCommand(char* );
void ParsePipeCommand(const char* , const char* , bool );
void RunSingleCommand(char** );

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
	char* cursor = (char*)start;
	char* singleStart = (char*)start;
	char* singleEnd = (char*)start;
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
				break;
			if (cursor == end) {
				isLast = true;
				break;
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
		if (IsBuiltinCommand(cData->args[0])) {
			int iOrigin = -1, oOrigin = -1;
			if (isFirst != true) {
				iOrigin = dup(STDIN_FILENO);
				close(STDIN_FILENO);
				dup(pfd[0]);
			}
			if (isLast != true) {
				oOrigin = dup(STDOUT_FILENO);
				close(STDOUT_FILENO);
				dup(pfd[1]);
			}
			if (cData->input != NULL) {
				if (iOrigin == -1) {
					iOrigin = dup(STDIN_FILENO);
				}
				close(STDIN_FILENO);
				open(cData->input, O_RDONLY);
			}
			if (cData->output != NULL) {
				if (oOrigin == -1) {
					oOrigin = dup(STDOUT_FILENO);
				}
				close(STDOUT_FILENO);
				open(cData->output, O_CREAT|O_WRONLY|(cData->appendOutput == true?O_APPEND:0)|((cData->ignoreClobber == true?O_TRUNC:(GetNoclobber() == true?0:O_TRUNC))), 0666);
			}

			RunBuiltinCommand(cData->args);

			if (iOrigin != -1) {
				close(STDIN_FILENO);
				dup(iOrigin);
				close(iOrigin);
			}
			if (oOrigin != -1) {
				close(STDOUT_FILENO);
				dup(oOrigin);
				close(oOrigin);
			}
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
				RunSingleCommand(cData->args);
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
			if (background)
				waitpid(pid, 0, 0);
		}
		isFirst = false;
	}
}

void RunSingleCommand(char** args) {
	if (args == NULL)
		return;
	execvp(args[0], args);
	char err[ERR_MAX_LEN];
	snprintf(err, ERR_MAX_LEN, ERRCMD_NOTFOUND, "command here");
	write(1, err, strlen(err));
}

/// Parsing and return structured data
struct DataNode {
	char* data;
	struct DataNode* next, *prev;
};

struct DataNode* CreateDataNode(char* data) {
	struct DataNode* node = (struct DataNode*)malloc(sizeof(struct DataNode));
	if (data == NULL) {
		node->data = NULL;
	} else {
		node->data = (char*)malloc(sizeof(char) * (strlen(data) + 1));
		memcpy(node->data, data, strlen(data) + 1);
	}
	node->next = NULL;
	node->prev = NULL;
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

	int i = 0;
	for(struct DataNode* n = list;n != NULL; n = n->next, ++i) {
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
	data->ignoreClobber = false;
	data->appendOutput = false;

	struct DataNode* clist = CreateDataNode(NULL);
	struct DataNode* tail = clist;

	char temp[MAX_COMMAND_LENGTH];
	char* start = command;
	char* cursor = command;
	char* end = (char*)((size_t)command + strlen(command));
	for(;start != end; ++cursor) {
		switch(*cursor) {
			case ' ':
			case '	':
				start = cursor + 1;
				break;
			default:
				if (cursor[1] == ' ' || cursor[1] == '	' || &cursor[1] == end) {
					size_t ll = cursor - start + 1;
					memcpy(temp, start, ll);
					temp[ll] = '\0';
					tail->next = CreateDataNode(temp);
					tail->next->prev = tail;
					tail = tail->next;
					if (&cursor[1] == end)
						--cursor;
				}
				break;
		}
	}

	size_t l;
	struct DataNode* dtemp;
	for(tail = clist->next; tail != NULL; tail = tail->next) {
		switch(tail->data[0]) {
			case '>':
				data->appendOutput = false;
				data->ignoreClobber = false;
				bool isTwo = false;
				if (tail->data[1] == '|') { // check >|
					data->appendOutput = true;
					isTwo = true;
				} else if (tail->data[1] == '>') { // check >>
					data->ignoreClobber = true;
					isTwo = true;
				}
				if (data->output != NULL)
					free(data->output);

				if (tail->data[1 + isTwo] != '\0') {
					l = strlen(tail->data) - 1 - isTwo;
					data->output = (char*)malloc(sizeof(char) * (l + 1));
					memcpy(data->output, tail->data + 1 + isTwo, l);
					data->output[l] = '\0';
					tail->prev->next = tail->next;
					tail->next->prev = tail->prev;
					tail->next = NULL;
					dtemp = tail;
					tail = tail->prev;
					RemoveDataNode(&dtemp);
				} else {
					l = strlen(tail->next->data);
					data->output = (char*)malloc(sizeof(char) * (l + 1));
					memcpy(data->output, tail->next->data, l);
					data->output[l] = '\0';
					tail->prev->next = tail->next->next;
					tail->next->next->prev = tail->prev;
					dtemp = tail;
					tail = tail->prev;
					tail->next->next = NULL;
					RemoveDataNode(&dtemp);
				}
			case '<':
				if (data->input != NULL)
					free(data->input);

				if (tail->data[1] != '\0') {
					l = strlen(tail->data) - 1;
					data->input = (char*)malloc(sizeof(char) * (l + 1));
					memcpy(data->input, tail->data + 1, l);
					data->input[l] = '\0';
					dtemp = tail;
					tail->prev->next = tail->next;
					tail->next->prev = tail->prev;
					tail->next = NULL;
					RemoveDataNode(&dtemp);
				} else {
					l = strlen(tail->next->data);
					data->input = (char*)malloc(sizeof(char) * (l + 1));
					memcpy(data->input, tail->next->data, l);
					data->input[l] = '\0';
					dtemp = tail;
					tail->prev->next = tail->next->next;
					tail->next->next->prev = tail->prev;
					tail->next->next = NULL;
					RemoveDataNode(&dtemp);
				}
			default:
				break;
		}
	}

	data->args = BuildDataNodeString(clist->next);
	RemoveDataNode(&clist);
	return data;
}

void FreeCommandData(struct CommandData* data) {
	if (data == NULL) return;
	for(int i = 0; data->args[i] != NULL; ++i)
		free(data->args[i]);
	free(data->args);
	if (data->input != NULL) free(data->input);
	if (data->output != NULL) free(data->output);
}