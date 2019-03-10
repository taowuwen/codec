#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import absolute_import
from urllib.parse import urlparse
import parser.quanben as quanben
import parser.qb5 as qb5
import parser.jueshitangmen as jueshitangmen
import parser.c23us as c23us
import parser.biquge as biquge
import parser.booktxt as booktxt
import parser.biquke as biquke
import parser.longwangchuanshuo as duoluodalu
import parser.luoxia as luoxia
import parser.qiushuge as qiushuge
import sys

__all__ = ['get_parser']

parser_info = {
    quanben._url_path : quanben,
    qb5._url_path : qb5,
    jueshitangmen._url_path: jueshitangmen,
    c23us._url_path: c23us,
    biquge._url_path: biquge,
    biquke._url_path: biquke,
    duoluodalu._url_path: duoluodalu,
    luoxia._url_path: luoxia,
    qiushuge._url_path: qiushuge,
    booktxt._url_path: booktxt
}

def get_parser(url):
    parsed = urlparse(url)
    mod = parser_info.get(parsed.netloc, None)

    if mod:
        mod._url_root = parsed.scheme + mod._url_path
        return mod

    sys.stderr.write("No parser for {}, exit\n".format(url))
    sys.stderr.write("Current supportted Parsers HOST: \n")
    for item in parser_info:
        sys.stderr.write("\t{}\n".format(item))

    sys.stderr.write("\n")
    sys.stderr.flush()

    sys.exit(1)
