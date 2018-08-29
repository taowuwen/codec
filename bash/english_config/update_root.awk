
BEGIN {
	FS="|"
}

function do_print(myitem)
{
	printf("%s", myitem)

	for (j = 2; j <= NF; j++) {
		printf("|%s", $j);
	}

	printf("\n");
}

{
	gsub(" ", "", $1);
	split($1, ary, ","); 

	for (var in ary) {
		item =  ary[var]
		gsub(",", "", item)
		if (item ~ /\//) {
			split(item, ary_item, "/")

			len = length(ary_item)
			if (len == 2)
				do_print(ary_item[1]);

			for (i = 2; i <= len; i++) {
				if (length(ary_item[i]) > 0) {
					do_print(sprintf("%s%s", ary_item[1], ary_item[i]))
				}
			}
		} else {
			if (length(item) > 0)
				do_print(item)
		}
	} 
}
