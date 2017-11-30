#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import string


def test_string_safe_replace():
	values = {'var': 'foo'}

	t = string.Template("$var is here but $missing is not provided")

	try:
		print('substitute()     :', t.substitute(values))
	except KeyError as err:
		print('ERROR:', str(err))
	print('safe_substitute():', t.safe_substitute(values))

def test_string_template():
	values = {'var': 'foo'}

	t = string.Template("""
			Variable        : $var
			Escape          : $$
			Variable in text: ${var}iable
			""")

	print('TEMPLATE:', t.substitute(values))

	s = """
	Variable        : %(var)s
	Escape          : %%
	Variable in text: %(var)siable
	"""

	print('INTERPOLATION:', s % values)

	s = """
	Variable        : {var}
	Escape          : {{}}
	Variable in text: {var}iable
	"""

	print('FORMAT:', s.format(**values))


def main():
	print("hello, string testing")
	test_string_template()
	test_string_safe_replace();


if __name__ == '__main__':
	main()

