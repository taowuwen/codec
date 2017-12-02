#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import io
import mimetypes
from urllib import request
import uuid


class Multiformed:

    def __init__(self):
        self.filed_formed = []
        self.file = []
        self.boundary = uuid.uuid4().hex.encode("utf-8")

    def get_content_type(self):
        return 'multipart/form-data; boundary={}'.format(
            self.boundary.decode('utf-8'))

    def add_filed(self, name, value):
        self.filed_formed.append((name, value))

    def add_file(self, filedname, filename, filehandle, mimetype=None):

        data = filehandle.read()

        if mimetype is None:
            mimetype = (
                mimetypes.guess_type(filename)[0] or
                'application/octet-stream'
            )

        self.file.append((filedname, filename, data, mimetype))

    @staticmethod
    def _form_data(name):
        return ('Content-Disposition: form-data; '
                'name="{}"\r\n').format(name).encode('utf-8')

    @staticmethod
    def _attached_file(name, filename):
        return ('Content-Disposition: file; '
                'name="{}"; filename="{}"\r\n').format(
                        name, filename).encode('utf-8')

    @staticmethod
    def _content_type(ct):
        return 'Content-Type: {}\r\n'.format(ct).encode('utf-8')

    def __bytes__(self):

        buffer = io.BytesIO()

        boundary = b'-----' + self.boundary + b'\r\n'

        for name, value in self.filed_formed:
            buffer.write(boundary)
            buffer.write(self._form_data(name))
            buffer.write(b'\r\n')
            buffer.write(value.encode('utf-8'))
            buffer.write(b'\r\n')

        for name, filename, data, mimetype in self.file:
            buffer.write(boundary)
            buffer.write(self._attached_file(name, filename))
            buffer.write(self._content_type(mimetype))
            buffer.write(b'\r\n')
            buffer.write(data)
            buffer.write(b'\r\n')

        buffer.write(b'--' + self.boundary + b'--\r\n')
        return buffer.getvalue()


def main():
    print("hello, my uploading")

    mul = Multiformed()
    print(str(mul.boundary))

    mul.add_filed("hello", "world")
    mul.add_filed("foo", "bar")

    mul.add_file('biography', 'bio.txt',
                 io.BytesIO(b'Python developer and blogger.'))

    data = bytes(mul)

    r = request.Request("http://localhost:8000/cgi-bin/hello.py", data=data)
    r.add_header('User-Agent', 'TWW-AGENT')
    r.add_header('Content-Type', mul.get_content_type())
    r.add_header('Content-Length', len(data))

    print()
    print("OUTGOING data:")

    for name, value in r.header_items():
        print("{} : {}".format(name, value))

    print()
    print(r.data.decode('utf-8'))

    print()
    print('SERVER RESPONSE:')
    print(request.urlopen(r).read().decode('utf-8'))


if __name__ == '__main__':
    main()
