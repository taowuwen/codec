
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int create_file(const char *fl, unsigned long sz, unsigned char ch)
{
	FILE     *fp;
	unsigned long i, len;
	size_t    rtn, sz_wr;
	char      buf[4096];

	printf("fl: %s, total size: %lu, %x\n", fl, sz, ch);

	fp = fopen(fl, "wb");
	sz_wr = 4096;

	if (fp) {
		memset(buf, ch, sz_wr);

		while (sz > 0) {
			if (sz_wr > sz) 
				sz_wr = sz;

			rtn = fwrite(buf, 1, sz_wr, fp);

		}

		for (i = 0; i < sz; i++) {

			rtn = 0;

			len = fwrite(buf, sz_wr, 1, fp);
		}

		fclose(fp);
	}


	return 0;
}



int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "%s filename filesize num\n", argv[0]);
		return 1;
	}

	return create_file(argv[1], atol(argv[2]), atoi(argv[3]));
}
