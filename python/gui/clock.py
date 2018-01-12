#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import tkinter
import time

class App:
    def __init__(self):

        self.root = tkinter.Tk()
        self.root.geometry('1024x768')
        self.root.title('my test for gui programming or components...')

        self.label = tkinter.Label(text='')
        self.label.pack()
        self.update_clock()

    def run(self):
        self.root.mainloop()

    def update_clock(self):
        now = time.strftime("%H:%M:%S")
        self.label.configure(text=now)
        self.root.after(1000, self.update_clock)

if __name__ == '__main__':
    app = App()
    app.run()
