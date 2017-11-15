
#ifndef	GENERIC_LINUX

#ifndef _EXECINFO_H
#define _EXECINFO_H 1

#include <features.h>

__BEGIN_DECLS

typedef struct _mips_dl_info_s
{
  const char *dli_fname;	/* File name of defining object.  */
  void *dli_fbase;		/* Load address of that object.  */
  const char *dli_sname;	/* Name of nearest symbol.  */
  void *dli_saddr;		/* Exact value of nearest symbol.  */
} mips_Dl_info;

/* Store up to SIZE return address of the current program state in
   ARRAY and return the exact number of values stored.  */
extern int backtrace (void **__array, int __size) __nonnull ((1));


/* Return names of functions from the backtrace list in ARRAY in a newly
   malloc()ed memory block.  */
extern char **backtrace_symbols (void *__const *__array, int __size)
     __THROW __nonnull ((1));


/* This function is similar to backtrace_symbols() but it writes the result
   immediately to a file.  */
extern void backtrace_symbols_fd (void *__const *__array, int __size, int __fd)
     __THROW __nonnull ((1));

__END_DECLS

#endif /* execinfo.h  */

#endif
