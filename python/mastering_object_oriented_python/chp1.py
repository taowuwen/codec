#!/usr/bin/env python
# -*- coding: utf-8 -*-

import random

u_suits = {
	"club":		'\033[01;32m\u2663\033[00m',
	"diamond":	'\033[01;31m\u2666\033[00m',
	"heart": 	'\033[01;33m\u2665\033[00m',
	"spade":	'\033[01;34m\u2660\033[00m',
}


# 1.1
#class X:
#	pass
#
#
#for item in dir(X):
#	value = eval("X.{}".format(item))
#	print("X.{} = {}".format(item, value))
#	#print("dir({}) = {}".format(item, dir(value)))
#
#print(X.__class__)
#print(X.__class__.__base__)


# 1.2
#class Rectangle:
#	def area(self):
#		return self.length * self.width
#
#
#r = Rectangle()
#r.length, r.width   = 5, 9
#print(r.area())
#
#
#print(dir(r))

# 1.3
#class Card:
#	def __init__(self, rank, suit):
#		self.suit = suit
#		self.rank = rank
#		self.hard, self.soft = self._points()


#class NumberCard(Card):
#	def _points(self):
#		return int(self.rank), int(self.rank)
#
#class AceCard(Card):
#	def _points(self):
#		return 1, 11
#
#class FaceCard(Card):
#	def _points(self):
#		return 10, 10

#cards = [AceCard('A', u_suits.get("spade")), 
#	 NumberCard('2', u_suits.get("heart")),
#	 NumberCard('3', u_suits.get("diamond")),
#	 FaceCard("K",   u_suits.get("club"))
#	]
#
#print(cards)
#for card in cards:
#	print(card.rank, card.suit)

# 1.4

class Suit:
	def __init__(self, name, symbol):
		self.name = name
		self.symbol = symbol

	def __str__(self):
		return self.symbol

	def __repr__(self):
		return "<" + self.name +"> "+ self.symbol


Club, Diamond, Heart, Spade = Suit("Club", u_suits.get("club")),\
		Suit("Diamond", u_suits.get("diamond")), \
		Suit("Heart",   u_suits.get("heart")), \
		Suit("Spade",	u_suits.get("spade"))

Suits = {
	"Club": Club,
	"Diamond": Diamond,
	"Heart": Heart,
	"Spade": Spade
}

#cards = [AceCard('A', Spade),
#	 NumberCard('2', Heart),
#	 NumberCard('3', Diamond),
#	 FaceCard("K",   Club)] 

#for card in cards:
#	print(card.rank, card.suit)

# 1.5
#def card(rank, suit):
#	if rank == 1:
#		return AceCard("A", suit)
#
#	elif 2 <= rank <= 10:
#		return NumberCard(str(rank), suit)
#
#	elif 11 <= rank <= 13:
#		name = { 11: "J", 12 :"Q", 13: "K"}[rank]
#		return FaceCard(name, suit)
#
#	else:
#		raise Exception("Rank out of range")
#
#deck = [ card(rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#for c in deck:
#	print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


# 1.5.1
# not a good design
#def card2(rank, suit):
#	if rank == 1:
#		return AceCard("A", suit)
#
#	elif 2 <= rank <= 10:
#		return NumberCard(str(rank), suit)
#
#	else:
#		name = { 11: "J", 12 :"Q", 13: "K"}[rank]
#		return FaceCard(name, suit)
#
#deck2 = [ card2(rank, suit) for rank in range(14) for suit in Suits.values() ]
#for c in deck2:
#	print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


# 1.5.2
# use elif
#def card3(rank, suit):
#	if rank == 1:
#		return AceCard("A", suit)
#
#	elif 2 <= rank <= 10:
#		return NumberCard(str(rank), suit)
#
#	elif rank == 11:
#		return FaceCard("J", suit)
#
#	elif rank == 12:
#		return FaceCard("Q", suit)
#
#	elif rank == 13:
#		return FaceCard("K", suit)
#
#	else:
#		raise Exception("Rank out of range")
#
#
#deck3 = [ card3(rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#for c in deck3:
#	print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


# 1.5.3
# use map
#def card4(rank, suit):
#	class_ = {1: AceCard, 11: FaceCard, 12: FaceCard, 13: FaceCard}.get(rank, NumberCard)
#
#	return class_(rank, suit)
#
#deck4 = [ card4(rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#for c in deck4:
#	print("Version4: {0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


# use defaultdict
# 1.5.3.0
#from collections import defaultdict
#card5 = defaultdict(lambda: NumberCard, {1: AceCard, 11: FaceCard, 12: FaceCard, 13: FaceCard})
#deck5 = [ card5[rank](rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#for c in deck5:
#	print("Version5: {0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


