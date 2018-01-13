#!/usr/bin/env python3
# -*- coding: utf-8

import tkinter
import time
import sys

v_label = None

class TestLabel:

    def __init__(self, root):
        self._root = root

        # self.test_side()
        # self.test_fill()
        # self.test_expand()
        self.test_anchor()
        #self.test_label()

    def test_side(self):
        tkinter.Label(self._root, text = 'left', fg="red", bg='#aabbcc').pack(side   = tkinter.LEFT)
        tkinter.Label(self._root, text = 'right', fg="blue", bg='#aabbcc').pack(side  = tkinter.RIGHT)
        tkinter.Label(self._root, text = 'top', fg="green").pack(side    = tkinter.TOP)
        tkinter.Label(self._root, text = 'bottom', fg="black").pack(side = tkinter.BOTTOM)

    def test_fill(self):
        tkinter.Label(self._root, text = 'left', fg="red", bg='#aabbcc').pack(side   = tkinter.LEFT, fill=tkinter.Y)
        tkinter.Label(self._root, text = 'right', fg="blue", bg='#aabbcc').pack(side  = tkinter.RIGHT, fill=tkinter.Y)
        tkinter.Label(self._root, text = 'top', fg="green", bg='#aabbcc').pack(side    = tkinter.TOP, fill=tkinter.X)
        tkinter.Label(self._root, text = 'bottom', fg="black", bg='#aabbcc').pack(side = tkinter.BOTTOM, fill=tkinter.X)

    def test_expand(self):
        tkinter.Label(self._root, text = 'top', fg="green", bg='#aabb00').pack(side    = tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(self._root, text = 'left', fg="red", bg='#aabbcc').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(self._root, text = 'right', fg="blue", bg='#aabbcc').pack(side  = tkinter.RIGHT, fill=tkinter.Y, expand=tkinter.YES)
        tkinter.Label(self._root, text = 'top2', fg="green", bg='#aabbcc').pack(fill=tkinter.BOTH, expand=tkinter.YES)
        #tkinter.Label(self._root, text = 'bottom', fg="black", bg='#aabbcc').pack(side = tkinter.BOTTOM, fill=tkinter.BOTH, expand=tkinter.YES)

    def test_anchor(self):

        frame = tkinter.Frame(self._root, bg='#aa0000')
        frame.pack(side = tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)

        tkinter.Label(frame, text = 't,l', fg="red", bg='#00bbcc').pack(side   = tkinter.LEFT, anchor=tkinter.NW, expand=tkinter.YES)
        tkinter.Label(frame, text = 't,m', fg="red", bg='#00bbcc').pack(side   = tkinter.LEFT, anchor=tkinter.N,  expand=tkinter.YES)
        tkinter.Label(frame, text = 't,r', fg="red", bg='#00bbcc').pack(side   = tkinter.LEFT, anchor=tkinter.NE, expand=tkinter.YES)

        frame = tkinter.Frame(self._root, bg='#00aa00')
        frame.pack(side = tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)

        frame_left = tkinter.Frame(frame, bg='#00aa00')
        frame_left.pack(side = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)

        frame_middle = tkinter.Frame(frame, bg='#00aa00')
        frame_middle.pack(side = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)

        frame_right = tkinter.Frame(frame, bg='#00aa00')
        frame_right.pack(side = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)

        tkinter.Label(frame_left, text = 'm,l', fg="blue", bg='#aa00cc').pack(side   = tkinter.LEFT, anchor=tkinter.CENTER, expand=tkinter.NO)
        tkinter.Label(frame_middle, text = 'm,m', fg="blue", bg='#aa00cc').pack(side   = tkinter.LEFT, anchor=tkinter.CENTER, expand=tkinter.YES)
        tkinter.Label(frame_right, text = 'm,r', fg="blue", bg='#aa00cc').pack(side   = tkinter.RIGHT, anchor=tkinter.CENTER, expand=tkinter.NO)


        frame = tkinter.Frame(self._root, bg='#0000aa')
        frame.pack(side = tkinter.BOTTOM, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 'b,l', fg="green", bg='#aabb00').pack(side   = tkinter.LEFT, anchor=tkinter.SW, expand=tkinter.YES)
        tkinter.Label(frame, text = 'b,m', fg="green", bg='#aabb00').pack(side   = tkinter.LEFT, anchor=tkinter.S , expand=tkinter.YES)
        tkinter.Label(frame, text = 'b,r', fg="green", bg='#aabb00').pack(side   = tkinter.LEFT, anchor=tkinter.SE, expand=tkinter.YES)

    def test_label(self):

        frame = tkinter.Frame(self._root, bg='#aa0000')
        frame.pack(side = tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)

        tkinter.Label(frame, text = 't,l', fg="red", bg='#00bbcc').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 't,m', fg="red", bg='#00bbcc').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 't,r', fg="red", bg='#00bbcc').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)

        frame = tkinter.Frame(self._root, bg='#00aa00')
        frame.pack(side = tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 'm,l', fg="blue", bg='#aa00cc').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 'm,m', fg="blue", bg='#aa00cc').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 'm,r', fg="blue", bg='#aa00cc').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)


        frame = tkinter.Frame(self._root, bg='#0000aa')
        frame.pack(side = tkinter.BOTTOM, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 'b,l', fg="green", bg='#aabb00').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 'b,m', fg="green", bg='#aabb00').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Label(frame, text = 'b,r', fg="green", bg='#aabb00').pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)



