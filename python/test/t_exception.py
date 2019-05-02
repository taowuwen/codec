#!/usr/bin/env python3
# -*- coding: utf-8 -*-

fl="a.txt"

def test_try_finally():

    try:
        with open(fl, "r+") as f:
            print(f.read())

    finally:
        print("end of test")

def test_try_except():

    try:
        with open(fl, "r+") as f:
            print(f.read())

    except FileNotFoundError as e:
        print(e)

    finally:
        print("end of test")

def test_try_except_else():
    try:
        print("just try")

    except FileNotFoundError as e:
        print(e)

    else:
        print("ready to go, in else")


def test_try_except_else_finally():
    try:
        print("hello")
        with open(fl, "r+") as f:
            print(f.read())

    except FileNotFoundError as e:
        print(e)

    else:
        print("ready to go, in else")

    finally:
        print("end of test try except else and finally")


if __name__ == '__main__':

    try:
        test_try_finally()
    except Exception as e:
        pass

    test_try_except()
    test_try_except_else()
    test_try_except_else_finally()
