#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import argparse
import queue
import threading
import platform

from dbggui    import DbgView
from dbgserver import DbgServerThread
from dbgfilter import DbgFilterThread
from dbgdata   import DbgData
from dbgconfig import DbgConfig
from dbgaction import ActionManagement
from dbgctl import dbg_ctrl_init, dbg_ctrl_notify_create
from dbgintf import dbg_intf_init
from dbgactiondef import dbg_print_init, dbg_print


class App:

    def __init__(self, *kargs, **kwargs):

        '''
        acl management
            action lists:
            filter lists:
        message queue: 
            1. gui received from msg from "DATA" domain: msgqueue1
            2. filter received from both server and "DATA" domain: msgqueue 2
            3. "DATA" domain send msg to gui via msgqueue1
            4. "DATA" domain send msg to filter via msgqueue2
            5. server send msg to filter vis msgqueue 2
        '''
        self.msgqueue1 = queue.Queue(2048)
        self.msgqueue2 = queue.Queue(2048)
        dbg_print_init(self.msgqueue1)

        self.datactl = DbgData(self.msgqueue1, *kargs, **kwargs)
        self.actionctl = ActionManagement()

        dbg_ctrl_init()
        dbg_intf_init(self.actionctl)
        dbg_ctrl_notify_create()

        self.actionctl.refresh_table()

        # signals catch from here
        self.start_server(self.msgqueue2, *kargs, **kwargs)
        self.start_filter(self.datactl, self.msgqueue2, self.actionctl, *kargs, **kwargs)
        self.init_gui(self.datactl, self.msgqueue1, self.actionctl, self, *kargs, **kwargs)

    def start_server(self, *kargs, **kwargs):
        self.server_thread = DbgServerThread(*kargs, **kwargs)
        self.server_thread.start()

    def start_filter(self, *kargs, **kwargs):
        self.filter_thread = DbgFilterThread(*kargs, **kwargs)
        self.filter_thread.start()

    def init_gui(self, *kargs, **kwargs):
        self.gui = DbgView(*kargs, **kwargs)

    def run(self):
        self.gui.run()
        self.filter_thread.stop()
        self.server_thread.stop()
        self.filter_thread.join()
        self.server_thread.join()

    def app_exec(self, cmd):
        
        self.exec_table = None

        if not self.exec_table:
            self.exec_table = {
                    "server.show": self.server_thread.dbg_show,
                    "server.clear": self.server_thread.dbg_clear,
                    "filter.show": self.filter_thread.dbg_show,
                    "filter.clear": self.filter_thread.dbg_clear
                }

        func = self.exec_table.get(cmd)
        if func:
            return func()
        else:
            dbg_print(f'{cmd} not found')


def main():

    if platform.system() == 'Windows':
        import ctypes
        try:
            import win32process
        except Exception as e:
            print(e)
        else:
            hwnd = ctypes.windll.kernel32.GetConsoleWindow()      
            if hwnd != 0:      
                ctypes.windll.user32.ShowWindow(hwnd, 0)      
                ctypes.windll.kernel32.CloseHandle(hwnd)
                _, pid = win32process.GetWindowThreadProcessId(hwnd)
                os.system('taskkill /PID ' + str(pid) + ' /f')

    return App().run()

if __name__ == '__main__':
    sys.exit(not main())
