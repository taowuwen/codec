#!/usr/bin/env python
# -*- coding: utf-8 -*-

import random
import sys

from collections import defaultdict

u_suits = {
	"club":		'\033[01;32m\u2663\033[00m',
	"diamond":	'\033[01;31m\u2666\033[00m',
	"heart": 	'\033[01;33m\u2665\033[00m',
	"spade":	'\033[01;34m\u2660\033[00m',
}



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

class Card:
	insure = False
	def __init__(self, rank, suit, hard, soft):
		self.rank = rank
		self.suit = suit
		self.hard = hard
		self.soft = soft

	def __repr__(self):
		return "{__class__.__name__}(suit={suit!r}, rank={rank!r})".\
				format(__class__=self.__class__, **self.__dict__)

	def __str__(self):
		return "{rank}{suit}".format(**self.__dict__)

	def __format__(self, format_spec):
		if format_spec == "":
			return str(self)

		rs = format_spec.replace("%r", self.rank).replace("%s", str(self.suit))
		rs = rs.replace("%%", "%")
		return rs


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

import random
deck = [ card10(rank, suit) for rank in range(1, 14) for suit in Suits.values() ]
#random.shuffle(deck)

for c in deck:
	print("format:{0:%r%s}".format(c)+ ", string: " + str(c) + ", repr: " + repr(c))

#hand = [deck.pop(), deck.pop()]
#
#print("dealer has {0:%r of %s}".format(hand.dealer_card))
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

#class Hand2():
#	def __init__(self, dealer_card, *cards):
#		self.dealer_card = dealer_card
#		self.cards = list(cards)
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
#
#Hand = Hand2
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

#class Hand3:
#	def __init__(self, *args, **kw):
#		if len(args) == 1 and isinstance(args[0], Hand3):
#			other = args[0]
#			self.dealer_card = other.dealer_card
#			self.cards = other.cards
#
#		else:
#			dealer_card, *cards = args
#			self.dealer_card = dealer_card
#			self.cards = list(cards)
#
#	def print(self):
#		for c in self.cards:
#			print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))
#
#Hand = Hand3

#d = Deck(decks=2)
#
#h = Hand(d.pop(), d.pop(), d.pop())
#h.print()
#
#memento = Hand(h)
#memento.print()

# 1.11.1

#class Hand4:
#	def __init__(self, *args, **kw):
#		if len(args) == 1 and isinstance(args[0], Hand4):
#			other = args[0]
#			self.dealer_card = other.dealer_card
#			self.cards = other.cards
#
#		elif len(args) == 2 and isinstance(args[0], Hand4) \
#				and 'split' in kw:
#			other, card = args
#			self.dealer_card = other.dealer_card
#			self.cards = [other.cards[kw['split']], card]
#
#		elif len(args) == 3:
#			dealer_card, *cards = args
#			self.dealer_card = dealer_card
#			self.cards = list(cards)
#
#		else:
#			raise TypeError("Invalid constructor args={0!r} kw={1!r}".format(args, kw))
#
#	def __str__(self):
#		return ", ".join(map(str, self.cards))
#
#	def print(self):
#		for c in self.cards:
#			print("{0:<10s}{1:>2s}".format(str(c.rank), str(c.suit)))
#
#Hand = Hand4
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
		return " ".join(map(str, self.cards))

	def __repr__(self):
		#	return "[" + ", ".join(map(repr, self.cards)) + "]"
		return "{__class__.__name__}({dealer_card!r}, {_cards_str}))".\
				format(__class__=self.__class__,
						_cards_str = ",".join(map(repr, self.cards)),
						**self.__dict__
						)

	def __format__(self, format_spec):
		if format_spec == "":
			return str(self)

		return ", ".join("{0:{fs}}".format(c, fs=format_spec) for c in self.cards)


Hand = Hand5

d = Deck(decks = 3)
h = Hand(d.pop(), d.pop(), d.pop())
s1, s2 = Hand.split(h, d.pop(), d.pop())

print(str(h), repr(h))
print(str(s1), repr(s1))
print(str(s2), repr(s2))

print("dealer_card is: {0:%r%s}".format(h.dealer_card))
print("Player: {hand: %r%s}".format(hand = h))

