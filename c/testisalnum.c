
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main()
{
	int i;
	char ary[] = {
		0xff,
		0xa1,
		0x30,
		0x31,
		0x65,
		'A',
		'Z',
		'a',
		'z'
	};

	char ch;

	for (i = 0; i < sizeof(ary); i++) {
		ch = ary[i];

		printf("0x%02x === isalnum? %s\n", (ch & 0xff), isalnum(ch)?"yes":"no");
	}

	return 0;
}
