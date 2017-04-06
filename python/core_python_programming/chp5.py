#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tkinter

top = tkinter.Tk()


# Label
#
label = tkinter.Label(top, text="hello, world")
label.pack()


# button
btn = tkinter.Button(top, text = "QUIT", 
		command=top.quit, bg="red", fg = "white")
btn.pack()




tkinter.mainloop()
