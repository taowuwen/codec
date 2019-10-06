#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import redis
import sys
import os

print(dir(redis))


class Logger:

    red = "\033[1;31m"
    green = "\033[1;32m"
    blue = "\033[1;34m"
    yellow = "\033[1;33m"

    @staticmethod
    def do_print(color, s):
        print(f'{color} {s}\033[m')

    @staticmethod
    def print_err(s):
        Logger.do_print(Logger.red, "err\t " + s)

    @staticmethod
    def print_debug(s):
        Logger.do_print(Logger.green, "dbg\t " + s)

    @staticmethod
    def print_info(s):
        Logger.do_print(Logger.blue, "info\t" + s)

    @staticmethod
    def print_warn(s):
        Logger.do_print(Logger.yellow, "warn\t" + s)


class TestRedis(redis.Redis):

    def print_methods(self):

        cmds = [ cmd for cmd in dir(self) if not cmd.startswith('_') and not cmd.startswith('redis_test_func_') ]

        cmds.remove('RESPONSE_CALLBACKS')

        for idx, cmd in enumerate(cmds):
            Logger.print_debug(f"{idx} : {cmd}")


    def run_test(self):

        for cmd in dir(self):

            if cmd.startswith('redis_test_func_'):
                Logger.print_info(f"run test method: {cmd}")
                try:
                    eval(f'self.{cmd}()')
                except AssertionError as e:
                    Logger.print_err(f"\033[1;31merror in {cmd} : {e}\033[m")
                    return


    def redis_test_func_generic(self):

        Logger.print_debug(str([a.decode() for a in self.keys('*')]))


    def redis_test_func_string(self):
        k = 'foo'
        v = 'bar'
        
        self.set(k, v.encode())

        assert self.get(k).decode() == v, f"Error: value should equal to '{v}'"

    def redis_test_func_list(self):
        pass

    def redis_test_func_set(self):
        pass

    def redis_test_func_sorted_set(self):
        pass

    def redis_test_func_hash(self):
        pass

    def redis_test_func_pubsub(self):
        pass

    def redis_test_func_transactions(self):
        pass

    def redis_test_func_connection(self):
        pass

    def redis_test_func_server(self):
        pass

    def redis_test_func_scripting(self):
        pass

    def redis_test_func_hyperloglog(self):
        pass

    def redis_test_func_cluster(self):
        pass

    def redis_test_func_geo(self):
        pass

    def redis_test_func_stream(self):
        pass




if __name__ == '__main__':

    print("hello, test redis goes from here")


    tr = TestRedis(host='10.10.10.11')

    tr.print_methods()
    tr.run_test()

    tr.close()

