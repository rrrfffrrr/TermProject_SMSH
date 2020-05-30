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

	char singleCmd[MAX_COMMAND_LENGTH];
	char* cmdStart = (char*)cbegin;
	size_t length;

	int fd[2][2];
	int nextPipe = 0;
	int *pipeS, *pipeR;

	for(char* cursor = cmdStart;cursor != cend; ++cursor) {
		switch(*cursor) {
			case '|':
				length = (size_t)(cursor - cmdStart);
				if (length <= 0) {
					printf(ERRCMD_SYNTAX, *cursor);
					return;
				}
				memcpy(singleCmd, cmdStart, length);
				singleCmd[length] = '\0';

				pipeS = fd[nextPipe];
				nextPipe = (nextPipe + 1) % 2;
				pipe(pipeS);
				RunSingleCommand(singleCmd, false, pipeR, pipeS);
				if (pipeR != NULL) {
					close(pipeR[0]);
					close(pipeR[1]);
				}
				pipeR = pipeS;
			break;
			case '&':
				length = (size_t)(cursor - cmdStart);
				if (length <= 0) {
					printf(ERRCMD_SYNTAX, *cursor);
					return;
				}
				memcpy(singleCmd, cmdStart, length);
				singleCmd[length] = '\0';

				RunSingleCommand(singleCmd, true, pipeR, NULL);
				if (pipeR != NULL) {
					close(pipeR[0]);
					close(pipeR[1]);
					pipeR = NULL;
				}
			break;
			case '\0': case ';':
				length = (size_t)(cursor - cmdStart);
				if (length > 0) {
					memcpy(singleCmd, cmdStart, length);
					singleCmd[length] = '\0';
					RunSingleCommand(singleCmd, false, pipeR, NULL);
					if (pipeR != NULL) {
						close(pipeR[0]);
						close(pipeR[1]);
						pipeR = NULL;
					}
				} else if (pipeR == NULL) {
					printf(ERRCMD_SYNTAX, *cursor);
					return;
				}
			break;
			default: break;
		}
	}
}

void RunSingleCommand(char* command, bool isBackground, int* pipeReceiver, int* pipeSender) {
	pid_t pid = fork();
	if (pid < 0) {
		printf(ERRFORK);
		return;
	} else if (pid == 0) {
		if (pipeReceiver != NULL)
			PipeReceiver(pipeReceiver)
		if (pipeSender != NULL)
			PipeSender(pipeSender);

		
	} else {
		if (isBackground == false)
			waitpid(pid);
	}
}