#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import datetime
import time


class TimedEvent:
	def __init__(self, endtime, callback):
		self.endtime = endtime
		self.callback = callback

	def ready(self):
		return self.endtime <= datetime.datetime.now()


class Timer:
	def __init__(self):
		self.events = []

	def call_after(self, delay, callback):
		end_time = datetime.datetime.now() + datetime.timedelta(seconds=delay)

		self.events.append(TimedEvent(end_time, callback))

	def run(self):
		while True and len(self.events) > 0:
			ready_events = (e for e in self.events if e.ready())

			for evt in ready_events:
				evt.callback(self)
				self.events.remove(evt)

			time.sleep(0.5)


def format_time(msg, *args):
	now = datetime.datetime.now().strftime("%I:%M:%S")
	print(msg.format(*args, now=now))


def one(timer):
	format_time("{now}: called ONE")


def two(timer):
	format_time("{now}: called TWO")


def three(timer):
	format_time("{now}: called THREE")


class Repeater:
	def __init__(self):
		self.count = 0

	def repeater(self, timer):
		format_time("{now}: repeat {0}", self.count)
		self.count += 1
		timer.call_after(5, self.repeater)


def main():
	print("hello, tests timedevent and timer")

	timer = Timer()

	timer.call_after(1, one)
	timer.call_after(2, one)
	timer.call_after(2, two)
	timer.call_after(4, two)
	timer.call_after(3, three)
	timer.call_after(6, three)

	repeater = Repeater()

	timer.call_after(5, repeater.repeater)

	format_time("{now}: starting")

	timer.run()


if __name__ == '__main__':
	main()
