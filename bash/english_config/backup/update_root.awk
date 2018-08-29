
BEGIN {
	FS="|"
}
{
	gsub(" ", "", $1);
	split($1, ary, ","); 

	print($1);

	for (var in ary) {
		item =  ary[var]
		printf("\t: ");

		gsub(",", "", item)
		if (item ~ /\//) {
			split(item, ary_item, "/")

			len = length(ary_item)
			if (len == 2)
				printf("--> %s ", ary_item[1])

			for (i = 2; i <= len; i++) {

				v = sprintf("%s%s", ary_item[1], ary_item[i]);

				printf("----> %s", v);
			}

		} else {
			printf("%s", item )
		}

		printf("\n");
	} 
}
