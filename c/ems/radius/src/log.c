/*
 * $Id: log.c,v 1.5 2007/06/21 18:07:23 cparker Exp $
 *
 * Copyright (C) 1995,1996,1997 Lars Fenneberg
 *
 * See the file COPYRIGHT for the respective terms and conditions.
 * If the file is missing contact me at lf@elemental.net
 * and I'll send you a copy.
 *
 */

#include <r_config.h>
#include <includes.h>
#include <freeradius-client.h>

#ifndef USE_EMS_MEMORY_MGMT

/*
 * Function: rc_openlog
 *
 * Purpose: open log
 *
 * Arguments: identification string
 *
 * Returns: nothing
 *
 */

void rc_openlog(char *ident)
{
#ifndef _MSC_VER /* TODO: Fix me */
	openlog(ident, LOG_PID, LOG_DAEMON);
#endif
}

/*
 * Function: rc_log
 *
 * Purpose: log information
 *
 * Arguments: priority (just like syslog), rest like printf
 *
 * Returns: nothing
 *
 */
void rc_log(int prio, const char *format, ...)
{
	char buff[1024];
	va_list ap;

	va_start(ap,format);
	vsnprintf(buff, sizeof(buff), format, ap);
	va_end(ap);
#if 0

#ifndef _MSC_VER /* TODO: Fix me */
	syslog(prio, "%s", buff);
#endif
#else
	fprintf(stdout, "%s\n", buff);
#endif
}
#else
void rc_openlog(char *ident)
{
	return;
}

void rc_log(int prio, const char *format, ...)
{
#ifdef DEBUG
	char buff[1024];
	va_list ap;
	va_start(ap,format);
	vsnprintf(buff, sizeof(buff), format, ap);
	va_end(ap);

	fprintf(stdout, "%s\n", buff);
#endif
}
#endif
