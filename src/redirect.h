/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#ifndef __SMSC_REDIRECT_H
#define __SMSC_REDIRECT_H
#include <stdbool.h>

#define RDO_ForceOverride	0x01
#define RDO_Append			0x02

void SetNoclobber(bool);

void PipeReceiver(int* pipe);
void PipeSender(int* pipe);

bool RedirectInput(char*);
bool RedirectOutput(char*, int);

// Deprecated
bool RedirectError(char*);

#endif