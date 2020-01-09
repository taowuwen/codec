#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tkinter


class MyGuiTest:

    def __init__(self):
        self.top = tkinter.Tk()


    def do_init_ui(self):
        pass


    def run(self):
        self.top.mainloop()


def main():
    gui = MyGuiTest()

    gui.run()


if __name__ == '__main__':
    main()
