
drop table if exists t_group_user;
drop table if exists t_group_task;
drop table if exists t_user_task;
drop table if exists t_task_repeat;
drop table if exists t_type_user;
drop table if exists t_tasks;
drop table if exists t_repeat;
drop table if exists t_user;
drop table if exists t_group;
drop table if exists t_type;

drop view if exists v_task;


create table t_type (
	id INTEGER PRIMARY KEY NOT NULL,
	name varchar(64) not null,
	bgcolor varchar(12) not null default "#aabbcc"
);

create table t_repeat (
	id int not null,
	name varchar(64) not null,
	primary key(id)
);

create table t_tasks (
	id INTEGER PRIMARY KEY NOT NULL,
	title varchar(256) not null,
	tm_start long not null,
	tm_end long,
	do_repeat int default 0,
	detail varchar(1024) default null ,

	id_ty int default 1,
	foreign key(id_ty) references t_type(id)
);

create table t_task_repeat (
	id_task int not null,
	id_repeat int not null,
	primary key(id_task, id_repeat),
	foreign key(id_task) references t_tasks(id),
	foreign key(id_repeat) references t_repeat(id)
);


create table t_user (
	id INTEGER PRIMARY KEY NOT NULL,
	name varchar(64) not null,
	pass varchar(64) not null,

	email varchar(64) not null,
	telphone varchar(64) default null,
	state int default 1,
	isadmin int default 0,
	enable_notify int default 1
);


create table t_group (
	id INTEGER PRIMARY KEY NOT NULL,
	name varchar(128) not null
);

create table t_group_task (
	id_grp int not null,
	id_task int not null,
	primary key(id_grp, id_task),
	foreign key(id_grp) references t_group(id),
	foreign key(id_task) references t_tasks(id)
);

create table t_group_user (
	id_grp  int not null,
	id_user int not null,
	primary key(id_grp, id_user),
	foreign key(id_grp) references t_group(id),
	foreign key(id_user) references t_user(id)
);

create table t_user_task (
	id_user int not null,
	id_task int not null,
	primary key(id_user, id_task),
	foreign key(id_user) references t_user(id),
	foreign key(id_task) references t_tasks(id)
);

create table t_type_user (
	id_user int not null,
	id_type int not null,
	primary key(id_user, id_type),
	foreign key(id_user) references t_user(id),
	foreign key(id_type) references t_type(id)
);

insert into t_repeat(id, name) values(1, "每周一"),(2, "每周二"),(3, "每周三"), (4, "每周四"), (5, "每周五"), (6, "每周六"), (7, "每周日");
insert into t_user(name, pass, email) values("admin", "admin", "taowuwen@126.com");

insert into t_type(name, bgcolor) values("Default", "#ffffff");
insert into t_type(name, bgcolor) values("Red", "#aa0000");
insert into t_type(name, bgcolor) values("Green", "#00aa00");
insert into t_type(name, bgcolor) values("Blue", "#0000aa");

insert into t_user(name, pass, email) values("taowuwen", "123456", "taowuwen@126.com");
insert into t_user(name, pass, email) values("tww", "123456", "taowuwen@126.com");
insert into t_user(name, pass, email) values("test", "123456", "taowuwen@126.com");

insert into t_type(name, bgcolor) values("红色", "red");
insert into t_type(name, bgcolor) values("黄色", "yellow");
insert into t_type(name, bgcolor) values("绿色", "green");

insert into t_type_user(id_user, id_type) values(
	(select id from t_user where name="taowuwen"), 
	(select id from t_type where name="红色")
);

insert into t_type_user(id_user, id_type) values(
	(select id from t_user where name="tww"), 
	(select id from t_type where name="黄色")
);

insert into t_type_user(id_user, id_type) values(
	(select id from t_user where name="test"), 
	(select id from t_type where name="绿色")
);

insert into t_tasks( title, tm_start, tm_end) values("go shopping tomorrow", (select strftime('%s', 'now') + 3600), (select strftime('%s', 'now') + 7200));

select * from t_type 
where id not in (select id_type from t_type_user where id_user != (select id from t_user where name="taowuwen"));

select * from t_type 
where id not in (select id_type from t_type_user where id_user != (select id from t_user where name="test"));

select * from t_type 
where id not in (select id_type from t_type_user where id_user != (select id from t_user where name="tww"));

create view v_task as select t.*, ty.name,ty.bgcolor from t_tasks t inner join t_type ty on t.id_ty = ty.id;
