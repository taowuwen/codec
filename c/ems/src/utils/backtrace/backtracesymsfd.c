#ifndef GENERIC_LINUX

#include "execinfo.h"
#include <string.h>
#include <sys/uio.h>
#include <dlfcn.h>
#include <stdio.h>
#include <link.h>	/* required for __ELF_NATIVE_CLASS */

#if __ELF_NATIVE_CLASS == 32
# define WORD_WIDTH 8
#else
/* We assyme 64bits.  */
# define WORD_WIDTH 16
#endif

#define BUF_SIZE (WORD_WIDTH + 1)

extern int dladdr(void *addr, mips_Dl_info *info);

void backtrace_symbols_fd (void *const *array, int size, int fd)
{
	struct iovec iov[9];
	int cnt;

	for (cnt = 0; cnt < size; ++cnt) {
		char buf[BUF_SIZE];
		mips_Dl_info info;
		size_t last = 0;
		size_t len = 0;

		memset(buf, 0, sizeof(buf));
		if (dladdr (array[cnt], &info)
			&& info.dli_fname && info.dli_fname[0] != '\0')	{
			/* Name of the file.  */
			iov[0].iov_base = (void *) info.dli_fname;
			iov[0].iov_len = strlen (info.dli_fname);
			last = 1;

			/* Symbol name.  */
			if (info.dli_sname != NULL) {
				char buf2[BUF_SIZE];
				memset(buf2, 0, sizeof(buf2));
				size_t diff;

				iov[1].iov_base = (void *) "(";
				iov[1].iov_len = 1;
				iov[2].iov_base = (void *) info.dli_sname;
				iov[2].iov_len = strlen (info.dli_sname);

				if (array[cnt] >= (void *) info.dli_saddr) {
					iov[3].iov_base = (void *) "+0x";
					diff = array[cnt] - info.dli_saddr;
				} else {
					iov[3].iov_base = (void *) "-0x";
					diff = info.dli_saddr - array[cnt];
				}

				iov[3].iov_len = 3;

				/* convert diff to a string in hex format */
				len = snprintf(buf2, sizeof(buf2), "%lx", (unsigned long) diff);
				iov[4].iov_base = buf2;
				iov[4].iov_len = len;

				iov[5].iov_base = (void *) ")";
				iov[5].iov_len = 1;

				last = 6;
			}
		}

		iov[last].iov_base = (void *) "[0x";
		iov[last].iov_len = 3;
		++last;

		/* convert array[cnt] to a string in hex format */
		len = snprintf(buf, sizeof(buf), "%lx", (unsigned long) array[cnt]);
		iov[last].iov_base = buf;
		iov[last].iov_len = len;

		++last;

		iov[last].iov_base = (void *) "]\n";
		iov[last].iov_len = 2;
		++last;

		writev (fd, iov, last);
	}
}
#endif
