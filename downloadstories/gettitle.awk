
/\<title\>/ {

	s = index($0, "<title>")
	if (s <= 0)
		next

	s = s + 7
	e = index($0, "</title>")
	if (e <= 0)
		next

	print substr($0, s, e-s)
}

