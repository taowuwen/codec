#!/usr/bin/env python3
# -*- coding: utf-8 -*-



import selectors
import socket


outgoing=[
        b'hello, world',
        b'this is another msg'
]

bytes_sent = 0
bytes_received = 0

server_address = ('localhost', 10000)

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(server_address)
sock.setblocking(False)

mysel = selectors.DefaultSelector()
mysel.register(sock, selectors.EVENT_READ | selectors.EVENT_WRITE,)


keep_running = True

while keep_running:
    print('wait for I/O...')

    for key, mask in mysel.select(timeout=1):

        connection = key.fileobj
        client_address = connection.getpeername()
        print('client({!r})'.format(client_address))

        if mask & selectors.EVENT_READ:
            print('   ready to read')
            data = connection.recv(1024)

            if data:
                print('    received {!r}'.format(data))
                bytes_received += len(data)

            if not data or bytes_received and bytes_received == bytes_sent:
                keep_running = False

        if mask & selectors.EVENT_WRITE:
            print('   ready to write')
            
            if not outgoing:
                print('   switched to read-only')
                mysel.modify(sock, selectors.EVENT_READ)

            else:
                next_msg = outgoing.pop()

                print('    sending {!r}'.format(next_msg))
                connection.sendall(next_msg)
                bytes_sent += len(next_msg)

mysel.unregister(sock)
sock.close()
mysel.close()

