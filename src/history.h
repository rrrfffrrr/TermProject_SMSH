/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#ifndef __HISTORY_H
#define __HISTORY_H
#include <stddef.h>
#include "type.h"

#define MAX_HISTORY_COUNT 1024

bool ParseCommandToAddHistory(char*, size_t, size_t*);
void AddHistory(char*);
void ShowHistory();
size_t GetHistoryCount();
void GetHistory(size_t, char*, size_t);
// Not implemented
void FindHistory(char*, char*, size_t);

#endif