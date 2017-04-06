show databases;

drop database school;
create database school;

use school;

create table Student (
    Sno CHAR(10)	PRIMARY KEY,
    Sname CHAR(20)	UNIQUE,
    Ssex  CHAR(2),
    Sage  smallint,
    Sdept CHAR(20)
);

create table Course(
    Cno CHAR(4)	PRIMARY KEY,
    Cname CHAR(20)	UNIQUE,
    Cpno  CHAR(4),
    Ccredit smallint,
    foreign key (Cpno) references Course(Cno)
);


create table SC (
    Sno CHAR(10),
    Cno CHAR(4),
    grade smallint,
    
    primary key(Sno, Cno),
    foreign key (Sno) references Student(Sno),
    foreign key (Cno) references Course(Cno)
);



insert into Student(Sno, Sname, Ssex, Sage, Sdept) values('200215121', 'liyong', 'M', 20, 'CS');
insert into Student(Sno, Sname, Ssex, Sage, Sdept) values('200215120', 'liuchen', 'F', 18, 'CS');
insert into Student(Sno, Sname, Ssex, Sage, Sdept) values('200215119', 'wangmin', 'F', 19, 'MA');
insert into Student(Sno, Sname, Ssex, Sage, Sdept) values('200215118', 'temp', 'M', 20, 'IS');
insert into Student(Sno, Sname, Ssex, Sage, Sdept) values('200215117', 'zhangli', 'M', 21, 'IS');
insert into Student(Sno, Sname, Ssex, Sage, Sdept) values('200215116', 'lishi', 'F', 22, 'MA');


insert into Course(Cno, Cname, Cpno, Ccredit) values('2', 'Math', 		NULL, 	2);
insert into Course(Cno, Cname, Cpno, Ccredit) values('6', 'Data Process', 	NULL, 	2);
insert into Course(Cno, Cname, Cpno, Ccredit) values('4', 'Operation System', 	'6', 	3);
insert into Course(Cno, Cname, Cpno, Ccredit) values('7', 'C Proamming', 	'6', 	4);
insert into Course(Cno, Cname, Cpno, Ccredit) values('5', 'Data Struct', 	'7', 	4);
insert into Course(Cno, Cname, Cpno, Ccredit) values('1', 'Database', 		'5', 	4);
insert into Course(Cno, Cname, Cpno, Ccredit) values('3', 'Information System', '1', 	4);


insert into SC(Sno, Cno, grade) values('200215121', '1', 92);
insert into SC(Sno, Cno, grade) values('200215121', '2', 85);
insert into SC(Sno, Cno, grade) values('200215121', '3', 82);
insert into SC(Sno, Cno, grade) values('200215120', '2', 90);
insert into SC(Sno, Cno, grade) values('200215120', '3', 83);
insert into SC(Sno, Cno, grade) values('200215119', '2', 80);

select * from Student;
select * from Course;
select * from SC;

select Student.*, SC.*
from Student, SC
where Student.Sno = SC.Sno;


select distinct Student.Sno, Sname, Sage
from Student, SC
where Student.Sno = SC.Sno;


select f.Cno F_NO, f.Cname F_NAME, s.Cno S_NO, s.Cname S_NAME
from Course f, Course s
where f.Cno = s.Cpno;

/* for out join */
select Student.Sno, Sname, Sage, Cno, grade
from Student left join SC 
	on (Student.Sno = SC.Sno)
order by Student.Sno desc, grade ASC;


select Sname
from Student
where Sno in (
	select Sno
	from SC
	where grade > 90
);



select Sno, Sname
from Student
where Sno in (
	select Sno
	from SC, Course
	where SC.Cno = Course.Cno and Course.Cname = 'Math'
);

select distinct s.Sno, s.Sname, 'Choosed: ', c.Cno, c.Cname
from Student as s right join SC as sc on (s.Sno = sc.Sno) left join Course as c on (c.Cno = sc.Cno)
order by s.Sno, c.Cno;


select Sname, Sage
from Student
where Sdept != 'CS' and Sage < any (
	select Sage
	from Student
	where Sdept = 'CS'
);


 create table course_avg(
	Cname	CHAR(20),
	Cavg	float
 );

insert into course_avg(Cname, Cavg)
	select Cname, avg(grade)
	from Course, SC
	where SC.Cno = Course.Cno
	group by SC.Cno;

select * from course_avg;


update Student set Sage=23 where Sno = '200215119';

select * from Student where Sno='200215119';

update Student set Sage = Sage - 10;
select * from Student;

select * from course_avg;
delete from course_avg where Cname = 'Information System';
select * from course_avg;


create view IS_Student
as
select Sno, Sname, Sage
from Student
where Sdept = 'IS';

select * from IS_Student;

create view IS_Student
as
select Sno, Sname, Sage
from Student
where Sdept = 'IS'
with check option;

select * from IS_Student;


create view IS_S2(Sno, Sname, grade)
AS
select Student.Sno, Sname, grade
from Student, SC
where Sdept = 'IS' AND Student.Sno = SC.Sno AND SC.Cno = '2';

select * from IS_Student;
select * from IS_S1;
select * from IS_S2;


create view IS_C1
as 
select Sno, Sname, grade
from IS_S1
where grade >= 90;

select * from IS_C1;

