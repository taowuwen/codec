#!/usr/bin/env bash

DB_ROOT=$cgi_root/db
DB=$DB_ROOT/tasks.db

reset_db() {
	rm -f $DB
	sqlite3 $DB <$DB_ROOT/sql/db_tasks.sql.sqlite3
}


#CREATE TABLE t_user (
#id INTEGER PRIMARY KEY NOT NULL,
#name varchar(64) not null,
#pass varchar(64) not null,
#
#email varchar(64) not null,
#telphone varchar(64) default null,
#state int default 1,
#isadmin int default 0,
#enable_notify int default 1
#);

get_user_info()
{
	eval "sqlite3 $DB 'select * from t_user where $*;'"
}

get_user_by_id()
{
	get_user_info id="\"$1\""
}

get_user_by_name()
{
	get_user_info name="\"$1\""
}

get_user_by_email()
{
	get_user_info email="\"$1\""
}

get_user_by_telephone()
{
	[ -n "$1" ] && get_user_info telphone="\"$1\""
}

get_user_by_user_pass()
{
	get_user_info name="\"$1\"" and pass="\"$2\""
}

db_list_all_users()
{
	sqlite3 $DB 'select * from t_user'
}


db_set_user_lock()
{
	local uid=$1
	eval "sqlite3 $DB 'update t_user set state=0 where id=$uid;'"
}

db_set_user_unlock()
{
	local uid=$1
	eval "sqlite3 $DB 'update t_user set state=1 where id=$uid;'"
}

db_set_user_delete()
{
	local uid=$1
	eval "sqlite3 $DB 'delete from t_user where id=$uid';"
}


add_user() 
{
	local user pass email telphone

	user="$1"
	pass="$2"
	email="$3"
	telphone=$4

	eval "sqlite3 $DB 'insert into t_user(name, pass, email, telphone) values(\"$user\",\"$pass\", \"$email\", \"${4:null}\");'"
}

