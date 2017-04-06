#!/usr/bin/env python


from sources import daily, weekly

print("Daidy weather: ", daily.forcast())
print("Week Forcast")

for num, outlook in enumerate(weekly.forcast(), 1):
	print(num, outlook)

