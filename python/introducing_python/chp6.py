#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#class Person():
#    def __init__(self, name):
#        self.name = name
#
#
#ll = Person("lilei")
#
#print("person's name: ", ll.name);
#
#class MDPerson(Person):
#    def __init__(self, name):
#        self.name = "Doctor " + name
#
#class JDPerson(Person):
#    def __init__(self, name):
#        self.name = name + ", Esquire"
#
#
#md = MDPerson("mdperson")
#jd = JDPerson("jdperson")
#
#print("person's name: ", md.name);
#print("person's name: ", jd.name);
#
#
#class EmailPerson(Person):
#    def __init__(self, name, email):
#        super().__init__(name)
#        self.email = email
#
#
#email = EmailPerson("tmp", "taowuwen@126.com");
#print("person's name: ", email.name, email.email);
#
#email.email = "taowuwen@gmail.com"
#print("person's name: ", email.name, email.email);



#class Car():
#    def exclaim(self):
#        print("I'm a car");
#
#
#class Yugo(Car):
#    def exclaim(self):
#        print("I'm a Yugo! Much like a Car, but more yugo-ish.");
#
#    def need_a_push(self):
#        print("A little help here?")
#
#
#give_me_a_car = Car()
#give_me_a_yugo = Yugo()
#
#give_me_a_car.exclaim()
#give_me_a_yugo.exclaim()
#
#give_me_a_yugo.need_a_push()



#class Duck():
#    def __init__(self, name):
#        self.hidden_name = name
#
#
#    def get_name(self):
#        print("inside get_name")
#        return self.hidden_name
#
#    def set_name(self, name):
#        print("inside set name")
#        self.hidden_name = name
#
#    name = property(get_name, set_name)


#class Duck():
#    def __init__(self, name):
#        self.hidden_name = name
#
#
#    @property
#    def name(self):
#        print("inside the getter")
#        return self.hidden_name
#
#    @name.setter
#    def name(self, name):
#        print("inside the setter")
#        self.hidden_name = name
#
#
#fowl = Duck("Howard")
#print("fowl.name = ", fowl.name)
#
#fowl.name = "tmpset"
#print("fowl.name = ", fowl.name)


#class Circle():
#    def __init__(self, radius):
#        self.radius = radius
#
#    @property
#    def diameter(self):
#        return 2 * self.radius
#
#
#c = Circle(5)
#print(c.radius)
#print(c.diameter)
#
#c.radius = 7
#print(c.radius)
#print(c.diameter)


#class Duck():
#    def __init__(self, name):
#        self.__name = name
#
#    @property
#    def name(self):
#        print("inside the getter")
#        return self.__name
#
#    @name.setter
#    def name(self, name):
#        print("inside the setter")
#        self.__name = name
#
#
#d = Duck("abc")
#
#print(d.name)
#
#d.name = "cba"
#print(d.name)




#class A():
#    count = 0
#    def __init__(self):
#        A.count += 1
#
#    def exclaim(self):
#        print("I am a A")
#
#    @classmethod
#    def kids(cls):
#        print("A has", cls.count, "little objects");
#
#    @staticmethod
#    def commercial():
#        print("commercial one")
#
#
#A.kids()
#esay_a = A()
#normal_a = A()
#hard_a = A()
#A.kids()
#
#A.exclaim(esay_a)
#normal_a.exclaim()
#A.commercial()



#class Quote():
#    def  __init__(self, person, words):
#        self.person = person
#        self.words  = words
#
#    def who(self):
#        return self.person
#
#    def says(self):
#        return self.words + "."
#
#class QuestionQuote(Quote):
#    def says(self):
#        return self.words + "?"
#
#class ExclamationQuote(Quote):
#    def says(self):
#        return self.words + "!"
#
#
#hunter_a = Quote("Elmer Fudd", "I'm hunting rabbits")
#hunter_b = QuestionQuote("Bugs Bunny", "What's up, doc")
#hunter_c = ExclamationQuote("Daffy Duck", "It's rabbit season")
#
#print(hunter_a.who(), "says", hunter_a.says())
#print(hunter_b.who(), "says", hunter_b.says())
#print(hunter_c.who(), "says", hunter_c.says())
#
#
#
#class BabblingBrook():
#    def who(self):
#        return "Brook"
#
#    def says(self):
#        return "Babble"
#
#brook = BabblingBrook()
#
#
#def who_says(obj):
#    print(obj.who(), "says", obj.says())
#
#
#who_says(hunter_a)
#who_says(hunter_b)
#who_says(hunter_c)
#who_says(brook)



