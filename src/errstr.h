/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#ifndef __SMSC_ERROR_H
#define __SMSC_ERROR_H

// error string
/// syntax error
#define ERREVENT_NOTFOUND "bash: !%lu: event not found.\n"

/// command error
#define ERRCMD_SYNTAX "bash: syntax error near unexpected token `%c'\n"
#define ERRCMD_NOTFOUND "Command '%s' not found."

#endif