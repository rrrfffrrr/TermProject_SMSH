/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#include "errstr.h"
#include "command.h"
#include "subshell.h"
#include "builtin.h"
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

enum CommandFS {
	CFS_Command,
	CFS_Argument,
	CFS_Operator,
	CFS_Separator,
	CFS_SubshellCommand,
	CFS_SubshellOperator
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
	enum CommandFS fsm = CFS_Separator;
	size_t subshellWrapCount = 0;
	size_t quoteCount = 0;
	size_t isBackground = 0;
	char* subshellBegin = NULL;

	for(char* cursor = (char*)cbegin;cursor != cend; ++cursor) {
		switch(fsm) {
			case CFS_SubshellCommand:
				switch(*cursor) {
					case '(': subshellWrapCount++; break;
					case ')': 
						if (--subshellWrapCount == 0) {
							fsm = CFS_SubshellOperator;
							
						}
					default: break;
				}
			break;
			default:
				printf(ERR_UNEXPECTED, *cursor);
				exit(0);
			return;
		}
	}
}