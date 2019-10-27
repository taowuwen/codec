#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import redis
from r_utils import logger

class Testcase(redis.Redis):

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

