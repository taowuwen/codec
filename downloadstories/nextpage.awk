BEGIN {
	FS=","
	reg="next_*page[[:blank:]]*="
	next_page=""
}

/next_*page/{

	for(i = 1; i <= NF;  i++) {
		where = match($i, reg)

		if (where != 0) {
			split($i, ary, "\"")
			next_page=ary[2]
			exit
		}
	}
}

END{
	print next_page

}
