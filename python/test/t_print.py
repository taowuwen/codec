#!/usr/bin/env python3
# -*- coding: utf-8 -*-


def main():

    print()
    for n in range(1, 101):
        if n % 3 == 0 or n % 5 == 0:
            if n % 3 == 0 and n % 5 == 0:
                print("Fizz-Buzz", end=" ")
            elif n % 3 == 0:
                print("Fizz", end=" ")
            else:
                print("Buzz", end=" ")

        else:
            print(n, end=" ")

    print()





if __name__ == '__main__':
    main()
