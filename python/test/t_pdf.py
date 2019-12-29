#!/usr/bin/env python3
#-*- coding: utf-8 -*-

import pdfkit
import sys
import PyPDF2


def main():


    # pdfkit.from_url('www.baidu.com', 'out.url.baidu.pdf')
    pdfkit.from_file('/tmp//python-patterns.guide/index.html', 'out.html.pdf')
    # pdfkit.from_string('hello, my pdf test', 'out.string.pdf')


if __name__ == '__main__':
    sys.exit(not main())

