/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#ifndef __SMSC_BUILTIN_H
#define __SMSC_BUILTIN_H
#include <stdbool.h>
#include <sys/types.h>

bool IsBuiltinCommand(char*);
ssize_t RunBuiltinCommand(char*, int, char**);

#endif