#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(void)
{
	char key[10];
	int  jackpot_open = 0;

	memset(key, 0, sizeof(key));

	printf("\n Enter the key : \n");
	gets(key);

	if (!strcmp(key,"Linux"))
	{
		jackpot_open = 1;
	}

	if(jackpot_open)
	{
		printf("\n Congrats, the jackpot is yours\n");
	}
	else
	{
		printf("\n Wrong key entered \n");
	}

	return 0;
}

