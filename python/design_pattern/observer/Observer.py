#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from Synchronization import Synchronization, synchronize


class Observer:
    def update(self, arg):
        pass


class Observable(Synchronization):

    def __init__(self):
        self.obss = []
        self.change = 0
        super().__init__()

    def add_observer(self, obs):
        if obs not in self.obss:
            self.obss.append(obs)

    def del_observer(self, obs):
        if obs in self.obss:
            self.obss.remove(obs)

    def notify_servers(self, arg = None):

        self.mutex.acquire()

        try:
            local_obs = self.obss[:]

            self.clear_changed()

        finally:
            self.mutex.release()

        for obs in local_obs:
            obs.update(arg)

    def clear_observers(self):
        self.obss = []

    def set_changed(self):
        self.change = 1

    def clear_changed(self):
        self.change = 0

    def is_changed(self):
        return self.change

    def count_observers(self):
        return len(self.obss)


synchronize(Observable, "add_observer del_observer clear_observers " +
        "set_changed clear_changed is_changed count_observers")

