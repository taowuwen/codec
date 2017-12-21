#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import absolute_import
from urllib.parse import urlparse
import parser.quanben as quanben
import parser.qb5 as qb5
import sys

__all__ = ['get_parser']

parser_info = {
    quanben.Quanben._url_path : quanben,
    qb5.Qb5._url_path : qb5
}

def get_parser(url):
    mod = parser_info.get(urlparse(url).netloc, None)

    if mod:
        return mod

    sys.stderr.write("No parser for {}, exit\n".format(url))
    sys.stderr.write("Current supportted Parsers HOST: \n")
    for item in parser_info:
        sys.stderr.write("\t{}\n".format(item))

    sys.stderr.write("\n")
    sys.stderr.flush()

    sys.exit(1)
