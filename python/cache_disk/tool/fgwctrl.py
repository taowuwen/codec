#!/usr/bin/env python3

from cache_disk import fgwctl_key
import sysv_ipc
import sys
import os

class FgwCtrl:
    def __init__(self):
        self._mq = sysv_ipc.MessageQueue(0x1000)

    def sendmsg(self, msg):
        self._mq.send(msg)
        _msg, _type = self._mq.receive()

        print(f'Result: Type: {_type}, Msg: {_msg.decode()}')



def main():

    args = ' '.join(sys.argv[1:])
    fgwctl = FgwCtrl()
    fgwctl.sendmsg(args)

if __name__ == '__main__':
    main()
