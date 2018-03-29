

-- create database if not exists db_student default character set utf8 default collate utf8_general_ci;
-- use db_student;
-- 
-- drop table if exists t_user;
-- drop table if exists t_score;
-- drop table if exists t_student;
-- drop table if exists t_course;
-- 
-- create table t_user (id int not null auto_increment, name varchar(255) not null, pass varchar(255) not null, primary key(id)) default character set utf8 default collate utf8_general_ci;
-- create table t_student(id int not null auto_increment, stu_id varchar(255) not null, name varchar(255) not null, sex int(1) not null, grade int(4) not null, major int(2) not null, detail varchar(255) default NULL, primary key(id), unique(stu_id))default character set utf8 default collate utf8_general_ci;
-- create table t_course(id int not null auto_increment, name varchar(255) not null, type int not null, credit float not null, grade int not null, major int(2) not null, detail varchar(255), primary key(id))default character set utf8 default collate utf8_general_ci;
-- create table t_score(stu_id int not null, course_id int not null, score float not null, primary key(stu_id, course_id), foreign key(stu_id) references t_student(id), foreign key(course_id) references t_course(id))default character set utf8 default collate utf8_general_ci;
-- 
-- 
-- insert into t_user (name, pass) values('admin', 'Abc123#');
-- insert into t_user (name, pass) values('user1', '123456');
-- insert into t_user (name, pass) values('user2', 'abcdef');
-- 
-- insert into t_student(stu_id, name, sex, grade, major) values('00001', '王亮', 0, 2015, 1);
-- insert into t_student(stu_id, name, sex, grade, major) values('00002', '李树国', 0, 2016, 1);
-- insert into t_student(stu_id, name, sex, grade, major) values('00003', '赵欣', 1, 2016, 1);
-- insert into t_student(stu_id, name, sex, grade, major) values('00004', '陶米', 1, 2015, 2);
-- insert into t_student(stu_id, name, sex, grade, major) values('00005', 'wangliang', 0, 2016, 2);
-- 
-- insert into t_course(name, type, credit, grade,major) values('Java程序设计', 4, 3, 2015, 1);
-- insert into t_course(name, type, credit, grade,major) values('Web程序设计', 5, 2, 2015, 1);
-- insert into t_course(name, type, credit, grade,major) values('JSP程序设计', 5, 2, 2015, 2);
-- insert into t_course(name, type, credit, grade,major) values('OS', 5, 2, 2016, 1);
-- insert into t_course(name, type, credit, grade,major) values('Data Struct', 5, 2, 2016, 2);
-- 
-- 
-- 
-- insert into t_score(stu_id, course_id, score) values(1, 1, 85);
-- insert into t_score(stu_id, course_id, score) values(1, 2, 90);
-- insert into t_score(stu_id, course_id, score) values(3, 3, 95);

create database if not exists db_tasks default character set utf8 default collate utf8_general_ci;
use db_tasks;

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
	id int not null auto_increment,
	name varchar(64) not null,
	bgcolor varchar(12) not null default "#aabbcc",
	primary key(id)
);

create table t_repeat (
	id int not null,
	name varchar(64) not null,
	primary key(id)
);

create table t_tasks (
	id int not null auto_increment,
	title varchar(256) not null,
	tm_start long not null,
	tm_end long,
	do_repeat int default 0,
	detail varchar(1024) default null ,

	id_ty int default 1,
	primary key(id),
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
	id int not null auto_increment,
	name varchar(64) not null,
	pass varchar(64) not null,

	email varchar(64) not null,
	telphone varchar(64) default null,
	state int default 1,
	isadmin int default 0,
	enable_notify int default 1,

	primary key(id)
);


create table t_group (
	id int not null auto_increment,
	name varchar(128) not null,
	primary key(id)
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

insert into t_tasks( title, tm_start, tm_end) values("go shopping tomorrow", (select unix_timestamp() + 3600), (select unix_timestamp() + 7200));

select * from t_type 
where id not in (select id_type from t_type_user where id_user != (select id from t_user where name="taowuwen"));

select * from t_type 
where id not in (select id_type from t_type_user where id_user != (select id from t_user where name="test"));

select * from t_type 
where id not in (select id_type from t_type_user where id_user != (select id from t_user where name="tww"));

create view v_task as select t.*, ty.name,ty.bgcolor from t_tasks t inner join t_type ty on t.id_ty = ty.id;
