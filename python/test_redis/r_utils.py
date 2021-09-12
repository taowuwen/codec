#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import time
try:
    from dbgview import DebugStringPrint
except Exception as e:
    def DebugStringPrint(*args, **kwargs): pass

class DecodeFailed(Exception): pass

def decode_str(s):

    if not s:
        return None

    codec = ('utf-8', 'gb2312', 'gbk')

    for c in codec:
        try:
            return s.decode(c)
        except UnicodeDecodeError:
            pass
        except Exception as e:
            raise e

    raise DecodeFailed(f"decode {s} failed")


class Color:
    red = "\033[1;31m"
    green = "\033[1;32m"
    blue = "\033[1;34m"
    yellow = "\033[1;33m"
    normal = "\033[1;00m"

class DebugLevel:
    dbg = 0
    info = 1
    notice = 2
    warn = 3
    err  = 4


class Logger:

    def __init__(self, fd=sys.stdout, level=DebugLevel.info):
        self._level = level
        self._fd = sys.stdout


    def _do_print(self, level, color, prefix, s):

        if level >= self._level:
            print(f"{color} {time.asctime()}    {prefix}\t{s}\033[m")
            DebugStringPrint("{:<8s}{}".format(prefix, s))

    def debug(self, s):
        self._do_print(DebugLevel.dbg, Color.normal, "debug", s)

    def info(self, s):
        self._do_print(DebugLevel.info, Color.green, "info", s)

    def notice(self, s):
        self._do_print(DebugLevel.notice, Color.yellow, "notice", s)

    def warn(self, s):
        self._do_print(DebugLevel.warn, Color.blue, "warn", s)

    def error(self, s):
        self._do_print(DebugLevel.err, Color.red, "error", s)

    trace = debug

    @property
    def level(self):
        return self._level

    @level.setter
    def level(self, val):
        self._level = val


logger = Logger()



if __name__ == '__main__':

    logger.trace(f'hello, logger , level: {logger.level}')
    logger.debug(f'hello, logger , level: {logger.level}')
    logger.info(f'hello, logger , level: {logger.level}')
    logger.notice(f'hello, logger , level: {logger.level}')
    logger.warn(f'hello, logger , level: {logger.level}')
    logger.error(f'hello, logger , level: {logger.level}')

    logger.level = DebugLevel.notice

    logger.trace(f'hello, logger , level: {logger.level}')
    logger.debug(f'hello, logger , level: {logger.level}')
    logger.info(f'hello, logger , level: {logger.level}')
    logger.notice(f'hello, logger , level: {logger.level}')
    logger.warn(f'hello, logger , level: {logger.level}')
    logger.error(f'hello, logger , level: {logger.level}')

    logger.level = DebugLevel.dbg
    logger.trace(f'hello, logger , level: {logger.level}')
    logger.debug(f'hello, logger , level: {logger.level}')
    logger.info(f'hello, logger , level: {logger.level}')
    logger.notice(f'hello, logger , level: {logger.level}')
    logger.warn(f'hello, logger , level: {logger.level}')
    logger.error(f'hello, logger , level: {logger.level}')
