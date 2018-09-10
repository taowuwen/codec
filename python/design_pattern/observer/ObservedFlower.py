#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from Observer import Observer, Observable


class Flower:
    def __init__(self):

        self.is_open = 0
        self.open_notifier = Flower.OpenNotifier(self)
        self.close_notifier = Flower.CloseNotifier(self)

    def open(self):
        self.is_open = 1
        self.open_notifier.notify_servers()
        self.close_notifier.open()

    def close(self):
        self.is_open = 0
        self.close_notifier.notify_servers()
        self.open_notifier.close()

    def closing(self):
        return self.close_notifier

    class OpenNotifier(Observable):
        def __init__(self, outer):
            self.outer = outer
            self.already_open = 0
            super().__init__()

        def notify_servers(self):
            
            if self.outer.is_open and not self.already_open:
                self.set_changed()
                super().notify_servers()
                self.already_open = 1

        def close(self):
            self.already_open = 0


    class CloseNotifier(Observable):
        def __init__(self, outer):
            self.outer = outer
            self.already_close = 0
            super().__init__()

        def notify_servers(self):
            
            if not self.outer.is_open and not self.already_close:
                self.set_changed()
                super().notify_servers()
                self.already_close = 1

        def open(self):
            self.already_close = 0


class Bee:

    def __init__(self, name):
        self.name = name
        self.open_obs = Bee.OpenObserver(self)
        self.close_obs = Bee.CloseObserver(self)

    class OpenObserver(Observer):
        def __init__(self, outer):
            self.outer = outer
            super().__init__()

        def update(self, arg=None):
            print("Bee" + self.outer.name +"'s breakfirst time.")

    class CloseObserver(Observer):
        def __init__(self, outer):
            self.outer = outer
            super().__init__()

        def update(self, arg=None):
            print("Bee" + self.outer.name +"'s bed time.")


class Hummingbird:
    def __init__(self, name):
        self.name = name

        self.open_obs = Hummingbird.OpenObserver(self)
        self.close_obs = Hummingbird.CloseObserver(self)

    class OpenObserver(Observer):
        def __init__(self, outer):
            self.outer = outer
            super().__init__()

        def update(self, arg=None):
            print("Hummingbird" + self.outer.name +"'s breakfirst time.")

    class CloseObserver(Observer):
        def __init__(self, outer):
            self.outer = outer
            super().__init__()


        def update(self, arg=None):
            print("Hummingbird" + self.outer.name +"'s bed time.")


class AllergyOnDust:

    def __init__(self, name):
        self.name = name

        self.open_obs = AllergyOnDust.OpenObserver(self)
        self.close_obs = AllergyOnDust.CloseObserver(self)

    class OpenObserver(Observer):
        def __init__(self, outer):
            self.outer = outer
            super().__init__()

        def update(self, arg=None):
            print("AllergyOnDust " + self.outer.name + " should take a gauze mask")

    class CloseObserver(Observer):
        def __init__(self, outer):
            self.outer = outer
            super().__init__()
        
        def update(self, arg=None):
            print("AllergyOnDust " + self.outer.name + " No need to worry about flower anymore")


class TemplateTarget:

    def __init__(self, name):
        self.name = name

        self.open_obs = eval(self.__class__.__name__ + ".OpenObserver(self)")
        self.close_obs = eval(self.__class__.__name__ + ".CloseObserver(self)")

    class OpenObserver(Observer):
        def __init__(self, outer):
            self.outer = outer
            super().__init__()

        def update(self, arg=None):
            print("{} {} be notified on open".format(self.outer.__class__.__name__, self.outer.name))

    class CloseObserver(Observer):
        def __init__(self, outer):
            self.outer = outer
            super().__init__()
        
        def update(self, arg=None):
            print("{} {} be notified on close".format(self.outer.__class__.__name__, self.outer.name))


class JustTest(TemplateTarget):
    pass



if __name__ == '__main__':
    print("hello, world")

    f = Flower()
    ba = Bee("Eric")
    bb = Bee("Eric 0.5")
    ha = Hummingbird('A')
    hb = Hummingbird('B')

    aa = AllergyOnDust('Troy')
    ab = AllergyOnDust('Bob')

    ja = JustTest('justtest')

    f.open_notifier.add_observer(ba.open_obs)
    f.open_notifier.add_observer(bb.open_obs)
    f.open_notifier.add_observer(ha.open_obs)
    f.open_notifier.add_observer(hb.open_obs)
    f.open_notifier.add_observer(aa.open_obs)
    f.open_notifier.add_observer(ab.open_obs)
    f.open_notifier.add_observer(ja.open_obs)

    f.close_notifier.add_observer(ba.close_obs)
    f.close_notifier.add_observer(bb.close_obs)
    f.close_notifier.add_observer(ha.close_obs)
    f.close_notifier.add_observer(hb.close_obs)
    f.close_notifier.add_observer(aa.close_obs)
    f.close_notifier.add_observer(ab.close_obs)
    f.close_notifier.add_observer(ja.close_obs)

    f.open_notifier.del_observer(hb.open_obs)
    f.open()
    f.open()
    f.close_notifier.del_observer(bb.close_obs)

    f.close()
    f.close()

    f.open_notifier.clear_observers()

    f.open()
    f.close()
