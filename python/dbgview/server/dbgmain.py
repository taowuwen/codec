#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import argparse
import queue
import threading

from dbggui    import DbgView
from dbgserver import DbgServerThread
from dbgfilter import DbgFilterThread
from dbgdata   import DbgData
from dbgconfig import DbgConfig


class App:

    def __init__(self, *kargs, **kwargs):

        '''
        message queue: 
            1. gui received from msg from "DATA" domain: msgqueue1
            2. filter received from both server and "DATA" domain: msgqueue 2
            3. "DATA" domain send msg to gui via msgqueue1
            4. "DATA" domain send msg to filter via msgqueue2
            5. server send msg to filter vis msgqueue 2
        '''
        self.msgqueue1 = queue.Queue(1024)
        self.msgqueue2 = queue.Queue(1024)
        self.datactl = DbgData(self.msgqueue1, *kargs, **kwargs)
        self.cfgctl = DbgConfig(self.msgqueue1, self.msgqueue2, *kargs, **kwargs)

        # signals catch from here
        self.start_server(self.msgqueue2, *kargs, **kwargs)
        self.start_filter(self.datactl, self.msgqueue2, *kargs, **kwargs)
        self.init_gui(self.datactl, self.cfgctl, self.msgqueue1, *kargs, **kwargs)

    def start_server(self, *kargs, **kwargs):
        print(*kargs, **kwargs)
        self.server_thread = DbgServerThread(*kargs, **kwargs)
        self.server_thread.start()

    def start_filter(self, *kargs, **kwargs):
        print(*kargs, **kwargs)
        self.filter_thread = DbgFilterThread(*kargs, **kwargs)
        self.filter_thread.start()

    def init_gui(self, *kargs, **kwargs):
        print(*kargs, **kwargs)
        self.gui = DbgView(*kargs, **kwargs)

    def run(self):
        self.gui.run()
        self.filter_thread.stop()
        self.server_thread.stop()
        self.filter_thread.join()
        self.server_thread.join()


def main():
    # do argparse goes from here
    # do init goes from here
    # start Server
    # start Filter
    # start gui
    # running
    return App().run()


if __name__ == '__main__':
    sys.exit(not main())