# 1.5.3.a
#def card6(rank, suit):
#	class_ = {1: AceCard, 11: FaceCard, 12: FaceCard, 13: FaceCard}.get(rank, NumberCard)
#	rank_str = {1: 'A', 11: 'J', 12: 'Q', 13: 'K'}.get(rank, str(rank))
#
#	return class_(rank_str, suit)
#
#deck = [ card6(rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#for c in deck:
#	print("Version6: {0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


# 1.5.3.b
#def card7(rank, suit):
#
#	class_, rank_str = {
#		1:	(AceCard,  "A"),
#		11:	(FaceCard, "J"),
#		12:	(FaceCard, "Q"),
#		13:	(FaceCard, "K")
#		}.get(rank, (NumberCard, str(rank)))
#
#	return class_(rank_str, suit)
#
#deck = [ card7(rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#for c in deck:
#	print("Version7: {0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))
# 1.5.3.c

#def card8(rank, suit):
#	from functools import partial
#
#	part_class = {
#		1:	partial(AceCard,  "A"),
#		11:	partial(FaceCard, "J"),
#		12:	partial(FaceCard, "Q"),
#		13:	partial(FaceCard, "K")
#	}.get(rank, partial(NumberCard, str(rank)))
#
#	return part_class(suit)
#
#deck = [ card8(rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#for c in deck:
#	print("Version8: {0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))

# 1.5.3.d
# 流线型设计
#class CardFactory:
#	def rank(self, rank):
#		self.class_, self.rank_str = {
#			1:	(AceCard,  "A"),
#			11:	(FaceCard, "J"),
#			12:	(FaceCard, "Q"),
#			13:	(FaceCard, "K")
#		}.get(rank, (NumberCard, str(rank)))
#		return self
#
#	def suit(self, suit):
#		return self.class_(self.rank_str, suit)
#
#cf = CardFactory()
#deck = [ cf.rank(rank + 1).suit(suit) for rank in range(13) for suit in Suits.values() ]
#for c in deck:
#	print("Version8: {0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))



# 1.6

# 1.6.a
#class Card:
#	pass
#
#class NumberCard(Card):
#	def __init__(self, rank, suit):
#		self.rank = rank
#		self.suit = suit
#		self.hard = self.soft = rank
#
#
#class AceCard(Card):
#	def __init__(self, rank, suit):
#		self.rank = 'A'
#		self.suit = suit
#		self.hard, self.soft = 1, 11
#
#class FaceCard(Card):
#	def __init__(self, rank, suit):
#		self.rank = {11: 'J', 12: 'Q', 13: 'K'}[rank]
#		self.suit = suit
#		self.hard, self.soft = 10


# 1.6.b

class Card:
	insure = False
	def __init__(self, rank, suit, hard, soft):
		self.rank = rank
		self.suit = suit
		self.hard = hard
		self.soft = soft


class NumberCard(Card):
	def __init__(self, rank, suit):
		super().__init__(str(rank), suit, rank, rank)

class AceCard(Card):
	def __init__(self, rank, suit):
		super().__init__("A", suit, 1, 11)

class FaceCard(Card):
	def __init__(self, rank, suit):
		super().__init__({11: 'J', 12: 'Q', 13: 'K'}[rank], suit, 10, 10)


def card10(rank, suit):
	if rank == 1:
		return AceCard("A", suit)

	elif 2 <= rank <= 10:
		return NumberCard(rank, suit)

	elif 11 <= rank <= 13:
		return FaceCard(rank, suit)

	else:
		raise Exception("Rank out of range")

#import random
#deck = [ card10(rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#random.shuffle(deck)
#
#hand = [deck.pop(), deck.pop()]
#for c in hand:
#	print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


# 1.7
#class Deck():
#	def __init__(self):
#		import random
#		self._cards = [ card10(r + 1, s) \
#				for r in range(13) \
#				for s in Suits.values() ]
#
#		random.shuffle(self._cards)
#
#	def pop(self):
#		return self._cards.pop()
#
#d = Deck()
#
#hand = [ d.pop(), d.pop(), d.pop(), d.pop()]
#for c in hand:
#	print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))

# 1.7.2
#class Deck2(list):
#	def __init__(self):
#		super().__init__(card10(r + 1, s) \
#				for r in range(13) \
#				for s in Suits.values())
#		random.shuffle(self)
#
#d = Deck2()
#
#hand = [ d.pop(), d.pop(), d.pop(), d.pop()]
#for c in hand:
#	print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))

