#!/usr/bin/env python3

import sysv_ipc


class FgwCtrl:
    def __init__(self):
        self._mq = sysv_ipc.MessageQueue(0x1000)

    def sendmsg(self, msg):
        self._mq.send(msg)


def main():

    fgwctl = FgwCtrl()
    fgwctl.sendmsg('hello, world')

if __name__ == '__main__':
    main()