#class Word():
#    def __init__(self, text):
#        self.text = text
#
#    def __eq__(self, word):
#        return self.text.lower() == word.text.lower()
#
#
#    def __ne__(self, word):
#        return self.text.lower() != word.text.lower()
#
#    def __add__(self, word):
#        self.text += word.text
#        return self.text
#
#    def __str__(self):
#        return self.text
#
#    def __repr__(self):
#        return "(Word: " + self.text + ")"
#
#    def __len__(self):
#        return len(self.text)
#
#
#
#first = Word("AAA")
#second = Word("aAa")
#
#print(first == second)
#print(first != second)
#
#print(first)
#print(len(first))
#print(repr(first))



#class Bill():
#    def __init__(self, desc):
#        self.desc = desc
#
#class Tail():
#    def __init__(self, length):
#        self.length = length
#
#class Duck():
#    def __init__(self, bill, tail):
#        self.bill = bill
#        self.tail = tail
#
#
#    def about(self):
#        print("This duck has a", bill.desc, "bill and a", tail.length, "tail")
#
#
#bill = Bill("wide orange")
#tail = Tail("long")
#duck = Duck(bill, tail)
#duck.about()



#from collections import namedtuple
#
#
##Duck = namedtuple('Duck', 'bill tail')
#Duck = namedtuple('Duck', ('bill', 'tail'))
#duck = Duck("wide orange", "long")
#print(duck)
#print(repr(duck))
#print(duck.bill, duck.tail)
#
#
#parts = {"bill": "wide orange", "tail": "long" }
#duck2 = Duck(**parts)
#print(duck2, duck2.bill, duck2.tail)
#
#
#duck3 = duck2._replace(tail= "mag", bill = "crushing")
#print(duck2, duck2.bill, duck2.tail)
#print(duck3, duck3.bill, duck3.tail)
#
#duck_dict = {
#        "bill": "wide orange",
#        "tail": "long",
#        }
#print(duck_dict)


#class Thing():
#    pass
#
#example = Thing()
#
#print(Thing)
#print(example)

#class Thing2():
#    letters = 'abc'
#
#letters = Thing2()
#print(Thing2.letters)


#class Thing3():
#    def __init__(self):
#        self.letters = "xyz"
#
#obj = Thing3()
#print(obj.letters)


#class Element():
#    def __init__(self, name, symbol, number):
#        self.name = name
#        self.symbol = symbol
#        self.number = number
#
#    def dump(self):
#        print("name, symbol, number: [", self.name, ",", self.symbol, ",", self.number, "]")
#
#
#    def __str__(self):
#        return "name: " + self.name \
#                + ", symbol " + self.symbol \
#                + ", number " + str(self.number)
#
#obj = Element('Hydrogen', 'H', 1)
#obj.dump()
#
#
#args= {
#    "name": "Hydrogen",
#    "symbol": "H",
#    "number": 1
#    }
#hydrogen = Element(**args)
#hydrogen.dump()
#
#print(hydrogen)


#class Bear():
#    def eats(self):
#        return "Berries"
#
#class Rabbit():
#    def eats(self):
#        return "clover"
#
#class Octothorpe():
#    def eats(self):
#        return "campers"
#
#
#bear = Bear()
#rabbit = Rabbit()
#octothorpe = Octothorpe()
#
#def animal_eats(obj):
#    print(obj.eats())
#
#animal_eats(bear)
#animal_eats(rabbit)
#animal_eats(octothorpe)


class Laser():
    def dose(self):
	return "disinterat"

class Claw():
    def dose(self):
        return "crush"

class SmartPhone():
    def dose(self):
        return "ring"

def does(obj):
    print(obj.dose())

laser = Laser()
claw = Claw()
smartphone = SmartPhone()

does(laser)
does(claw)
does(smartphone)


