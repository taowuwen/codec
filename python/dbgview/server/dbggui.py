import tkinter
import queue
from dbgmsg import DbgMessage

try:
    import tkFont
except ImportError:
    import tkinter.font as tkFont

class DbgView:
    def __init__(self, datactl, cfgctl, mq_gui, *kargs, **kwargs):
        self.root = tkinter.Tk()
        self.root.geometry('1024x768')
        self.root.title('DebugView by taowuwen@gmail.com')
        self.datactl = datactl
        self.cfgctl  = cfgctl
        self.mq_gui = mq_gui

        self.do_init_widget()

    def check_msg_queue(self):
        while True:
            try:
                _id = self.mq_gui.get_nowait()
            except queue.Empty:
                self.root.after(100, self.check_msg_queue)
                break
            else:
                if isinstance(_id, str):
                    msg = _id
                    self.listbox.insert(tkinter.END, msg)
                elif isinstance(_id, int):
                    msg = self.datactl.get(_id)
                    self.listbox.insert(tkinter.END, str(msg))
                else:
                    print(f"warnning, unkown id: {_id}")
                    continue

                self.listbox.yview(tkinter.END)

    def run(self):
        self.root.after(100, self.check_msg_queue)
        return self.root.mainloop()

    def do_init_widget(self):
        self.frame    = tkinter.Frame(self.root)
        self.listbox  = tkinter.Listbox(self.frame)
        self.y_scroll = tkinter.Scrollbar(self.root, orient=tkinter.VERTICAL)
        self.x_scroll = tkinter.Scrollbar(self.frame, orient=tkinter.HORIZONTAL)

        self.frame.pack(side=tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)

        self.listbox.pack(side=tkinter.TOP, fill = tkinter.BOTH, expand=tkinter.YES)
        self.x_scroll.pack(side=tkinter.TOP, fill=tkinter.X)

        self.y_scroll.pack(side=tkinter.LEFT, fill=tkinter.Y)

        self.listbox.config(
            fg               = 'black',
            selectmode       = tkinter.EXTENDED,
            selectforeground = 'white',
            selectbackground = '#33B397',
            yscrollcommand   = self.y_scroll.set,
            xscrollcommand   = self.x_scroll.set,
            height           = 10,
            width            = 10,
            font             = 'systemSystemFont',
        )

        self.y_scroll.config(command = self.listbox.yview)
        self.x_scroll.config(command = self.listbox.xview)


    def do_test_widget(self):
        for item in ["one", "two", "three", "four"]:
            self.listbox.insert("end", item)

        tmpfont = tkFont.Font(family='Helvetica', size=20, weight='normal')
        self.listbox.config(font='systemSystemFont')

        self.listbox.itemconfig(1, {'bg':'red'})
        self.listbox.itemconfig(3, {'fg':'blue'})
        self.listbox.itemconfig(2, bg='green')
        self.listbox.itemconfig(0, foreground="purple")
        self.listbox.itemconfig(tkinter.END, {'bg':'black'})