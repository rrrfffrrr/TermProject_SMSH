/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include "errstr.h"
#include "command.h"
#include "subshell.h"
#include "builtin.h"
#include <stdio.h>
#include <string.h>

enum CommandFS {
	CFS_Ready,
	CFS_Operator,
	CFS_Identifier,
	CFS_Background,
	CFS_Separator,
	CFS_SubshellStarter,
	CFS_SubshellFinisher
};

bool CheckCommandSyntax(char* command) {
	const char* const cbegin = command;
	const char* const cend = &command[strlen(command)+1];
	enum CommandFS fsm = CFS_Ready;
	size_t subshellCounter = 0;

	for(char* cursor = (char*)cbegin;cursor != cend; ++cursor) {
		switch(fsm) {
			case CFS_Ready: // first character
				switch(*cursor) {
					case '	': case ' ': break;
					case '>':
						if (*(cursor+1) == '|' || *(cursor+1) == '>') { // hard coding for fast complete
							++cursor;
						}
					case '<':
						fsm = CFS_Operator;
					break;
					case ';': break;
					case '|':
					case '&':
						printf(ERRCMD_SYNTAX, *cursor);
						return false;
					break;
					case '(':
						fsm = CFS_Ready;
						subshellCounter++;
					break;
					case ')':
						if (--subshellCounter < 0) {
							printf(ERRCMD_SYNTAX, *cursor);
							return false;
						}
						fsm = CFS_Ready;
					break;
					default:
						fsm = CFS_Identifier;
					break;
				}
			break;
			case CFS_Identifier:
				switch(*cursor) {
					case ';':
						fsm = CFS_Ready;
					break;
					case '>':
						if (*(cursor+1) == '|' || *(cursor+1) == '>') { // hard coding for fast complete
							++cursor;
						}
					case '|':
					case '<':
						fsm = CFS_Operator;
					break;
					case '&':
						fsm = CFS_Background;
					break;
					case '(': case ')':
						printf(ERRCMD_SYNTAX, *cursor);
						return false;
					default: break;
				}
			break;
			case CFS_Background:
				switch(*cursor) {
					case ' ': case '	': break;
					case '(':
						fsm = CFS_Ready;
						subshellCounter++;
					break;
					case ')':
						if (--subshellCounter < 0) {
							printf(ERRCMD_SYNTAX, *cursor);
							return false;
						}
						fsm = CFS_Ready;
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
						fsm = CFS_Operator;
					break;
					default:
						fsm = CFS_Identifier;
					break;
				}
			break;
			case CFS_Operator:
				switch(*cursor) {
					case ' ': case '	': break;
					case ';': case '>': case '<': case '(': case ')':
						printf(ERRCMD_SYNTAX, *cursor);
						return false;
					default:
						fsm = CFS_Identifier;
					break;
				}
			break;
			case CFS_SubshellStarter: case CFS_SubshellFinisher: case CFS_Separator: // Deprecated for now...
			default:
				printf("Unexpected syntax.");
				return false;
		}
	}

	return true;
}

void RunCommand(char* command) {
	const char* const cbegin = command;
	const char* const cend = &command[strlen(command)+1];
	enum CommandFS fsm = CFS_Ready;

	for(char* cursor = (char*)cbegin;cursor != cend; ++cursor) {
		printf("%c", *cursor);
	}
}