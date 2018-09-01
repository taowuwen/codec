#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Item:

    def __init__(self, name, price, quantity):
        self.name = name
        self.price = price
        self.quantity = quantity
        self.id = 0

    def __str__(self):
        return "{} {}: {}$ {}".format(self.id, self.name, self.price, self.quantity)

"""
+---------------+---------------+----------------+
| 00            |               |                |
|               |               |                |
|               |               |                |
|               |               |                |
+---------------+---------------+----------------+
| 10            |               |                |
|               |               |                |
|               |               |                |
|               |               |                |
+---------------+---------------+----------------+
| 20            |               |                |
|               |               |                |
|               |               |                |
|               |               |                |
+---------------+---------------+----------------+
| 30            |               |                |
|               |               |                |
|               |               |                |
|               |               |                |
+---------------+---------------+----------------+
"""


class ItemSlots:

    width = 20
    height = 4

    def __init__(self, x, y):

        self.x = x
        self.y = y
        self.items = [ [None for a in range(x)] for b in range(y) ]

    def __str__(self):
        return str(self.items)

    def show_items(self):

        for _y in range(self.y * ItemSlots.height + 1):
            for _x in range(self.x * ItemSlots.width + 1):

                if _x % ItemSlots.width == 0 and _y % ItemSlots.height == 0:
                    print("+", end="")
                elif _x % ItemSlots.width == 0:
                    print("|", end="")
                elif _y % ItemSlots.height == 0:
                    print("-", end="")
                else:
                    print(" ", end="")

            print("")

    def show_items_v1(self):
        from beautifultable import BeautifulTable

        table = BeautifulTable(default_padding=2)

        [ table.append_row(self.items[_y]) for _y in range(self.y) ]

        print(table)

    def append_item(self, it):

        for _y in range(self.y):
            for _x in range(self.x):
                if not self.items[_y][_x]:
                    self.items[_y][_x] = it
                    it.id = str(_y) + str(_x)
                    return

    def load(self, fl="items.txt"):

        with open(fl, "r") as fin:
            for ln in fin:
                self.append_item(Item(*ln.split()))


def test():

    from beautifultable import BeautifulTable

    table = BeautifulTable(default_padding=2)

    table.column_headers = ["name", "rank", "gender"]
    table.append_row(["Jacob", 1, "boy"])
    table.append_row(["Isabella", 1, "girl"])
    table.append_row(["Ethan", 2, "boy"])
    table.append_row(["Sophia", 2, "girl"])
    table.append_row(["Michaela", 3, "boy"])

    table.set_style(BeautifulTable.STYLE_DEFAULT)
    print(table)
    table.set_style(BeautifulTable.STYLE_DOTTED)
    print(table)
    table.set_style(BeautifulTable.STYLE_MYSQL)
    print(table)
    table.set_style(BeautifulTable.STYLE_SEPERATED)
    print(table)
    table.set_style(BeautifulTable.STYLE_COMPACT)
    print(table)
    table.set_style(BeautifulTable.STYLE_MARKDOWN)
    print(table)
    table.set_style(BeautifulTable.STYLE_RESTRUCTURED_TEXT)
    print(table)

    from prettytable import PrettyTable

    t = PrettyTable(['Name', 'Age'], padding_width=10, vertical_char='|', horizontal_char='-')
    t.add_row(['alice', 20])
    t.add_row(['Bob', 21])
    print(t)



if __name__ == '__main__':

    vm = ItemSlots(5,4)

    #print(vm)
    #vm.show_items()
    vm.load()
    vm.show_items_v1()

