#show databases;
#use information_schema;
#use mysql;
#show tables;

#show variables like 'char%';
#show variables like 'collation%';
#select * from character_sets;


#create database school;
#show databases;
#drop database school;
#show databases;


use school;
show tables;

#drop table Student;
#drop tables Course;
#drop tables SC;

create table Student (
	Sno CHAR(9) PRIMARY KEY,
    Sname CHAR(20) UNIQUE,
    Ssex CHAR(2),
    Sage SMALLINT,
    Sdept CHAR(20)    
);

create table Course (
	Cno CHAR(4) PRIMARY KEY,
    Cname CHAR(40) UNIQUE,
    Cpno CHAR(4), /* pre-cource*/
    Ccredit SMALLINT,
    FOREIGN KEY (Cpno) references Course(Cno)
);

create table SC (
	Sno CHAR(9),
    Cno CHAR(4),
    Grade SMALLINT,
    PRIMARY KEY (Sno, Cno),
    FOREIGN KEY (Sno) references Student(Sno),
    foreign key (Cno) references Course (Cno)
);