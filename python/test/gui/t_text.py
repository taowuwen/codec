#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import tkinter


class App():
    def __init__(self):
        self.root = tkinter.Tk()
        self.root.geometry('1024x768')

        text = tkinter.Text(self.root)

        text.insert(tkinter.END, 'hello....\n')
        text.insert(tkinter.END, 'world....\n')

        text.pack(side=tkinter.TOP, expand=True, fill=tkinter.BOTH)

    def run(self):
        self.root.mainloop()

if __name__ == '__main__':
    App().run()