print(hash(h.dealer_card))
print(id(h.dealer_card))
print(id(h.dealer_card)//16)


c1 = AceCard(1, Club)
c2 = AceCard(1, Club)

print(c1, c2)
print(id(c1), id(c2))
print(c1 is c2)
print(c1 == c2)
print(hash(c1), hash(c2))

print(set([c1, c2]))



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

#class ValidPlayer:
#	def __init__(self, table, bet_strategy, game_strategy):
#
#		assert isinstance(table, Table)
#		assert isinstance(bet_strategy, BettingStrategy)
#		assert isinstance(game_strategy, GameStrategy)
#
#		self.table = table
#		self.bet_strategy = bet_strategy
#		self.game_strategy = game_strategy
#
#	def game(self):
#
#		self.table.place_bet(self.bet_strategy.bet())
#		self.hand = self.table.get_hand()
#
#		assert isinstance(self.hand, Hand)
#
#		if self.table.can_insure(self.hand):
#			if self.game_strategy.insurance(self.hand):
#				self.table.insure(self.bet_strategy.bet())
#
#Player = ValidPlayer
#p = Player(table, flat_bet, dumb)
#p.game()

# 2.3.3
class Card2:
	insure = False
	def __init__(self, rank, suit, hard, soft):
		self.rank = rank
		self.suit = suit
		self.hard = hard
		self.soft = soft

	def __repr__(self):
		return "{__class__.__name__}(suit={suit!r}, rank={rank!r})".\
				format(__class__=self.__class__, **self.__dict__)

	def __str__(self):
		return "{rank}{suit}".format(**self.__dict__)

	def __format__(self, format_spec):
		if format_spec == "":
			return str(self)

		rs = format_spec.replace("%r", self.rank).replace("%s", str(self.suit))
		rs = rs.replace("%%", "%")
		return rs

	def __hash__(self):
		return hash(self.suit) ^ hash(self.rank)

	def __bytes__(self):

		class_node = self.__class__.__name__[0]

		rank_number_str = {'A': '1', 'J': '11', 'Q': '12', 'K': '13'}.get(self.rank, self.rank)

		string = "(" + " ".join([class_node, rank_number_str, str(self.suit),]) + ")"

		return bytes(string, encoding="utf-8")

	def __gt__(self, other):
		try:
			return self.rank > other.rank
		except AttributeError:
			return NotImplemented


	def __ge__(self, other):
		if not isinstance(other, Card2):
			return NotImplemented

		return self.rank >= other.rank


	def __eq__(self, other):
		try:
			return self.rank == other.rank and self.suit == other.suit
		except:
			return NotImplemented

	def __ne__(self, other):
		try:
			return self.rank != other.rank or self.suit != other.suit
		except:
			return NotImplemented

	def __str__(self):
		return "{rank}{suit}".format(**self.__dict__)



class NumberCard2(Card2):
	insure = False
	def __init__(self, rank, suit):
		super().__init__(str(rank), suit, rank, rank)

class AceCard2(Card2):
	insure = True
	def __init__(self, rank, suit):
		super().__init__("A", suit, 1, 11)

class FaceCard2(Card2):
	insure = True
	def __init__(self, rank, suit):
		super().__init__({11: 'J', 12: 'Q', 13: 'K'}[rank], suit, 10, 10)


Card = Card2
NumberCard = NumberCard2
AceCard = AceCard2
FaceCard = FaceCard2


print("================================")

c1 = AceCard(1, Club)
c2 = AceCard(1, Club)

print(c1, c2)
print(id(c1), id(c2))
print(c1 is c2)
print(c1 == c2)
print(hash(c1), hash(c2))

print(set([c1, c2]))



# 2.3.4
class Card3:
	insure = False
	def __init__(self, rank, suit, hard, soft):
		self.rank = rank
		self.suit = suit
		self.hard = hard
		self.soft = soft

	def __repr__(self):
		return "{__class__.__name__}(suit={suit!r}, rank={rank!r})".\
				format(__class__=self.__class__, **self.__dict__)

	def __str__(self):
		return "{rank}{suit}".format(**self.__dict__)

	def __format__(self, format_spec):
		if format_spec == "":
			return str(self)

		rs = format_spec.replace("%r", self.rank).replace("%s", str(self.suit))
		rs = rs.replace("%%", "%")
		return rs

	def __eq__(self, other):
		return self.suit == other.suit and self.rank == other.rank

	def __lt__(self, other):
		return self.rank < other.rank

	__hash__ = None


class NumberCard3(Card3):
	insure = False
	def __init__(self, rank, suit):
		super().__init__(str(rank), suit, rank, rank)

class AceCard3(Card3):
	insure = True
	def __init__(self, rank, suit):
		super().__init__("A", suit, 1, 11)

class FaceCard3(Card3):
	insure = True
	def __init__(self, rank, suit):
		super().__init__({11: 'J', 12: 'Q', 13: 'K'}[rank], suit, 10, 10)


Card = Card3
NumberCard = NumberCard3
AceCard = AceCard3
FaceCard = FaceCard3


print("================================")

c1 = AceCard(1, Club)
c2 = AceCard(1, Club)
c3 = NumberCard(2, Club)
c4 = FaceCard(11, Club)


print(c1, c2)
print(id(c1), id(c2))
print(c1 is c2)
print(c1 == c2)
print(c1 < c2)
print(c1 < c3)
print(c1 < c4)
#print(hash(c1), hash(c2))
#print(set([c1, c2]))

Card = Card2
NumberCard = NumberCard2
AceCard = AceCard2
FaceCard = FaceCard2

print("============ update hand ================================")
class Hand6:
	def __init__(self, dealer_card, *cards):
		self.dealer_card =dealer_card
		self.cards = list(cards)

	@staticmethod
	def freeze(other):
		hand = Hand6(other.dealer_card, *other.cards)
		return hand

	@staticmethod
	def split(other, card0, card1):
		hand0 = Hand6(other.dealer_card, other.cards[0], card0)
		hand1 = Hand6(other.dealer_card, other.cards[1], card1)

		return hand0, hand1

	def __str__(self):
		return " ".join(map(str, self.cards))

	def __repr__(self):
		#	return "[" + ", ".join(map(repr, self.cards)) + "]"
		return "{__class__.__name__}({dealer_card!r}, {_cards_str}))".\
				format(__class__=self.__class__,
						_cards_str = ",".join(map(repr, self.cards)),
						**self.__dict__
						)

	def __format__(self, format_spec):
		if format_spec == "":
			return str(self)

		return ", ".join("{0:{fs}}".format(c, fs=format_spec) for c in self.cards)

	def __lt__(self, other):

		if isinstance(other, int):
			return self.total() < other

		try:
			return self.total() < other.total()

		except AttributeError:
			return NotImplemented


	def __gt__(self, other):

		if isinstance(other, int):
			return self.total() > other

		try:
			return self.total() > other.total()

		except AttributeError:
			return NotImplemented



	def __ge__(self, other):
		if isinstance(other, int):
			return self.total() >= other
		try:
			return self.total() >= other.total()

		except AttributeError:
			return NotImplemented

	def __le__(self, other):
		if isinstance(other, int):
			return self.total() <= other
		try:
			return self.total() <= other.total()

		except AttributeError:
			return NotImplemented


	def __eq__(self, other):
		if isinstance(other, int):
			return self.total() == other
		
		try:
			return (self.cards == other.cards) and \
				(self.dealer_card == other.dealer_card)
		except:
			return NotImplemented

	def __ne__(self, other):
		if isinstance(other, int):
			return self.total() != other
		
		try:
			return (self.cards != other.cards) or \
				(self.dealer_card != other.dealer_card)
		except:
			return NotImplemented

	def total(self):
		delta_soft = max(c.soft - c.hard for c in self.cards)

		hard = sum(c.hard for c in self.cards)

		if hard + delta_soft <= 21:
			return hard + delta_soft

		return hard


	__hash__ = None


Hand = Hand6

class FrozenHand(Hand):
	def __init__(self, *args, **kw):
		if len(args) == 1 and isinstance(args[0], Hand):
			other = args[0]
			self.dealer_card = other.dealer_card
			self.cards = other.cards

		else:
			super().__init__(*args, **kw)

	def __hash__(self):
		h = 0

		for c in self.cards:
			h = (h + hash(c)) % sys.hash_info.modulus

		return h

d = Deck(decks = 3)
h = Hand(d.pop(), d.pop(), d.pop())
s1, s2 = Hand.split(h, d.pop(), d.pop())



print("===================")
print(h, ">>>>> ", repr(h))
print(s1, ">>>>> ", repr(s1))
print(s2, ">>>>> ", repr(s2))

print("h.total = ", h.total())
print("s1.total = ", s1.total())
print("s2.total = ", s2.total())

print(h == s1)
print(h == h)
print(h > s1)
print(h > s2)

print(h == 13)
print(s2 < 15)

print(s1 > 18)
print(s1 >= 18)
print(s1 < 18)
print(s1 <= 18)
print(s1 == 18)

print("===================")


print("dealer_card is: {0:%r%s}".format(h.dealer_card))
print("Player: {hand: %r%s}".format(hand = h))

h_f = FrozenHand(h)

stats = defaultdict(int)
stats[h_f] += 1

print(stats)

#def card_from_bytes(buffer):
#	string = buffer.decode("utf-8")
#
#	assert string[0] == "(" and string[-1] == ")"
#
#	code, rank_num, suit = string[1:-1].split()
#
#	class_ = {'A': AceCard,  'N': NumberCard, 'F': FaceCard }[code]
#
#	return class_(int(rank_num), suit)
#
#
#d = Deck()
#while d:
#	c = d.pop()
#	b_c = bytes(c)
#	crd = card_from_bytes(b_c)
#	print(c, ">>>",  b_c, ">>>>", crd)
#



#define 2.6.1

#class BlackJackCard_p:
#	def __init__(self, rank, suit):
#		self.rank = rank
#		self.suit = suit
#
#	def __lt__(self, other):
#		return self.rank < other.rank
#
#	def __str__(self):
#		return "{rank}{suit}".format(**self.__dict__)
#
#
#two = BlackJackCard_p(2, Club)
#three = BlackJackCard_p(3, Club)
#print(two, three)
#print(two < three)


#class BlackJackCard_p:
#	def __init__(self, rank, suit, hard, soft):
#		self.rank = rank
#		self.suit = suit
#		self.hard = hard
#		self.soft = soft
#
#	def __lt__(self, other):
#		try:
#			return self.rank < other.rank
#		except AttributeError:
#			return NotImplemented
#
#
#	def __gt__(self, other):
#		if not isinstance(other, BlackJackCard_p):
#			return NotImplemented
#
#		return self.rank > other.rank
#
#
#	def __ge__(self, other):
#		if not isinstance(other, BlackJackCard_p):
#			return NotImplemented
#
#		return self.rank >= other.rank
#
#
#	def __eq__(self, other):
#		try:
#			return self.rank == other.rank and self.suit == other.suit
#		except:
#			return NotImplemented
#
#	def __ne__(self, other):
#		try:
#			return self.rank != other.rank or self.suit != other.suit
#		except:
#			return NotImplemented
#
#	def __str__(self):
#		return "{rank}{suit}".format(**self.__dict__)
#
#
#
#
#
#class AceBlackJack(BlackJackCard_p):
#	def __init__(self, rank, suit):
#		super().__init__('A', suit, 1, 11)
#
#class NumberBlackJack(BlackJackCard_p):
#	def __init__(self, rank, suit):
#		super().__init__(str(rank), suit, rank, rank)
#
#class FaceBlackJack(BlackJackCard_p):
#	def __init__(self, rank, suit):
#		super().__init__({11: 'J', 12: 'Q', 13: 'K'}[rank], suit, 10, 10)
#
#
#
#def card21(rank, suit):
#	class_ = { 1: AceBlackJack, 11: FaceBlackJack, 12: FaceBlackJack, 13: FaceBlackJack}.get(rank, NumberBlackJack)
#
#	return class_(rank, suit)



#b_cards = [ card21(r+1, s) for r in range(13) for s in Suits.values()]
#for c in b_cards:
#	print(c)


#two = card21(2, Club)
#three = card21(3, Club)
#two_c = card21(2, Spade)
#
#def do_print(*args):
#	print(" ".join(map(str, args)), *args)
#
#print(two, three)
#print(two == two_c)
#print(two >= three)
#print(two.rank == two_c.rank)
#print(two != three)
#print(two == three)
#
#print("two == 2 ?", two == 2)


#class Noisy:
#	def __del__(self):
#		print("Removing {0}".format(id(self)))
#
#
#x = Noisy()
#del x
#
#
#ln = [Noisy(), Noisy()]
#
#ln2 = ln[:]
#
#for item in ln2:
#	print("ln2", item)
#
#del ln
#
#
#for item in ln2:
#	print("ln2", item)
#


class Parent:
	def __init__(self, *children):
		self.children = list(children)
		for child in self.children:
			child.parent = self

	def __del__(self):
		print("Removing {__class__.__name__} {id:d}".
				format(__class__=self.__class__, id=id(self)))


class Child:
	def __del__(self):
		print("Removing {__class__.__name__} {id:d}".
				format(__class__=self.__class__, id=id(self)))



p = Parent(Child(), Child())

print(p)

del p

print("do import gc")

import gc
gc.collect()

print("do import weakref")

import weakref

class Parent2:
	def __init__(self, *children):
		self.children = list(children)
		for child in self.children:
			child.parent = weakref.ref(self)

	def __del__(self):
		print("Removing {__class__.__name__} {id:d}".
				format(__class__=self.__class__, id=id(self)))


class Child2:
	def __del__(self):
		p = self.parent()
		if p is not None:
			print(" >>> p : {}".format(p))
			self.parent = None

		else:
			print("Removing {__class__.__name__} {id:d}".
				format(__class__=self.__class__, id=id(self)))

p = Parent2(Child2(), Child2())

del p


print("do existing")





