

#include <stdio.h>

unsigned long get_sp(void)
{
    /* This function (suggested in alephOne's paper) prints the 
       stack pointer using assembly code. */
    __asm__("movl %esp,%eax");
}

int main() {
//	register long i asm("esp");
	unsigned long sp = 0;
	char buffer[512];

//	printf("$esp = %#010x\n", i);

	sp = get_sp();
	printf("$esp = 0x%lx\n", get_sp());
	return 1;
}
