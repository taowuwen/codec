
#include "ems.h"

#if 0
static ems_char from_hex(ems_char ch)
{
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}
#endif

static ems_char to_hex(ems_char code)
{
	static ems_char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

ems_char *url_encode(ems_cchar *str, ems_int lstr)
{
	ems_char  *buf = NULL, *pbuf, ch;

	buf = (ems_char *)ems_malloc(lstr * 3 + 1);
	if (!buf)
		return NULL;

	pbuf = buf;

	while (*str && lstr-- > 0) {
		ch = *str;
		str++;

		if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') 
			*pbuf++ = ch;
		else if (ch == ' ')
			*pbuf++ = '+';
		else 
			*pbuf++ = '%', *pbuf++ = to_hex(ch >> 4), *pbuf++ = to_hex(ch & 15);
	}
	*pbuf = '\0';

	return buf;
}

#if 0
ems_char *url_decode(ems_cchar *str, ems_char *buf)
{
	ems_char *pstr = str, *pbuf = buf;

	while (*pstr) {
		if (*pstr == '%') {
			if (pstr[1] && pstr[2]) {
				*pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
				pstr += 2;
			}
		} else if (*pstr == '+') { 
			*pbuf++ = ' ';
		} else {
			*pbuf++ = *pstr;
		}

		pstr++;
	}
	*pbuf = '\0';

	return buf;
}
#endif
