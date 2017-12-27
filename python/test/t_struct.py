#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import struct
import binascii

from test_utils import timeit


endianness = [
        ('@', 'native', 'native' ),
        ('=', 'native', 'standard'),
        ('>', 'big-endian'),
        ('<', 'little-endian'),
        ('!', 'network'),
]

def _get_struct(fmt=None):
    values = (1, 'ab'.encode('utf-8'), 2.7)

    if not fmt:
        fmt = 'I 2s f'

    s = struct.Struct(fmt)

    return (values, s)


@timeit
def _test_ctypes(fmt='I 2s f'):
    import ctypes

    values, s = _get_struct(fmt)

    b = ctypes.create_string_buffer(s.size)

    print('{:30} : {}'.format('Before', binascii.hexlify(b.raw)))

    s.pack_into(b, 0, *values)

    print('{:30} : {}'.format('After', binascii.hexlify(b.raw)))
    print('{:30} : {}'.format('unpacked', s.unpack_from(b, 0)))


@timeit
def _test_array(fmt = None):

    import array

    values, s = _get_struct(fmt)

    a = array.array('b', b'\0' * s.size)

    print('{:30} : {}'.format('Before', binascii.hexlify(a)))

    s.pack_into(a, 0, *values)

    print('{:30} : {}'.format('After', binascii.hexlify(a)))
    print('{:30} : {}'.format('unpacked', s.unpack_from(a, 0)))



@timeit
def _test_packed(fmt='I 2s f'):
    values = (1, 'ab'.encode('utf-8'), 2.7)

    s = struct.Struct(fmt)

    packed = s.pack(*values)

    print('{:30} : {}'.format('Original', values))
    print('{:30} : {}'.format('Format String', s.format))
    print('{:30} : {}'.format('Uses ' + str(s.size),  values))
    print('{:30} : {}'.format('Packed ', binascii.hexlify(packed)))

    return binascii.hexlify(packed)


@timeit
def _test_unpack(packed, fmt='I 2s f'):

    print(type(packed), packed)

    data = binascii.unhexlify(packed)

    s = struct.Struct(fmt)

    unpacked_data = s.unpack(data)

    print('Unpacked data: {}'.format(unpacked_data))

@timeit
def _test_endian():

    fmt = 'I 2s f'

    for endian in endianness:

        _fmt = "{} {}".format(endian[0], fmt)

        print('test {}.......'.format(endian))
        _test_unpack(_test_packed(_fmt), _fmt)


if __name__ == '__main__':
    print('hello, struct test...')
    packed = _test_packed()
    _test_unpack(packed)

    _test_endian()
    _test_ctypes('!I 2s f')
    _test_array('@I 2s f')
