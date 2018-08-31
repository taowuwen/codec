#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class ApplicationTemplate:

    def __init__(self):

        self._template_method()

    def _template_method(self):

        for i in range(5):

            self.custom_method1()
            self.custom_method2()

class MyApp(ApplicationTemplate):

    def custom_method1(self):
        print("Hello, Custom method one")

    def custom_method2(self):
        print("World, Custom Method two")


if __name__ == '__main__':
    print("hello, TemplateMethod")

    MyApp()
