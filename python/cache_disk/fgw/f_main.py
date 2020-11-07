#!/usr/bin/env python3

import os
import sys

from f_queue import FilePriorityQueue
from f_command import fgwcommand
from f_tool import FileTool
from fgw import FGW
from f_fuse import f_fuse_init
from fr import FileRouter
from f_event import FGWEventFactory
from f_disk import DiskManager
from f_file import file_system
from f_disk_oper import disk_oper_register_all_event
from f_file_oper import file_oper_register_all_event

def main():
    '''
    1. start tool server to recv msg from tool
    2. start fgw
    3. start timers
    '''

    print('Cachedisk version 1.0, created by taowuwen@gmail.com, 20201004')

    queue = FilePriorityQueue('fgw')

    f_fuse_init(queue)
    tool = FileTool(queue)
    tool.start()

    fr = FileRouter(queue)
    diskmgr = DiskManager(queue)

    fgwcommand.mq_fgw = queue
    fgwcommand.disk_mgr = diskmgr
    fgwcommand.register_all()
    disk_oper_register_all_event()
    file_oper_register_all_event()

    fr.subscribe(diskmgr)
    file_system.subscribe(diskmgr)

    FGW(queue).run()

if __name__ == '__main__':
    sys.exit(not main())
