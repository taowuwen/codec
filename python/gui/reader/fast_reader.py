#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tkinter
import sys
import os
import time
import inspect


# first page
class ScrolledText(tkinter.Frame):
    def __init__(self, parent=None):
        super(ScrolledText, self).__init__(parent)
        self.pack(expand=tkinter.YES, fill=tkinter.BOTH)

        self.make_widgets()

    def make_widgets(self):

        self.sbar = tkinter.Scrollbar(self)
        self.text = tkinter.Text(self, relief=tkinter.SUNKEN)

        self.sbar.config(command=self.text.yview)
        self.text.config(yscrollcommand=self.sbar.set)

        self.sbar.pack(side=tkinter.RIGHT, expand=tkinter.NO, fill=tkinter.Y)
        self.text.pack(side=tkinter.LEFT, expand=tkinter.YES, fill=tkinter.BOTH)

    def settext(self, text = '', file=None):
        if file:
            with open(file, 'r') as f:
                text = f.read()

        self.text.delete('1.0', tkinter.END)
        self.text.insert('1.0', text)
        self.text.mark_set(tkinter.INSERT, '1.0')
        self.text.focus()

    def gettext(self):
        return self.text.get('1.0', tkinter.END+'-1c')


class StatusBar(tkinter.Frame):

    def __init__(self, parent=None):
        super(StatusBar, self).__init__(parent)
        self.pack()

        self.status = {'Status': 'paused'}

        font = ('DejaVu Sans Mono', 10, 'bold')
        self.var = tkinter.StringVar()

        self.label = tkinter.Label(self, textvariable=self.var)
        self.label.pack(side=tkinter.LEFT)
        self.label.config(font=font)
        self.update_content()


    def update_content(self):

        _txt = ""

        for k in self.status:
            _txt += "\t{} : {}".format(k, self.status[k])

        self.var.set(_txt)


    def config(self, **options):
        if options:
            self.label.config(**options)

    def status_update(self, **st):
        self.status.update(st)
        self.update_content()


class ButtonStart(tkinter.Button):

    def __init__(self, parent=None, **options):
        super(ButtonStart, self).__init__(parent, **options)
        self['fg'] = 'white'
        self['bg'] = 'black'
        self['text'] = 'START'
        self.config(font = ('DejaVu Sans Mono', 20, 'bold'))


class MenuConfig(tkinter.Frame):
    def __init__(self, parent=None, cb=None, **options):

        super(MenuConfig, self).__init__(parent, None, **options)
        self.pack()

        self.mbutton = tkinter.Menubutton(self, text='...')
        self.mbutton.config(font = ('DejaVu Sans Mono', 20, 'bold'))

        menu = tkinter.Menu(self.mbutton)

        if cb:
            cb(menu)
        else:
            menu.add_command(label='Quit', command=self.quit)

        self.mbutton.config(menu=menu)
        self.mbutton.pack(side=tkinter.LEFT, anchor=tkinter.E)
        self.mbutton.config(bg='white', bd=4)



