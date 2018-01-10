#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import array
import binascii

from test_utils import timeit

@timeit
def _test2():

    a = array.array('i', range(3))
    print("Original    : ", a)
    print("Hex Version : ", binascii.hexlify(a))

    a.extend(range(3))
    print("extended    : ", binascii.hexlify(a))

    print("Sliced : ", a[2:5])
    print("Sliced Hex  : ", binascii.hexlify(a[2:5]))

    print("Iterator    : ", list(enumerate(a)))


@timeit
def _test_file():
    import tempfile

    a = array.array('i', range(5))
    print("Original    ", a)

    output = tempfile.NamedTemporaryFile()

    a.tofile(output.file)

    output.flush()


    with open(output.name, 'rb') as f:

        raw_s = f.read()
        print('Readed content :', binascii.hexlify(raw_s))

        f.seek(0)

        a2 = array.array('i')

        a2.fromfile(f, len(a))
        print('Input is   ', a2)


@timeit
def _test_bytes():

    a = array.array('i', range(5))

    raw_b = a.tobytes()
    print(raw_b)
    print('Hex : ', binascii.hexlify(raw_b))

    a2 = array.array('i')
    a2.frombytes(raw_b)
    print('bytes: ', a2)
    print('Hex : ', binascii.hexlify(a2))


@timeit
def _test_ordering():


    def to_hex(a):

        size_per_item = a.itemsize * 2
        chunk = binascii.hexlify(a)
        chunk_size = len(chunk)

        for pos in range(0, chunk_size, size_per_item):
            yield chunk[pos:pos + size_per_item]

    start = int('0x12345678', 16)
    end   = start + 5

    a1 = array.array('i', range(start, end))
    a2 = array.array('i', range(start, end))
    a2.byteswap()

    fmt = '{:>12}' * 4
    print(fmt.format('A1 Hex', 'A1', 'A2 Hex', 'A2'))
    print(fmt.format('-'*12,'-'*12,'-'*12,'-'*12))

    fmt = '{!r:>12}{:>12}' * 2
    for value in zip(to_hex(a1), a1, to_hex(a2), a2):
        print(fmt.format(*value))


@timeit
def _test1():

    s = b'This is a array'
    a = array.array('b', s)

    print('As byte string :', s)
    print('As Array :',     a)
    print('As Hex :', binascii.hexlify(a))


if __name__ == '__main__':
    print("hello, test array...")
    _test1()
    _test2()
    _test_file()
    _test_bytes()
    _test_ordering()

