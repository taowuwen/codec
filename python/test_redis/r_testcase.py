#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import redis
from r_utils import logger, decode_str


def get_parser(a):
    global parser_tbl
    return parser_tbl.get(type(a), parse_passthrough)


def parse_str(s):
    if s.isdigit():
        return int(s)

    return s

def parse_bytes(s):
    return parse_str(decode_str(s))

def parse_list(l):

    rsp_l = []

    for i in l:
        parser = get_parser(i)
        rsp_l.append(parser(i))

    return rsp_l

def parse_set(s):
    return set(parse_list(s))

def parse_tuple(s):
    return tuple(parse_list(s))

def parse_dict(d):

    new_dict = {}

    for k, v in d.items():
        parser_k = get_parser(k)
        parser_v = get_parser(v)
        new_dict[parser_k(k)] = parser_v(v)

    return new_dict

parse_passthrough = lambda x:x

parser_tbl = {
        type(1): parse_passthrough,
        type(1.0): parse_passthrough,
        type('a'): parse_str,
        type(True): parse_passthrough,
        type(None): parse_passthrough,
        type(b'1'): parse_bytes,
        type(list()): parse_list,
        type(set()): parse_set,
        type(tuple()): parse_tuple,
        type(dict()): parse_dict,
}

def parse_response(rsp):
    parser = get_parser(rsp)
    return parser(rsp)

class Testcase(redis.Redis):

    def execute_command(self, *args, **kwargs):
        try:
            rsp = super().execute_command(*args, **kwargs) or None
        except redis.exceptions.ResponseError as e:
            logger.error(f"{e} > {args}, {kwargs}")
            return None
        else:
            try:
                logger.trace(f"> {args}, {kwargs} ---> {parse_response(rsp)}")
            except Exception as e:
                logger.error(f"> {args}, {kwargs} ---> {rsp} {type(rsp)} {e}")
            return rsp
        

    def select_db(self, db=15):

        if hasattr(self, "select"):
            return getattr(self, "select")(db)

        return self.execute_command('SELECT', db)

    @property
    def db(self):

        clnt_id = self.client_id()
        for clnt in self.client_list():
            if int(clnt['id']) == clnt_id:
                return int(clnt['db'])

        assert 0, "bugs or server down should never show up this line"


    def prepare(self):
        self.key = f'{self}'
        self.val = self.key + "-" + self.key
        pass

    def run(self):

        for tc in dir(self):
            if tc.startswith('do_test_'):
                logger.info(f'>>>>>>>>>>> {tc}')
                testcase = getattr(self, tc)
                testcase()

    def clean(self):
        pass

    def __str__(self):
        return f'{self.__class__.__name__}'