#class Deck3(list):
#	def __init__(self, decks=1):
#		super().__init__()
#
#		for i in range(decks):
#			self.extend(card10(r + 1, s) \
#				for r in range(13) \
#				for s in Suits.values())
#		random.shuffle(self)
#
#		burn = random.randint(1, 52)
#		for i in range(burn):
#			self.pop()
#
#d = Deck3(decks = 6)
#
#hand = [ d.pop(), d.pop(), d.pop(), d.pop()]
#for c in hand:
#	print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))

# 1.7.3
class Deck4(list):
	def __init__(self, decks=1):
		super().__init__(card10(r+1, s) \
				for i in range(decks) \
				for r in range(13) \
				for s in Suits.values())

		random.shuffle(self)

		burn = random.randint(1, 52)
		for i in range(burn):
			self.pop()

Deck = Deck4
#d = Deck4(decks = 6)
#
#hand = [ d.pop(), d.pop(), d.pop(), d.pop()]
#for c in hand:
#	print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


# 1.8

# 1.8.a
#class Hand():
#	def __init__(self, dealer_card):
#		self.dealer_card = dealer_card
#		self.cards = list()
#
#	def hard_total(self):
#		return sum(c.hard for c in self.cards)
#
#	def soft_total(self):
#		return sum(c.soft for c in self.cards)
#
#	def print(self):
#		for c in self.cards:
#			print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))
#d = Deck(decks = 2)
#h = Hand(d.pop())
#
#h.cards.extend([d.pop(), d.pop()])
#
#print(h.hard_total())
#print(h.soft_total())
#h.print()

class Hand2():
	def __init__(self, dealer_card, *cards):
		self.dealer_card = dealer_card
		self.cards = list(cards)

	def hard_total(self):
		return sum(c.hard for c in self.cards)

	def soft_total(self):
		return sum(c.soft for c in self.cards)

	def print(self):
		for c in self.cards:
			print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))

Hand = Hand2
#d = Deck(decks = 2)
#h = Hand(d.pop(), d.pop(), d.pop())
#
#print(h.hard_total())
#print(h.soft_total())
#h.print()




# 1.9
class GameStrategy:
	def insurance(self, hand):
		return False

	def split(self, hand):
		return False

	def double(self, hand):
		return False

	def hit(self, hand):
		return hand.hard_total() <= 17

#dumb = GameStrategy()
#print(dumb.insurance(h))
#print(dumb.split(h))
#print(dumb.double(h))
#print(dumb.hit(h))

# 1.10
class Table:
	def __init__(self):
		self.deck = Deck(decks = 3)

	def place_bet(self, amount):
		print("Bet", amount)

	def get_hand(self):
		try:
			d = self.deck
			self.hand = Hand(d.pop(), d.pop(), d.pop())
			self.hole_card = d.pop()

		except IndexError:
			""" out of cards: need to shuffle """
			print("get new deck")
			self.deck = Deck(decks = 2)
			return self.get_hand()

		print ("Deal", self.hand)
		return self.hand

	def can_insure(self, hand):
		return hand.dealer_card.insure


# 1.10.a

#class  BettingStrategy:
#	def bet(self):
#		raise NotImplementedError("No bed method")
#
#	def record_win(self):
#		pass
#
#	def record_loss(self):
#		pass
#
#class Flat(BettingStrategy):
#	def bet(self):
#		return 1


#b =  BettingStrategy()
#b.bet()
#b.record_win()
#b.record_loss()
#
#f = Flat()
#f.bet()
#f.record_win()
#f.record_loss()

# 1.10.b
import abc
class BettingStrategy2(metaclass = abc.ABCMeta):
	@abc.abstractmethod
	def bet(self):
		return 1

	def record_win(self):
		pass

	def record_loss(self):
		pass

BettingStrategy = BettingStrategy2

class Flat(BettingStrategy):
	def bet(self):
		return 1

#b = Flat()

# 1.11

class Hand3:
	def __init__(self, *args, **kw):
		if len(args) == 1 and isinstance(args[0], Hand3):
			other = args[0]
			self.dealer_card = other.dealer_card
			self.cards = other.cards

		else:
			dealer_card, *cards = args
			self.dealer_card = dealer_card
			self.cards = list(cards)

	def print(self):
		for c in self.cards:
			print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))

Hand = Hand3

#d = Deck(decks=2)
#
#h = Hand(d.pop(), d.pop(), d.pop())
#h.print()
#
#memento = Hand(h)
#memento.print()

# 1.11.1

