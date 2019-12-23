#!/usr/bin/env python3
#-*- coding: utf-8 -*-


import glob
from PyPDF2 import PdfFileWriter, PdfFileReader

def merger(output_path, input_paths):
    pdf_writer = PdfFileWriter()

    for path in input_paths:
        pdf_reader = PdfFileReader(path)

        for page in range(pdf_reader.getNumPages()):
            pdf_writer.addPage(pdf_reader.getPage(page))

    with open(output_path, 'wb') as fh:
        pdf_writer.write(fh)


if __name__ == '__main__':

    p = '/home/tww/mount/resources/ebooks/Network/IPv6'

    paths = glob.glob(f'{p}/*.PDF')
    paths.sort()
    for k in paths:
        print(k)

    merger(f'{p}/IPV6详解.pdf', paths)
