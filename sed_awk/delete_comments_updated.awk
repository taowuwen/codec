{
	if ((t = index($0, "/*")) != 0) {

		tmp = substr($0, 1, t - 1)
		u = index(substr($0, t + 2), "*/")

		if (u == 0) {
			t = 0
			do {
				if (getline <= 0) {
					m = "unexpected EOF or error"
					m = (m ": " ERRNO)
					print m > "/dev/stderr"
						exit
				}

				u = index($0, "*/")
			} while (u == 0)
		} else
			u = u + t + 1

# substr expression will be "" if */
# occurred at end of line
		$0 = tmp substr($0, u + 2)
	}

	print $0
}
