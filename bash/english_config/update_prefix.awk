BEGIN {
	FS="|"
	lastline[2] = ""

}

function do_print(str)
{
	printf("%s", str)

	for (j = 2; j <= NF; j++) {
		str = $j

		if (length(str) > 0) {
			lastline[j] = str
		} else {
			str = lastline[j]
		}
		 
		printf("|%s", str);
	}

	printf("\n");

}

{
	gsub("[()-]", " ", $1);

	split($1, ary, ",");

	for (item in ary )
	{
		split(ary[item], item_ary, " ");

		len = length(item_ary)

		do_print(item_ary[1])

		for ( i = 2; i <= len; i++) {
			do_print(sprintf("%s%s", item_ary[1], item_ary[i]))
		}
	}
}
