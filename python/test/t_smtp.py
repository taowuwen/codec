#!/usr/bin/env python3
# -*- coding: utf-8 -*-



import smtplib
from email.mime.text import MIMEText


def send_email(subject, message, from_addr, *to_address,
		host="localhost", port=1025, headers = None):
	email = MIMEText(message)
	email['Subject'] = subject
	email['From'] = from_addr

	headers = {} if headers is None else headers

	for header, value in headers.items():
		email[header] = value


	sender = smtplib.SMTP(host, port)

	for addr in to_address:
		del email['To']
		email['To'] = addr
		sender.sendmail(from_addr, addr, email.as_string())

	sender.quit()



from collections import defaultdict

class MaillingList:

	def __init__(self):
		self.email_map = defaultdict(set)

	def add_to_group(self, email, group):
		self.email_map[email].add(group)

	def emails_in_groups(self, *groups):
		groups = set(groups)

		return {e for (e, g) in self.email_map.items() if g & groups}

	def send_mailling(self, subject, message, from_addr, *groups, **kwargs):
		emails = self.emails_in_groups(*groups)
		send_email(subject, message, from_addr, *emails, **kwargs)



def main():
	print("hello, test smtplib for now")

	send_email("A model subject", "The message contents",
			"from@example.com",
			"to1@example.com",
			"to2@example.com")

	m = MaillingList()

	m.add_to_group("friend1@example.com", "friends")
	m.add_to_group("friend2@example.com", "friends")
	m.add_to_group("friend3@example.com", "friends")
	m.add_to_group("family@example.com", "family")
	m.add_to_group("pro1@example.com", "professional")
	m.add_to_group("aaa@example.com", "family")
	m.add_to_group("aaa@example.com", "friends")


	m.send_mailling("A Party",
			"Friends and family only: a party",
			"me@example.com",
			"friends",
			"family", 
			headers={"Reply-to": "me2@example.com"})

if __name__ == '__main__':
	main()


