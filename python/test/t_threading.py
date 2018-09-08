#!/usr/bin/env python3
# -*- coding: utf-8 -*-


def do_test_try_catch():

    a = 1
    try:

        print("hello, a")
        a = 2
        return a
    except:
        print("catched...")
        return a -1 
    else:
        print("else...")
        a = 3
    finally:
        print("finally...")
        a = 10
    print("a = {}".format(a))
    return a


def main():

    print(do_test_try_catch())


if __name__ == '__main__':
    print("test for threading...")
    main()
