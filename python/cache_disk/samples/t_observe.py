#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from f_observer import FileObject, FileObserveObject
import random

class Person(FileObject):
    def __init__(self, name):
        super().__init__()
        self._name = name

    def __str__(self):
        return f'{self._name}'

    def __repr__(self):
        return str(self)

class Teacher(Person):
    def push_task(self, task):
        self.notify(self, task)

    def update(self, *args, **kwargs):
        print(f'{self} be notified: {args}, {kwargs}')

class Student(Person):
    def handle_task(self, task):
        self.notify(self, 'handle task: {}'.format(task))

    def update(self, *args):
        print(f'{self} be notified by {args}')

class Flower(FileObserveObject):
    def __init__(self, name):
        super().__init__()
        self._name = name

    def flower_open(self):
        self.notify(self, 'opened')

    def flower_close(self):
        self.notify(self, 'closed')

    def __str__(self):
        return f'{self._name}'

    def __repr__(self):
        return str(self)

if __name__ == '__main__':

    t_jim = Teacher('Jim')
    t_math = Teacher('math')
    t_english = Teacher('english')
    f_flower_a = Flower('flower_AAA')
    f_flower_b = Flower('flower_BBB')

    stus = [Student('stu' + str(x)) for x in range(50)]

    for stu in stus:
        stu.subscribe(t_jim)
        stu.subscribe(t_math)
        stu.subscribe(t_english)

        t_math.subscribe(stu)
        t_english.subscribe(stu)

    for x in range(10):
        stu = random.choice(stus)
        stu.subscribe(f_flower_a)

    for x in range(10):
        stu = random.choice(stus)
        stu.subscribe(f_flower_b)

    t_jim.push_task('TASK_JIM')
    t_math.push_task('TASK_MATH')
    t_english.push_task('TASK_ENGLISH')
    f_flower_a.flower_open()
    f_flower_b.flower_open()

    for x in range(10):
        stu = random.choice(stus)
        stu.handle_task('TASK_MATH')

    for x in range(10):
        stu = random.choice(stus)
        stu.handle_task('TASK_JIM')

    for x in range(10):
        stu = random.choice(stus)
        stu.handle_task('TASK_ENGLISH')

    f_flower_b.flower_close()
    f_flower_a.flower_close()
