#!/usr/bin/env python3

import os
import sys

from f_queue import FilePriorityQueue
from f_tool import FileTool
from fgw import FGW

def main():
    '''
    1. start tool server to recv msg from tool
    2. start fgw
    3. start timers
    '''

    print('Cachedisk version 1.0, created by taowuwen@gmail.com, 20201004')

    queue = FilePriorityQueue('fgw')

    tool = FileTool(queue)
    tool.start()

    FGW(queue).run()

if __name__ == '__main__':
    sys.exit(not main())
