#!/usr/bin/env python3
# -*- coding:utf-8 -*-

import sys
import os
import tkinter
import tkinter.messagebox as msgbox
from tkinter.filedialog import askopenfilename
from tkinter.colorchooser import askcolor
from tkinter.simpledialog import askfloat
import glob


class HelloButton(tkinter.Button):

    def __init__(self, parent, **configs):
        
        super(HelloButton, self).__init__(parent, **configs)

        self['bg'] = 'black'
        self['fg'] = 'white'
        self['padx'] = 20
        self['pady'] = 20

        if 'command' not in configs:
            self['command'] = sys.exit

        self.pack()


def _test_1():
    win1 = tkinter.Toplevel()
    win2 = tkinter.Toplevel()

    win1.geometry('200x100')
    win2.geometry('200x100')

    HelloButton(win1, text='spam')
    HelloButton(win2, text='spam')

    tkinter.Label(text='popups').pack()

    win1.mainloop()



class TestMsg:
    def __init__(self, parent=None, **configs):

        if not parent:
            parent = tkinter.Tk()

        self.top = parent

        self.top.geometry('1024x768')
        self.top.title('test for testmsg')

        self.prepare()


    def mainloop(self):
        self.top.mainloop()

    def callback(self):

        if msgbox.askyesno('Verify', 'Do you really want to quit?'):
            msgbox.showwarning('Yes', 'but quit not yes implemented, HAHAHA...')
        else:
            msgbox.showinfo('No', 'Quit has been cancelled')

    def prepare(self):
        errmsg = 'Sorry, no Spam allowed'
        HelloButton(self.top, text='quit', command=self.callback).pack(fill=tkinter.X)
        HelloButton(self.top, text='SPAM', command=lambda: msgbox.showerror('Spam', errmsg)).pack(fill=tkinter.X)
        HelloButton(self.top, text='QUIT......').pack(fill=tkinter.X)


demos = {
        'Open': askopenfilename,
        'Color': askcolor,
        'Query': lambda: msgbox.askquestion('Warning!', 'You typed "rm -rf" \n Confirmed?'),
        'Error': lambda: msgbox.showerror('Error!!!', "he's dead, jim"),
        'Input': lambda: askfloat('Entry', 'Enter credit card number')
}


class TestDemo(tkinter.Frame):

    def __init__(self, parent=None, **options):

        if not parent:
            parent = tkinter.Tk()

        super(TestDemo, self).__init__(parent, **options)
        self.pack(expand=tkinter.YES, fill=tkinter.BOTH)
        parent.geometry('1024x768')

        self.v_label = tkinter.StringVar()
        self.v_label.set('Basic Demos')
        self.label = tkinter.Label(self, textvariable=self.v_label)
        self.label.pack()

#        for k, v in demos.items():
#            HelloButton(self, text=k, command=v).pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)

        for k in demos:
            HelloButton(self, text=k, command=(lambda k=k: self.print_result(k))).pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)
        HelloButton(self, text='QUIT').pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)

    def print_result(self, key):

        res = demos[key]()
        self.v_label.set('{}'.format(res))

        if key is 'Color' and res[-1]:
            self.label.config(fg=res[-1])
        


from tkinter.dialog import Dialog

class OldDialogDemo(tkinter.Frame):

    def __init__(self, parent=None, **configs):
        super(OldDialogDemo, self).__init__(parent, **configs)
        self.pack()

        HelloButton(self, text='Pop1', command=self.dialog1).pack(fill=tkinter.BOTH)
        HelloButton(self, text='Pop2', command=self.dialog2).pack(fill=tkinter.BOTH)

    def dialog1(self):
        ans = Dialog(self,
                title='Popup Fun!',
                text = 'An example of a popup-dialog'
                        'box, using older "dialog.py"?',
                bitmap = 'questhead',
                default = 0, strings=('yes', 'no', 'cancel'))

        if ans.num == 0: self.dialog2()

    def dialog2(self):
        Dialog(self, title='HAL-9000', text="Im afraid i can't let you do that, Dave...",
                bitmap='questhead',
                default=0, strings=('spam', 'SPAM'))




