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
	char* cmdEnd = cmdStart;
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
			if (*cursor == '&') {
				isBackground = true;
				break;
			}
			if (*cursor != ' ' && *cursor != '	')
				cmdEnd = cursor + 1;
			++cursor;
		}

		ParsePipeCommand(cmdStart, cmdEnd, isBackground);
		if (cursor == cend)
			break;
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
		while(*cursor == ' ' || *cursor == '	') {
			++cursor;
		}
		singleStart = cursor;
		while(true) {
			if (*cursor == '|' && *(cursor - 1) != '>') {
				++cursor;
				break;
			}
			if (cursor == end) {
				isLast = true;
				break;
			}
			if (*cursor != ' ' && *cursor != '	')
				singleEnd = cursor + 1;
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
				dup(lastOut);
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
				int flag = O_WRONLY;
				flag |= (cData->appendOutput) ? O_APPEND : O_CREAT;
				flag |= (cData->ignoreClobber) ? O_TRUNC : (GetNoclobber()?0:O_TRUNC);
				open(cData->output, flag, 0666);
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
			char err[ERR_MAX_LEN];
			switch(pid = fork()) {
			case -1:
				printf(ERRFORK);
				exit(1);
				break;
			case 0:
				if (isFirst != true) {
					close(STDIN_FILENO);
					dup(lastOut);
				}
				if (isLast != true) {
					close(STDOUT_FILENO);
					dup(pfd[1]);
				}
				close(pfd[0]);
				close(pfd[1]);
				if (cData->input != NULL) {
					close(STDIN_FILENO);
					open(cData->input, O_RDONLY);
				}
				if (cData->output != NULL) {
					close(STDOUT_FILENO);
					int flag = O_CREAT|O_WRONLY;
					flag |= (cData->appendOutput) ? O_APPEND : O_TRUNC;
					if (GetNoclobber() == false && cData->ignoreClobber == false && cData->appendOutput == false) {
						snprintf(err, ERR_MAX_LEN, ERRCMD_NOCLOBBER, cData->output);
						write(2, err, strlen(err));
						exit(0);
					}
					open(cData->output, flag, 0666);
				}

				RunSingleCommand(cData->args);
				exit(0);
				break;
			default:
				if (isLast == false || background == false) {
					waitpid(pid, 0, 0);
				}
				FreeCommandData(cData);
				break;
			}
		}

		close(pfd[1]);
		if (isFirst != true)
			close(lastOut);
		lastOut = pfd[0];
		if (isLast) {
			close(lastOut);
		}
		isFirst = false;
	}
}

void RunSingleCommand(char** args) {
	if (args == NULL)
		return;
	execvp(args[0], args);
	char err[ERR_MAX_LEN];
	snprintf(err, ERR_MAX_LEN, ERRCMD_NOTFOUND, args[0]);
	write(2, err, strlen(err));
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
	for(;cursor != end; ++cursor) {
		switch(*cursor) {
			case ' ':
			case '	':
				start = cursor + 1;
				break;
			case '>':
				if (cursor[1] == '|') {
					start = cursor + 2;
					tail->next = CreateDataNode(">|");
					tail->next->prev = tail;
					tail = tail->next;
					++cursor;
				} else if (cursor[1] == '>') {
					start = cursor + 2;
					tail->next = CreateDataNode(">>");
					tail->next->prev = tail;
					tail = tail->next;
					++cursor;
				} else {
					start = cursor + 1;
					tail->next = CreateDataNode(">");
					tail->next->prev = tail;
					tail = tail->next;
				}
			break;
			case '<':
				tail->next = CreateDataNode("<");
				tail->next->prev = tail;
				tail = tail->next;
				start = cursor + 1;
			break;
			default:
				if (cursor[1] == ' ' || cursor[1] == '	' || &cursor[1] == end || cursor[1] == '>' || cursor[1] == '<') {
					size_t ll = cursor - start + 1;
					memcpy(temp, start, ll);
					temp[ll] = '\0';
					tail->next = CreateDataNode(temp);
					tail->next->prev = tail;
					tail = tail->next;
				}
				break;
		}
	}

	size_t l;
	struct DataNode* dtemp;
	for(tail = clist->next; tail != NULL;) {
		switch(tail->data[0]) {
			case '>':
				data->appendOutput = false;
				data->ignoreClobber = false;
				if (tail->data[1] == '|') { // check >|
					data->ignoreClobber = true;
				} else if (tail->data[1] == '>') { // check >>
					data->appendOutput = true;
				}
				if (data->output != NULL)
					free(data->output);

				dtemp = tail;
				tail->prev->next = tail->next;
				tail->next->prev = tail->prev;
				tail = tail->next;
				dtemp->next = NULL;
				RemoveDataNode(&dtemp);

				dtemp = tail;
				tail->prev->next = tail->next;
				if (tail->next != NULL)
					tail->next->prev = tail->prev;

				l = strlen(tail->data);
				data->output = (char*)malloc(sizeof(char) * (l + 1));
				memcpy(data->output, tail->data, l);
				data->output[l] = '\0';

				tail = tail->next;
				dtemp->next = NULL;
				RemoveDataNode(&dtemp);
			break;
			case '<':
				if (data->input != NULL)
					free(data->input);

				dtemp = tail;
				tail->prev->next = tail->next;
				tail->next->prev = tail->prev;
				tail = tail->next;
				dtemp->next = NULL;
				RemoveDataNode(&dtemp);

				dtemp = tail;
				tail->prev->next = tail->next;
				if (tail->next != NULL)
					tail->next->prev = tail->prev;

				l = strlen(tail->data);
				data->input = (char*)malloc(sizeof(char) * (l + 1));
				memcpy(data->input, tail->data, l);
				data->input[l] = '\0';

				tail = tail->next;
				dtemp->next = NULL;
				RemoveDataNode(&dtemp);
			break;
			default:
				tail = tail->next;
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