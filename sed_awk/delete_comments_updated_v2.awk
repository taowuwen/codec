
{
	ptr = $0

	while (0 != (t = index(ptr, "/*"))) {

		printf("%s", substr(ptr, 1, t -1))

		ptr = substr(ptr, t + 2)

		u = index(ptr, "*/")

		while (u == 0) {
			printf("\n")
			if (getline <= 0) {
				m = "unexpected EOF or error"
				m = (m ": " ERRNO)
				print m > "/dev/stderr"
					exit
			}

			ptr = $0
			u = index(ptr, "*/")
		}

		ptr = substr(ptr, u + 2)
	}


	printf("%s\n", ptr)
}