makemodal = (len(sys.argv) > 1)

def dialog():

    win = tkinter.Toplevel()
    tkinter.Label(win, text='hard drive reformatted!!!').pack()
    tkinter.Button(win, text='OK', command=win.destroy).pack()

    if makemodal:
        win.focus_set()
        win.grab_set()
        win.wait_window()

    print('dialog exit...')


def dialog_v2():
    win = tkinter.Toplevel()
    tkinter.Label(win, text='Hard drive reformatted!!!!').pack()
    tkinter.Button(win, text='OK', command=win.quit).pack()
    win.protocol('WM_DELETE_WINDOW', win.quit)

    win.focus_set()
    win.grab_set()
    win.mainloop()
    win.destroy()
    print('dialog exit')


class TestEntry:

    def __init__(self, parent=None):

        self.top = parent
        #self.test_1()
        #self.test_2()
        self.test_v3()

        
    def makeforms(self):
        fields = 'Name', 'Job', 'Pay'

        entries = []

        for field in fields:

            row = tkinter.Frame(self.top)
            lab = tkinter.Label(row, width=5, text=field)
            ent = tkinter.Entry(row)

            row.pack(side=tkinter.TOP, fill=tkinter.X)
            lab.pack(side=tkinter.LEFT)
            ent.pack(side=tkinter.RIGHT, expand=tkinter.YES, fill=tkinter.X)

            entries.append(ent)

        return entries

    def makeforms_v1(self):
        fields = 'Name', 'Job', 'Pay'
        
        frame = tkinter.Frame(self.top)
        left = tkinter.Frame(frame)
        right = tkinter.Frame(frame)

        frame.pack(side=tkinter.TOP)
        left.pack(side=tkinter.LEFT, fill=tkinter.X)
        right.pack(side=tkinter.RIGHT, fill=tkinter.X, expand=tkinter.Y)

        entries = []

        for field in fields:

            v_ent = tkinter.StringVar()
            lab = tkinter.Label(left, width=5, text=field)
            ent = tkinter.Entry(right, textvariable=v_ent)
            v_ent.set('Enter something')

            lab.pack(side=tkinter.TOP)
            ent.pack(side=tkinter.TOP, expand=tkinter.Y, fill=tkinter.X)
            entries.append(v_ent)

        return entries

    def test_v3(self):
        entries = self.makeforms()
        self.top.bind('<Return>', (lambda event: self.fetch_all(entries)))
        tkinter.Button(self.top, text='Fetch', command=lambda: self.fetch_all(entries)).pack(fill=tkinter.X)

    def test_2(self):

        entries = self.makeforms()
        self.top.bind('<Return>', (lambda event: self.fetch_all(entries)))
        tkinter.Button(self.top, text='Fetch', command=lambda: self.fetch_all(entries)).pack(fill=tkinter.X)

    def fetch_all(self, entries):

        for ent in entries:
            print('Input => {}'.format(ent.get()))


    def test_1(self):
        self.ent = tkinter.Entry(self.top)
        self.ent.insert(0, 'Type words here')
        self.ent.pack(fill=tkinter.X)
        self.ent.focus()
        self.ent.bind('<Return>', (lambda event: self.fetch()))
        tkinter.Button(self.top, text='Fetch', command=self.fetch_1).pack(fill=tkinter.X)


    def fetch(self):
        print('Input => "{}"'.format(self.ent.get()))

    def fetch_1(self):
        print('Input => "{}"'.format(self.ent.get()))
        #self.ent.delete(0, tkinter.END)
        self.ent.insert(tkinter.END, 'x')
        self.ent.insert(0, 'x')


def show_up(top, t_ent, entries):
    t_ent.fetch_all(entries)
    top.destroy()

def show_up_v1(top, entries):
    top.destroy()
    for ent in entries:
        print("Input => {}".format(ent.get()))


def ask(root):

    top = tkinter.Toplevel(root)

    t_ent = TestEntry(top)

    entries = t_ent.makeforms()
    tkinter.Button(top, text='OK', command=lambda: show_up(top, t_ent, entries)).pack(fill=tkinter.X)
    top.bind('<Return>', (lambda event: show_up(top, t_ent, entries)))

    top.grab_set()
    top.focus_set()
    top.wait_window()