class ButtonTest:

    def __init__(self, root):

        frame = tkinter.Frame(root, bg='#bbaacc')
        frame.pack(side = tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)
        self._root = frame

        #self.test_button_sys_exit()
        #self.test_quit()
        #self.test_lambda()

        self.test_button()

    def test_button_sys_exit(self):
        btn = tkinter.Button(self._root, text = 'hello, button', command=sys.exit)
        btn.pack(expand=tkinter.YES, fill=tkinter.X)

    def test_quit(self):

        btn = tkinter.Button(self._root, text = "quit", command=self.quit, fg="red")
        btn.pack(expand=tkinter.YES, fill=tkinter.X)

    def test_lambda(self):
        btn = tkinter.Button(self._root, text = "test lambda", command=(lambda: print("hello, lambda") or sys.exit()), fg="green")
        btn.pack(expand=tkinter.YES, fill=tkinter.X)


    def quit(self):
        print("do exit")
        sys.exit()


    def show_msg(self, msg="i don't know"):
        print(msg)
        global v_label
        v_label.set(msg)

    def test_button(self):
        frame = tkinter.Frame(self._root, bg='#aa0000')
        frame.pack(side = tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)

        tkinter.Button(frame, text = 't,l', fg="red", bg='#00bbcc', command=lambda: self.show_msg("t,l")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Button(frame, text = 't,m', fg="red", bg='#00bbcc', command=lambda: self.show_msg("t,m")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Button(frame, text = 't,r', fg="red", bg='#00bbcc', command=lambda: self.show_msg("t,r")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)

        frame = tkinter.Frame(self._root, bg='#00aa00')
        frame.pack(side = tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Button(frame, text = 'm,l', fg="blue", bg='#aa00cc', command=lambda: self.show_msg("m,l")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Button(frame, text = 'm,m', fg="blue", bg='#aa00cc', command=lambda: self.show_msg("m,m")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Button(frame, text = 'm,r', fg="blue", bg='#aa00cc', command=lambda: self.show_msg("m,r")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)


        frame = tkinter.Frame(self._root, bg='#0000aa')
        frame.pack(side = tkinter.BOTTOM, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Button(frame, text = 'b,l', fg="green", bg='#aabb00', command=lambda: self.show_msg("b,l")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Button(frame, text = 'b,m', fg="green", bg='#aabb00', command=lambda: self.show_msg("b,m")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)
        tkinter.Button(frame, text = 'b,r', fg="green", bg='#aabb00', command=lambda: self.show_msg("b,r")).pack(side   = tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)



class BindTest:
    def __init__(self, root):

        frame = tkinter.Frame(root, bg='#bbaacc')
        frame.pack(side = tkinter.BOTTOM, fill=tkinter.BOTH, expand=tkinter.YES)

        self._root = frame
        self.btn_bind()
        self.label_bind()


    def hello(self, *args):
        global v_label
        v_label.set('double click to quit {}'.format(str(args)))

    def quit(self, *args):
        print('do exit... {}'.format(str(args)))
        self._root.quit()
#       assert 0, "never show up this line"
#       sys.exit(0)


    def btn_bind(self):
        btn = tkinter.Button(self._root, text='bind for button')
        btn.pack(side=tkinter.LEFT, expand=tkinter.YES, fill=tkinter.BOTH)

        btn.bind('<Button-1>', self.hello)
        btn.bind('<Double-1>', self.quit)

    def label_bind(self):

        lab = tkinter.Label(self._root, text='press me')
        lab.pack(side=tkinter.LEFT, expand=tkinter.YES, fill=tkinter.BOTH)
        lab.bind('<Button-1>', self.hello)
        lab.bind('<Double-1>', self.quit)



class QuitButton(tkinter.Button):

    def __init__(self, parent=None, **configs):

        super(QuitButton, self).__init__(parent, **configs)
        self.config(command=self.command_cb)
        if 'text' not in configs:
            self.config(text='QuitButton')

        self.pack(side=tkinter.TOP, expand=tkinter.NO, fill=tkinter.X)

    def command_cb(self):
        print("QuitButton pressed")
        self.quit()





if __name__ == '__main__':

    root = tkinter.Tk()
    root.geometry('1024x768')
    root.title('testing for gui')

    v_label = tkinter.StringVar()
    tkinter.Label(root, textvariable=v_label, fg="red", bg='#aabbcc').pack(side = tkinter.TOP, expand=tkinter.NO, fill=tkinter.X)
    QuitButton(root, text='hello, my quit button').pack(side=tkinter.TOP, expand=tkinter.NO, fill=tkinter.X)

#    TestLabel(root)
#    lab = TestLabel(root)
#    btn = ButtonTest(root)
    BindTest(root)

    root.mainloop()

