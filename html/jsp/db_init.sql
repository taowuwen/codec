create database if not exists db_student default character set utf8 default collate utf8_general_ci;
use db_student;

drop table if exists t_user;
drop table if exists t_score;
drop table if exists t_student;
drop table if exists t_course;

create table t_user (id int not null auto_increment, name varchar(255) not null, pass varchar(255) not null, primary key(id)) default character set utf8 default collate utf8_general_ci;
create table t_student(id int not null auto_increment, stu_id varchar(255) not null, name varchar(255) not null, sex int(1) not null, grade int(4) not null, major int(2) not null, detail varchar(255) default NULL, primary key(id), unique(stu_id))default character set utf8 default collate utf8_general_ci;
create table t_course(id int not null auto_increment, name varchar(255) not null, type int not null, credit float not null, grade int not null, major int(2) not null, detail varchar(255), primary key(id))default character set utf8 default collate utf8_general_ci;
create table t_score(stu_id int not null, course_id int not null, score float not null, primary key(stu_id, course_id), foreign key(stu_id) references t_student(id), foreign key(course_id) references t_course(id))default character set utf8 default collate utf8_general_ci;


insert into t_user (name, pass) values('admin', 'Abc123#');
insert into t_user (name, pass) values('user1', '123456');
insert into t_user (name, pass) values('user2', 'abcdef');

insert into t_student(stu_id, name, sex, grade, major) values('00001', '王亮', 0, 2015, 1);
insert into t_student(stu_id, name, sex, grade, major) values('00002', '李树国', 0, 2016, 1);
insert into t_student(stu_id, name, sex, grade, major) values('00003', '赵欣', 1, 2016, 1);
insert into t_student(stu_id, name, sex, grade, major) values('00004', '陶米', 1, 2015, 2);
insert into t_student(stu_id, name, sex, grade, major) values('00005', 'wangliang', 0, 2016, 2);

insert into t_course(name, type, credit, grade,major) values('Java程序设计', 4, 3, 2015, 1);
insert into t_course(name, type, credit, grade,major) values('Web程序设计', 5, 2, 2015, 1);
insert into t_course(name, type, credit, grade,major) values('JSP程序设计', 5, 2, 2015, 2);
insert into t_course(name, type, credit, grade,major) values('OS', 5, 2, 2016, 1);
insert into t_course(name, type, credit, grade,major) values('Data Struct', 5, 2, 2016, 2);



insert into t_score(stu_id, course_id, score) values(1, 1, 85);
insert into t_score(stu_id, course_id, score) values(1, 2, 90);
insert into t_score(stu_id, course_id, score) values(3, 3, 95);