# reading page
class FastReading(tkinter.Frame):
    def __init__(self, parent=None, event_cb=None):
        super(FastReading, self).__init__(parent)

        self.var = tkinter.StringVar()
        self.label = tkinter.Label(self, textvariable=self.var)

        self.label.config(font = ('DejaVu Sans Mono', 40, 'bold'))
        self.label.pack(expand=tkinter.YES, fill=tkinter.BOTH)

        self.var.set(u'Hello, FastReading is here')

        self._font_size = 40
        self._chunk_size = 5
        self._speed = 1500
        self._run   = False
        self._after_id = None
        self._event_cb = event_cb

    def _font_larger(self, event):
        self._font_size += 1
        self.label.config(font = ('DejaVu Sans Mono', self._font_size, 'bold'))
        self._event_cb(inspect.currentframe().f_code.co_name)

    def _font_smaller(self, event):
        self._font_size -= 1
        self.label.config(font = ('DejaVu Sans Mono', self._font_size, 'bold'))
        self._event_cb(inspect.currentframe().f_code.co_name)

    def _speed_up(self, event):
        self.speed += 10
        self._event_cb(inspect.currentframe().f_code.co_name)

    def _speed_down(self, event):
        self.speed -= 10
        self._event_cb(inspect.currentframe().f_code.co_name)

    def _chunk_size_plus(self, event):
        self.chunk += 1
        self._event_cb(inspect.currentframe().f_code.co_name)

    def _chunk_size_minus(self, event):
        self.chunk -= 1
        self._event_cb(inspect.currentframe().f_code.co_name)

    def start(self):
        self._run = True
        self._timeout()

    def stop(self):
        self._run = False

        if self._after_id:
            self.after_cancel(self._after_id)

    def _timeout(self):
        self.timeout = int(60000 * self._chunk_size / self._speed)

        if self._run:
            if self._after_id:
                self.after_cancel(self._after_id)

            self.update_content()
            self._after_id = self.after(self.timeout, self.update_content)

    def _do_run(self, event):
        self._run = not self._run

        if self._run:
            self._event_cb('running')
            self._after_id = self.after(self.timeout, self.update_content)
        else:
            self._event_cb('paused')


    def update_content(self):

        ctx = ""
        if not self._event_cb:
            ctx = "{}ms > {:.2f}".format(self.timeout, time.time())
        else:
            ctx = self._event_cb('next_chunk')

        if self._after_id:
            self.after_cancel(self._after_id)


        if ctx:
            self.var.set(ctx)

            if self._run:
                self._after_id = self.after(self.timeout, self.update_content)
        else:
            self._run = False

    @property
    def chunk(self):
        return self._chunk_size

    @chunk.setter
    def chunk(self, v):
        self._chunk_size = v

        if self._chunk_size <= 1:
            self._chunk_size = 1

        self._timeout()

    @property
    def speed(self):
        return self._speed

    @speed.setter
    def speed(self, v):

        self._speed = v
        if self._speed <= 10:
            self._speed = 10

        self._timeout()

    @property
    def font_size(self):
        return self._font_size

    @font_size.setter
    def font_size(self, v):
        self._font_size = v

        if self._font_size <= 10:
            self._font_size = 10

        self.label.config(font = ('DejaVu Sans Mono', self._font_size, 'bold'))

    @property
    def run(self):
        return self._run


class ButtonQuitter(tkinter.Button):
    def __init__(self, parent=None, **options):
        super(ButtonQuitter, self).__init__(parent, **options)

        self['fg'] = 'white'
        self['bg'] = 'black'
        self['text'] ='X'
        self.config(font = ('DejaVu Sans Mono', 20, 'bold'))


def time_now(sbar):
    import time
    sbar.status_update(timenow=time.time())

if __name__ == '__main__':
    root = tkinter.Tk()

    root.title('Fast reader')
    root.geometry('1024x768')

    frame = tkinter.Frame(root)
    frame.pack(side=tkinter.TOP, fill=tkinter.X)

    sbar = StatusBar(frame)
    root.bind('<Return>', lambda event, sbar=sbar: time_now(sbar))

    sbar.pack(side=tkinter.LEFT, fill=tkinter.X, expand=tkinter.YES, anchor=tkinter.W)
    MenuConfig(frame).pack(side=tkinter.RIGHT, fill=tkinter.X, expand=tkinter.YES, anchor=tkinter.E)

#    st = ScrolledText(root)
#    st.settext(file='/home/tww/b.txt')
    fr = FastReading(root)
    fr.pack(side=tkinter.TOP, fill=tkinter.BOTH)

    root.bind('+', fr._speed_up)
    root.bind('-', fr._speed_down)
    root.bind('<space>', fr._do_run)

    fr.start()

    btn = ButtonStart(root)
    btn.pack(side=tkinter.TOP, fill=tkinter.X)


    ButtonQuitter(root).pack(side=tkinter.TOP, fill=tkinter.X)

    root.mainloop()

