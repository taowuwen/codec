#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>


extern char **environ;
char **argvs;

typedef union _ems_request_s  ems_request;
typedef union _ems_response_s ems_response;

typedef unsigned int  ems_uint;
typedef unsigned char ems_uchar;

//#pragma pack(push, 1)

union _ems_request_s {
	ems_uchar val[8];
	struct {
		union {
			ems_uint  val;
			struct {
				union {
					unsigned short mod;
					struct {
						ems_uchar :7;
						ems_uchar ty:1;
					};
				};
				unsigned short msg;
			};
		} tag;
		ems_uint  len:32;
	};
};

union _ems_response_s {
	ems_uchar val[12];
	struct {
		union {
			ems_uint val;
			struct {
				ems_uint  :7;
				ems_uint  ty:1;
				ems_uint  mod:8;
				ems_uint  msg:16;
			};
		} tag;
		ems_uint  len;
		ems_uint  st;
	};
};

//#pragma pack(pop)

void do_test_setproctitle()
{
	int size, i;
	char *p;

	if (environ && environ[0])
		printf("environ[0] = %s\n", environ[0]);

	//setproctitle("setproctitle: %s", "hello,world");
	
	for (i = 0; environ[i]; i++) {
		printf("environ[%d] = %s\n", i, environ[i]);
	}
}

void do_print_hex(char *src, int len)
{
	char ch;
	int i;

	for (i = 0; i < len; i++)
	{
		ch = *src++;
		printf("%.2X ", (unsigned int)(ch & 0xff));
	}

	printf("\n");
}


int main(int argc, char **argv)
{
	printf("hello, world!\n");
	argvs=argv;
	do_test_setproctitle();


	printf("0 ^ 0x01 = %d\n", 0 ^ 0x01);


	printf("sizeof(ems_request)=%d\n", (int)sizeof(ems_request));
	printf("sizeof(ems_response)=%d\n", (int)sizeof(ems_response));

	{
		ems_uint tag = 0;
		ems_request req = {0};

		req.len = htonl(15);

		req.tag.mod = htons(2);
		req.tag.msg = htons(4);
		req.tag.ty  = 1;

		do_print_hex(req.val, sizeof(req));


		printf("ty:  %d\n", req.tag.ty);
		printf("mod: %X\n", ntohs(req.tag.mod));
		printf("msg: %X\n", ntohs(req.tag.msg));

		printf("val: 0x%X\n", ntohl(req.tag.val));

		tag = (0x80000000 | (0x02 << 16) | 0x02);
		req.tag.val = htonl(tag);
		do_print_hex(req.val, sizeof(req));
		printf("val: 0x%X\n", ntohl(req.tag.val));
	}

	{
#include <time.h>
		time_t tm = 2988428202;
		time_t tm1 = 1413022673;

		printf("ctime(2988428202) = %s\n", ctime(&tm));
		printf("ctime(1413022673) = %s\n", ctime(&tm1));
	}


#define CENGBAR	"cengbar_"	
#define FORWARD	CENGBAR"fwd"

	printf("cengbar: %s, forward: %s\n", CENGBAR, FORWARD);

	printf("cengbar: " CENGBAR " forward: " FORWARD "\n");

	{
		void *ptr = malloc(10);

		unsigned int p = (unsigned int )ptr;

		printf("ptr: %p: , 0x%x\n", ptr, p);

		free(ptr);

	}

	return 0;
}
