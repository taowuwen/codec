#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tkinter
import sys
import os

class ScrolledText(tkinter.Frame):

    def __init__(self, parent=None, text='', file=None):

        super(ScrolledText, self).__init__(parent)

        self.pack(expand=tkinter.YES, fill=tkinter.BOTH)

        self.make_widgets()
        self.set_text(text, file)

    def make_widgets(self):

        sbar = tkinter.Scrollbar(self)
        text = tkinter.Text(self, relief=tkinter.SUNKEN)

        sbar.config(command=text.yview)
        text.config(yscrollcommand=sbar.set)

        sbar.pack(side=tkinter.RIGHT, fill=tkinter.Y)
        text.pack(side=tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)

        self.text=text

    def set_text(self, text='', file=None):

        if file:
            with open(file, 'r') as f:
                text=f.read()

        self.text.delete('1.0', tkinter.END)
        self.text.insert('1.0', text)
        self.text.mark_set(tkinter.INSERT, '1.0')
        self.text.focus()

    def gettext(self):
        return self.text.get('1.0', tkinter.END+'-1c')



if __name__ == '__main__':
    root =  tkinter.Tk()

    if len(sys.argv) > 1:
        st = ScrolledText(root, file=sys.argv[1])

    else:
        st = ScrolledText(root, text='hello\nworld')


    def show(event):
        txt = st.gettext()
        for l in txt:
            print(l)
        #print(repr(st.gettext()))

    root.bind('<Key-Escape>', show)
    root.mainloop()