def ask_v1(root):

    top = tkinter.Toplevel(root)

    t_ent = TestEntry(top)
    entries = t_ent.makeforms_v1()
    tkinter.Button(top, text='OK', command=lambda: show_up(top, t_ent, entries)).pack(fill=tkinter.X)
    top.bind('<Return>', (lambda event: show_up(top, t_ent, entries)))

    top.grab_set()
    top.focus_set()
    top.wait_window()



class TestCheckbutton:

    def __init__(self, parent=None):
        self.top = parent

        frame = tkinter.Frame(parent)
        frame.pack(expand=tkinter.YES, fill=tkinter.BOTH)
        self.top = frame

        #self.test_1()
        #self.test_2()

        self.test_3()

    def show_var(self):
        print([var.get() for var in self.states])

    def test_3(self):

        self.states = []

        for i in range(9):
            var = tkinter.IntVar()
            tkinter.Checkbutton(self.top, text=str(i), variable=var, command=self.show_var).pack(side=tkinter.LEFT)
            self.states.append(var)


    def on_press(self, i):
        self.states[i] = not self.states[i]
        print(self.states)

    def test_2(self):

        self.states = []

        for i in range(10):
            tkinter.Checkbutton(self.top, text=str(i), command=(lambda i=i: self.on_press(i))).pack(side=tkinter.LEFT)
            self.states.append(False)


    def test_1(self):

        frm = tkinter.Frame(self.top)
        frm.pack(side=tkinter.RIGHT)

        tkinter.Button(frm, text='State', command=self.state).pack()
        tkinter.Button(frm, text='Quit',  command=frm.quit).pack()

        tkinter.Label(self.top, text='Checkbutton Demo').pack(side=tkinter.TOP)

        self.vars = []

        for key in demos:

            var = tkinter.IntVar()

            tkinter.Checkbutton(self.top,
                    text=key,
                    variable=var,
                    command=demos[key]).pack(side=tkinter.LEFT)

            self.vars.append(var)

    def state(self):
        for var in self.vars:
            print(var.get(), end=' ')
        print()
        sys.stdout.flush()


class TestRadiobutton:
    def __init__(self, parent=None):
        self.top = parent
        frame = tkinter.Frame(self.top)
        frame.pack(fill=tkinter.BOTH)
        self.top = frame

        #self.test_1()
        #self.test_2()
        #self.test_3()
        self.var = tkinter.IntVar()
        self.test_4()

    def test_4(self):

        for i in range(10):
            tkinter.Radiobutton(self.top, text=str(i), value=i, variable=self.var).pack(side=tkinter.LEFT)

        print(self.var.get())


    def on_press_3(self, i):

        for btn in self.buttons:
            btn.deselect()

        self.buttons[i].select()


    def test_3(self):
        self.buttons = []

        for i in range(10):
            btn = tkinter.Radiobutton(self.top, text=str(i), value=str(i), command=(lambda i=i: self.on_press_3(i)))
            btn.pack(side=tkinter.LEFT)
            self.buttons.append(btn)

        self.on_press_3(0)
        


    def test_2(self):

        self.var = tkinter.StringVar()

        for i in range(10):
            tkinter.Radiobutton(self.top, text=str(i),
                    variable=self.var,
                    value=str(i%4)).pack(side=tkinter.LEFT)

        self.var.set(' ')

    def test_1(self):

        tkinter.Label(self.top, text='Radio demos').pack(side=tkinter.TOP)
        self.var = tkinter.StringVar()

        for key in demos:

            tkinter.Radiobutton(self.top, text=key,
                    command=self.on_press,
                    variable=self.var,
                    value=key).pack(anchor=tkinter.NW)

        self.var.set(key)

        tkinter.Button(self.top, text='State', command=self.report).pack(fill=tkinter.X)

    def on_press(self):
        pick = self.var.get()
        print('You pressed', pick)
        print('result: {}'.format(demos[pick]()))

    def report(self):
        print(self.var.get())

        


