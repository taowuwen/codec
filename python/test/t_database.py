#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sqlite3


sql_create="""
create table project (
		name        text primary key,
		description text,
		deadline    date
	);
create table task (
		id           integer primary key autoincrement not null,
		priority     integer default 1,
		details      text,
		status       text,
		deadline     date,
		completed_on date,
		project      text not null references project(name)
	);
"""

sql_insert="""
insert into project (name, description, deadline)
	values ('pymotw', 'Python Module of the Week', '2016-11-01');

insert into task (details, status, deadline, project)
	values ('write about select', 'done', '2016-04-25', 'pymotw');

insert into task (details, status, deadline, project)
	values ('write about random', 'waiting', '2016-08-22', 'pymotw');

insert into task (details, status, deadline, project)
	values ('write about sqlite3', 'active', '2017-07-31', 'pymotw');
"""


class TestSqlite:

    def __init__(self, fl="/tmp/tmp.db", *args, **kwargs):

        self.db = fl
        self.conn = None

        self.open(fl)

    def open(self, db=None):

        db_is_new = False
        if not os.path.exists(db):
            db_is_new = True

        try:
            self.conn = sqlite3.connect(db)

        except Exception as e:
            print(e)
            return None

        if db_is_new:
            self.conn.executescript(sql_create)
            self.conn.executescript(sql_insert)

        return self.conn

    def _cb_db(self, *args):
        print("|".join([str(a) for a in args]))

    def _query(self, sql=None):
        if not sql:
            return
        
        cursor = self.conn.cursor()
        cursor.execute(sql)

        return cursor

    def query(self, sql=None, cb=None):

        if not cb: cb = self._cb_db

        cursor = self._query(sql)
        for row in cursor.fetchall():
            cb(*row)

    def query_one(self, sql=None, cb=None):
        if not cb: cb = self._cb_db

        cursor = self._query(sql)
        cb(*cursor.fetchone())

    def query_many(self, sql=None, cb=None, many=5):
        if not cb: cb = self._cb_db

        cursor = self._query(sql)

        for row in cursor.fetchmany(many):
            print(row['id'])
            cb(*row)


    def dotest(self):
        print("test query")
        self.query(sql="select * from project")
        self.query(sql="select * from task")

        print("test query_one")

        self.query_one(sql="select * from project")
        self.query_one(sql="select * from task")

        print("test query_many")

        self.query_many(sql="select * from project", many=2)
        self.query_many(sql="select * from task", many=2)


    @classmethod
    def test(cls, *args, **kwargs):
        a = cls()
        a.dotest()

if __name__ == '__main__':
    print("test database")

    TestSqlite.test()