class Hand4:
	def __init__(self, *args, **kw):
		if len(args) == 1 and isinstance(args[0], Hand4):
			other = args[0]
			self.dealer_card = other.dealer_card
			self.cards = other.cards

		elif len(args) == 2 and isinstance(args[0], Hand4) \
				and 'split' in kw:
			other, card = args
			self.dealer_card = other.dealer_card
			self.cards = [other.cards[kw['split']], card]

		elif len(args) == 3:
			dealer_card, *cards = args
			self.dealer_card = dealer_card
			self.cards = list(cards)

		else:
			raise TypeError("Invalid constructor args={0!r} kw={1!r}".format(args, kw))

	def __str__(self):
		return ", ".join(map(str, self.cards))

	def print(self):
		for c in self.cards:
			print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))

Hand = Hand4
#d = Deck(decks = 2)
#h = Hand(d.pop(), d.pop(), d.pop())
#s1 = Hand(h, d.pop(), split = 0)
#s2 = Hand(h, d.pop(), split = 1)
#
#h.print()
#print(h)
#
#s1.print()
#print(s1)
#
#s2.print()
#print(s2)

# 1.11.2

class Hand5:
	def __init__(self, dealer_card, *cards):
		self.dealer_card =dealer_card
		self.cards = list(cards)

	@staticmethod
	def freeze(other):
		hand = Hand5(other.dealer_card, *other.cards)
		return hand

	@staticmethod
	def split(other, card0, card1):
		hand0 = Hand5(other.dealer_card, other.cards[0], card0)
		hand1 = Hand5(other.dealer_card, other.cards[1], card1)

		return hand0, hand1

	def __str__(self):
	#	return ", ".join( map(str, self.cards) )
		return ", ".join("{rank}{suit}".format(rank=str(c.rank), suit=str(c.suit))\
				for c in self.cards)

	def print(self):
		print(", ".join("{rank}{suit}".format(rank=str(c.rank), suit=str(c.suit))\
				for c in self.cards))

	#	for c in self.cards:
	#		print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))


Hand = Hand5

#d = Deck(decks = 3)
#h = Hand(d.pop(), d.pop(), d.pop())
#s1, s2 = Hand.split(h, d.pop(), d.pop())
#
#print(h)
#print(s1)
#print(s2)


# 1.12

table = Table()
flat_bet = Flat()
dumb = GameStrategy()

#class Player:
#	def __init__(self, table, bet_strategy, game_strategy):
#		self.bet_strategy = bet_strategy
#		self.game_strategy = game_strategy
#		self.table = table
#
#	def game(self):
#		self.table.place_bet(self.bet_strategy.bet())
#		self.hand = self.table.get_hand()
#
#		if self.table.can_insure(self.hand):
#			if self.game_strategy.insurance(self.hand):
#				self.table.insure(self.bet_strategy.bet())
#
#
#p = Player(table, flat_bet, dumb)
#p.game()
#
#
#class Player2:
#	def __init__(self, **kw):
#		self.__dict__.update(kw)
#
#	def game(self):
#		self.table.place_bet(self.bet_strategy.bet())
#		self.hand = self.table.get_hand()
#
#		if self.table.can_insure(self.hand):
#			if self.game_strategy.insurance(self.hand):
#				self.table.insure(self.bet_strategy.bet())
#
#Player = Player2
#p = Player(table=table, bet_strategy = flat_bet, game_strategy=dumb)
#p.game()
#
#p = Player(table=table, bet_strategy = flat_bet, game_strategy=dumb, log_name = "hello, log")
#p.game()


#class Player3:
#	def __init__(self, table, bet_strategy, game_strategy, **kw):
#		self.table = table
#		self.bet_strategy = bet_strategy
#		self.game_strategy = game_strategy
#		self.__dict__.update(kw)
#
#	def game(self):
#		self.table.place_bet(self.bet_strategy.bet())
#		self.hand = self.table.get_hand()
#
#		if self.table.can_insure(self.hand):
#			if self.game_strategy.insurance(self.hand):
#				self.table.insure(self.bet_strategy.bet())
#
#Player = Player3
#p = Player(table, flat_bet, dumb)
#p.game()
#print(dir(p))
#
#p = Player(table, flat_bet, dumb, log_name = "hello, log")
#p.game()
#print(dir(p))

# 1.12.1

class ValidPlayer:
	def __init__(self, table, bet_strategy, game_strategy):

		assert isinstance(table, Table)
		assert isinstance(bet_strategy, BettingStrategy)
		assert isinstance(game_strategy, GameStrategy)

		self.table = table
		self.bet_strategy = bet_strategy
		self.game_strategy = game_strategy

	def game(self):

		self.table.place_bet(self.bet_strategy.bet())
		self.hand = self.table.get_hand()

		assert isinstance(self.hand, Hand)

		if self.table.can_insure(self.hand):
			if self.game_strategy.insurance(self.hand):
				self.table.insure(self.bet_strategy.bet())

Player = ValidPlayer
p = Player(table, flat_bet, dumb)
p.game()
