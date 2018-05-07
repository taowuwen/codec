#!/usr/bin/env bash

DB_ROOT=$cgi_root/db
DB=$DB_ROOT/tasks.db

reset_db() {
	rm -f $DB
	sqlite3 $DB <$DB_ROOT/sql/db_tasks.sql.sqlite3
}

