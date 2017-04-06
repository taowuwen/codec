#!/usr/bin/env bash


# key
getargs()
{
	echo ${QUERY_STRING} |
	awk -v search="$1" -F\& '{
		for ( i = 1; i <= NF; i++) {
			split($i, ary, "=")
			if (ary[1] == search) {
				printf("%s", ary[2])
				exit
			}
		}
	}'
}

# query_string
print_all_key_value()
{
	echo $1 |
	awk -F\& '{
		print $0

		for ( i = 1; i <= NF; i++) {
			printf("<p>%d: %s</p>", i, $i);
		}
	}'
}


echo -ne "Content-Type: text/html\r\n\r\n"

echo "
<html>
<body>

<p> `env` </p>

<h1> query_string: ${QUERY_STRING} </h1>
<h4> `print_all_key_value ${QUERY_STRING}` </h4>

<h4> wanna get firstname:  `getargs firstname` </h4>
<h4> wanna get lastname:  `getargs lastname` </h4>
</body>
</html>
"