class TestScale:
    def __init__(self, parent=None):
        self.top = parent
        frame = tkinter.Frame(self.top)
        frame.pack(fill=tkinter.BOTH)
        self.top = frame

        #self.test_1()
        self.test_2()

    def test_2(self):

        self.scl = tkinter.Scale(self.top, from_=-100, to=100, tickinterval=50, resolution=10)
        self.scl.pack(expand=tkinter.YES, fill=tkinter.Y)
        tkinter.Button(self.top, text='State', command=self.report).pack(side=tkinter.RIGHT, anchor=tkinter.S)

    def report(self):
        print(self.scl.get())


    def test_1(self):
        tkinter.Label(self.top, text='scale demo').pack(side=tkinter.TOP)

        self.var = tkinter.IntVar()

        tkinter.Scale(self.top, label='Pick demo number',
                command=self.on_move,
                variable=self.var,
                from_=0, to=len(demos)-1).pack()

        tkinter.Scale(self.top, label='Pick demo number',
                command=self.on_move,
                variable=self.var,
                from_=0, to=len(demos)-1,
                length=200, tickinterval=1,
                showvalue=tkinter.YES, orient='horizontal').pack()

        tkinter.Button(self.top, text='Run demo', command=self.on_run).pack(side=tkinter.LEFT)
        tkinter.Button(self.top, text='State', command=self.state).pack(side=tkinter.RIGHT)

    def on_move(self, value):
        print('in on_move {}'.format(value))


    def on_run(self):
        pos = self.var.get()
        print('You picked {}'.format(pos))
        demo = list(demos.values())[pos]
        print(demo())

    def state(self):
        print(self.var.get())


#from tkinter import PhotoImage

from PIL import Image
from PIL.ImageTk import PhotoImage


image_root=u'/home/tww/Pictures'

class NoPictureInDirectory(Exception): pass


class TestPhotoImage:

    def __init__(self, top=None):

        self.top = top

        frame = tkinter.Frame(self.top)
        frame.pack(expand=tkinter.YES, fill=tkinter.BOTH)

        self.top=frame

        self.test_1()

    def test_1(self):
        self.pictures = glob.glob(os.path.join(image_root, u'*.jpg'))
        self._cur = 0

        if len(self.pictures) <= 0:
            raise NoPictureInDirectory(image_root)

        self.img = PhotoImage(file=self.pictures[self._cur%len(self.pictures)])
        self.btn = tkinter.Button(self.top, image=self.img, command=self.next_picture)
        self.btn.pack()

        #label = tkinter.Label(self.top, image=img)
        #label.image = img
        #label.pack()

#	can = tkinter.Canvas(self.top)
#	can.pack(fill=tkinter.BOTH)
#       can.create_image(2, 2, image=img, anchor=tkinter.NW)
#       can.image=img

    def next_picture(self):
        self._cur += 1

        self.img = PhotoImage(file=self.pictures[self._cur%len(self.pictures)])
        self.btn.config(image=self.img)



if __name__ == '__main__':
    print('hello, test for chp8')
#    _test_1()

#    TestMsg().mainloop()
#    TestDemo().mainloop()

    root = tkinter.Tk()
    root.geometry('1024x768')

  #  OldDialogDemo(root)
#    HelloButton(root, text='QUIT', command=dialog).pack(fill=tkinter.BOTH, expand=tkinter.YES)
#   HelloButton(root, text='popup', command=dialog_v2).pack(fill=tkinter.X, expand=tkinter.YES)
#   msg = tkinter.Message(root, text="Oh by the way, which one's Pink")
#   msg.config(bg='pink', font=('times', 16, 'italic'))
#   msg.pack(fill=tkinter.X, expand=tkinter.YES)

#    TestEntry(root)

#    HelloButton(root, text='ASK', command=lambda: ask(root)).pack(fill=tkinter.X)
#    HelloButton(root, text='ASK', command=lambda: ask_v1(root)).pack(fill=tkinter.X)
#    TestCheckbutton(root)
#    TestRadiobutton(root)
#    TestScale(root)
    TestPhotoImage(root)
    HelloButton(root, text='QUIT').pack(expand=tkinter.NO, fill=tkinter.X)

    root.mainloop()


