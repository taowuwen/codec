#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_binary(unsigned int n)
{
	static char buf[64] = {0};
	int sz = 32;

	buf[sz] = '\0';

	do {
		buf[sz - 1] = (n & (1 << (32 - sz)))? 0x31: 0x30;
	} while (sz-- > 0);

	return buf;
}


void test_5()
{
	int i = 0;

	printf("================================================================================\n");

	for (i = -16; i <= 0; i++) {
		printf("\t%12d, %12u, %15o, 0x%08x, %32s\n", i, i, i, i, get_binary(i));
		printf("\t%12d, %12u, %15o, 0x%08x, %32s\n", -i, -i, -i, -i, get_binary(-i));
	}

	printf("================================================================================\n");
}


void test_4()
{
	int a,b,c,d;
	unsigned int u;

	a = 12;
	b = -24;
	u = 10;

	c = a + u;
	d = b + u;

	printf("c = %d, %u, d = %d, %u, 0x%x\n", c ,c, d ,d, d);

}


void test_3()
{
	printf("sizeof(int) = %d\n", (int)sizeof(int));
	printf("sizeof(short) = %d\n", (int) sizeof(short));
	printf("sizeof(long) = %d\n", (int) sizeof(long));
	printf("sizeof(float) = %d\n", (int) sizeof(float));
	printf("sizeof(double) = %d\n", (int) sizeof(double));
	printf("sizeof(long double) = %d\n", (int) sizeof(long double));
	printf("sizeof(long long) = %d\n", (int) sizeof(long long));
}

void test_2()
{
	int i, n;
	long npp;

	npp = 1; 
	i = 2;

	scanf("%d", &n);

	while (i++ <= n)
		npp = npp * (i - 1);

	printf("n = %d, n! = %ld, i = %d\n", n, npp, i);
}

void test_1()
{

	struct stu {
		int x;
		int *y;
	} *p;

	int dt[4] = {10, 20, 30, 40};

	struct stu 
	a[4] = {50, &dt[0], 60, &dt[1], 70, &dt[2], 80, &dt[3]};

	p = a;

	printf("\n%p\n", p);

	printf("%d,", ++p->x);

	printf("\n%p\n", p);

	printf("%d,", (++p)->x);
	printf("\n%p\n", p);

	printf("%d,", ++(*p->y));
	printf("\n%p\n", p);
}


void test_6()
{
#define mystr	"hello, world"

	printf("sizeof(%s) = %d, len(%s) = %d\n", 
			mystr, (int)sizeof(mystr),
			mystr, (int)strlen(mystr)
			);

	printf("%s = %10s\n", mystr, mystr);
	printf("%s = %15s\n", mystr, mystr);
	printf("%s = %-10s\n", mystr, mystr);
	printf("%s = %-15s\n", mystr, mystr);
	printf("%s = %15.10s\n", mystr, mystr);
	printf("%s = %10.15s\n", mystr, mystr);
	printf("%s = %-15.10s\n", mystr, mystr);
	printf("%s = %-10.15s\n", mystr, mystr);

	{
		float f = 123.456;

		printf("%f\n", f);
		printf("%.4e, %g\n", f, f);

		f = 123321321.123;
		printf("%.4e, %g\n", f, f);
	}
}

void test_7()
{
	int a, b, c;

	scanf("%d%d%d", &a, &b, &c);
	printf("%d, %d, %d\n", a, b, c);
}


int main(int argc, char **argv)
{

//	test_1();
//	test_2();
//	test_3();
//	test_4();
//	test_5();

//	test_6();

	test_7();

	return 0;
}
