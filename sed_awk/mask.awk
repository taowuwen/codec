{
	sum = 0;
	end = 32 - mask;
	
	for (i = 32; i > end; i--){
		sum += lshift(1, i - 1);
	}

	printf("%d.%d.%d.%d\n", 
	       rshift(and(sum, 0xff000000), 24), \
	       rshift(and(sum, 0x00ff0000), 16), \
	       rshift(and(sum, 0x0000ff00), 8), \
	       rshift(and(sum, 0x000000ff), 0)\
	       );

	exit
}
