/*
* KANG CHAN YEONG(rrrfffrrr@naver.com)
*/
#ifndef __SMSC_ERROR_H
#define __SMSC_ERROR_H

#define ERR_MAX_LEN 4096

// error string
#define ERR_UNEXPECTED "Critical: Unexpected syntax near `%c'\n"
#define ERRFORK	"bash: Cannot fork new process.\n"

/// syntax error
#define ERREVENT_NOTFOUND "bash: !%lu: event not found.\n"

/// command error
#define ERRCMD_SYNTAX "bash: syntax error near unexpected token `%c'\n"
#define ERRCMD_NOCLOBBER "bash: %s: cannot overwrite existing file\n"
#define ERRCMD_NOTFOUND "Command '%s' not found.\n"
#define ERRCMD_CD_DEFAULT "bash: cd: %s: Unexpected error.\n"
#define ERRCMD_CD_NODIR "bash: cd: %s: No such file or directory\n"

#endif