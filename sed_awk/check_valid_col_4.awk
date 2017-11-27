
{
	if (NF != 6)
		next

	if (! ($4 in arr)) {
		arr[$4] = $0
		next
	}

	split(arr[$4], tmpary);
	if (tmpary[1] != $1) {
		printf("%s \n%s\n", arr[$4], $0);
	}
}
