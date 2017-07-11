#include <stdio.h>
#include <stdlib.h>


#define MY_HTTP		"123\r\n345"
#define MY_HTTP1	"a123\r\n343"
#define MY_HTTP2	"0123\r\n3"
#define MY_HTTP3	" 123\r\n34"

int main(int argc, char *argv[])
{
	printf("%s -> %d\n", MY_HTTP, atoi(MY_HTTP));
	printf("%s -> %d\n", MY_HTTP1, atoi(MY_HTTP1));
	printf("%s -> %d\n", MY_HTTP2, atoi(MY_HTTP2));
	printf("%s -> %d\n", MY_HTTP3, atoi(MY_HTTP3));

	return 0;
}
