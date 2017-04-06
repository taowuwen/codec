#!/usr/bin/env python3
# -*- coding: utf-8 -*-


#years_list = [ y for y in range(1987, 1987 + 6) ]
years_list = [ y + 1987 for y in range(6) ]
#years_list = list(range(1987, 1987 + 6));
print(years_list)


print("birthday = ", years_list[0])
print("oldest = ", years_list[-1])



things = ["mozzarella", "cinderella", "salmonella" ]
print(things)

for word in things:
    print(word, "capitalize() = ", word.capitalize())

print(things)


for word in things:
    print(word, "upper() = ", word.upper())


del things[0];
print(things)

things.remove('salmonella');

print(things)



surprise = ['Groucho', 'Chico', 'Harpo'];

for word in surprise:
    print(word, 'lower, reverse, capitalize = ', word.lower(), word.swapcase(), word.capitalize())



e2f = {
        "dog":"chien",
        "cat":"chat",
        "walrus": "morse"
        };

print("e2f, walrus = ", e2f['walrus']);


key = e2f.keys();
val = e2f.values();

print("key = ", key)
print("val = ", val)

l = [item for item in zip(val, key)]

print(l)

f2e = dict([item for item in zip(val, key)])
print(f2e)
print(f2e['chien'])


english_words = { word for word in e2f.keys()}

print("english_words = ", english_words)








life = {
        "animals" : {
            "cats": ["Henri", "grumpy", "Lucy"],
            "octopi": {},
            "emus": {}
            },

        "plants": {} ,
        "other" : {}
       }

print("life.keys() = ", list(life.keys()))
print("animals = ", life['animals'])
print("animals-> cats", life['animals']['cats'])
