#!/usr/bin/env python3

import os
import sys

from f_queue import FilePriorityQueue
from f_command import fgwcommand
from f_tool import FileTool
from fgw import FGW
from f_fuse import f_fuse_init

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

    fgwcommand.mq_fgw = queue
    fgwcommand.register_all()

    FGW(queue).run()

if __name__ == '__main__':
    sys.exit(not main())
