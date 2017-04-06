/&nbsp;/ {
	gsub("&nbsp;", " ", $0)
	gsub("<br />", "\n", $0)

#	gsub("<div.*./div>", "", $0)

#	gsub("<.*.>", "", $0)
	print $0

}

