#!/usr/bin/env python3

from cache_disk import fgwctl_out, fgwctl_in
import sysv_ipc
import sys
import os
import time

class FgwCtrl:
    def __init__(self):
        self._mq_tx = sysv_ipc.MessageQueue(fgwctl_out)
        self._mq_rx = sysv_ipc.MessageQueue(fgwctl_in)

        while True:
            try:
                self._mq_rx.receive(0)
            except sysv_ipc.BusyError:
                break

    def sendmsg(self, msg):
        self._mq_tx.send(msg)
        _msg, _type = self._mq_rx.receive()

        print(f'Result: Type: {_type}, Msg: {_msg.decode()}')

def main():

    args = ' '.join(sys.argv[1:])
    fgwctl = FgwCtrl()
    fgwctl.sendmsg(args)

if __name__ == '__main__':
    main()
