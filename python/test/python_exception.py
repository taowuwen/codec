#!/usr/bin/env python3


def do_exception_test(tel = 0):
	try:
		assert tel < 3, "tel {tel} should less equal than 2 {}".format("ERROR", tel=tel)
		print("hello, world {tel}\n".format(tel=tel))

		#open("/var/log/dmesg", "r");
		#raise FileNotFoundError

	except (PermissionError, AssertionError) as e:
		print("AssertionError Or PermissionError " + str(e));

	else:
		print("in else...")

	finally:
		print("finally")


do_exception_test(2)
do_exception_test(3)
