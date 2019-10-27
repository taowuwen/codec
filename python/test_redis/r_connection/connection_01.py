#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_utils import logger, decode_str
from . import TestcaseConnection
import redis

class ConnectionTest_001(TestcaseConnection):
    '''
    connection:
        auth
        echo
        ping
        quit
        select
        swapdb
    '''

    def prepare(self):
        super().prepare()

    def clean(self):
        self.delete(self.key)
        super().clean()

    def do_test_auth(self):
        try:
            self.execute_command('AUTH', self.key)
        except redis.exceptions.AuthenticationError as e:
            logger.warn(f"{type(e)}: auth failed {e}")


    def do_test_echo(self):
        val = decode_str(self.echo(self.key))
        logger.trace(f"{val}")

        
    def do_test_ping(self):
        val = self.ping()
        logger.trace(f"{val}")

    def do_test_select(self):

        backup_db = self.db

        logger.trace(f"current db {backup_db}")

        self.select_db(13)
        logger.trace(f"current db {self.db}")
        assert self.db == 13, "invalid db select"

        self.select_db(backup_db)
        logger.trace(f"current db {self.db}")
        assert self.db == backup_db, "invalid db select"



class ConnectionTest_002(TestcaseConnection):
    '''
    connection:
        swapdb
    '''

    def prepare(self):
        super().prepare()

        self.backup_db = self.db
        self.from_db =12
        self.to_db = 11

        self.select_db(self.from_db)
        self.set(self.key, self.val)

        self.select_db(self.to_db)
        self.set(self.key, self.val * 2)


    def clean(self):

        self.select_db(self.from_db)
        self.delete(self.key)

        self.select_db(self.to_db)
        self.delete(self.key)

        self.select_db(self.backup_db)
        self.delete(self.key)
        super().clean()

    def do_test_swapdb(self):

        self.select_db(self.from_db)
        val = decode_str(self.get(self.key))
        logger.trace(f"key: {self.key}, val: {self.val}, val_get: {val}")
        assert val == self.val, "val should equal to self.val"

        self.select_db(self.to_db)
        val = decode_str(self.get(self.key))
        logger.trace(f"key: {self.key}, val: {self.val * 2}, val_get: {val}")
        assert val == self.val * 2, "val should equal to self.val"


        self.swapdb(self.from_db, self.to_db)

        self.select_db(self.from_db)
        val = decode_str(self.get(self.key))
        logger.trace(f"key: {self.key}, val: {self.val * 2}, val_get: {val}")
        assert val == self.val * 2, "val should equal to self.val"

        self.select_db(self.to_db)
        val = decode_str(self.get(self.key))
        logger.trace(f"key: {self.key}, val: {self.val}, val_get: {val}")
        assert val == self.val, "val should equal to self.val"
