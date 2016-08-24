-- @description : : Update Partitioned Updatable AO tables 
-- 

-- Create AO part tables

DROP TABLE if exists sto_uao_p1_upd;
CREATE TABLE  sto_uao_p1_upd
 (
 col1 bigint, col2 date, col3 text, col4 int) with(appendonly=true, orientation=column)
 distributed randomly  partition by range(col2)
 subpartition by list (col3)
 subpartition template ( default subpartition subothers, subpartition sub1 values ('one'), subpartition sub2 values ('two')) 
 (default partition others, start(date '2008-01-01') end(date '2008-04-30') every(interval '1 month'));

SELECT 1  AS VisimapPresent FROM pg_appendonly WHERE visimapidxid is not NULL AND visimapidxid is not NULL AND relid=(SELECT oid  FROM pg_class WHERE relname='sto_uao_p1_upd');
\d+ sto_uao_p1_upd;
insert into sto_uao_p1_upd(col1, col2, col3) values (1, '2008-01-02', 'one'), (2, '2008-01-02', 'two'), (3, '2008-01-02', 'three');
insert into sto_uao_p1_upd(col1, col2, col3) values (2, '2008-02-03', 'two'), (2, '2008-02-03', 'one'), (3, '2008-02-03', 'three');
insert into sto_uao_p1_upd(col1, col2, col3)  values(3, '2008-03-04', 'three'),(1, '2008-04-04', 'one'), (2, '2008-03-04', 'two') ;
insert into sto_uao_p1_upd(col1, col2, col3) values (4, '2008-04-05', 'one'), (2, '2008-04-05', 'two'), (3, '2008-04-05', 'three');
insert into sto_uao_p1_upd(col1, col2, col3) values (5, '2008-05-06', 'one'), (2, '2008-05-06', 'two'), (3, '2008-05-06', 'three');

select count(*) from sto_uao_p1_upd ;
update sto_uao_p1_upd set col4=22 where col3='two' and col1=2;
select count(*) AS only_visi_tups  from sto_uao_p1_upd;
set gp_select_invisible = true;
select count(*) AS invisi_and_visi_tups  from sto_uao_p1_upd;
set gp_select_invisible = false;

-- AO Mixed part tables
-- Mixed part tables
Drop table if exists sto_uao_mx_p1_upd;
create table sto_uao_mx_p1_upd
(
col1 bigint, col2 date, col3 text, col4 int) with(appendonly=true, orientation=column)
distributed randomly  partition by range(col2)
subpartition by list (col3)
subpartition template (
default subpartition subothers,
subpartition sub1 values ('one'),
subpartition sub2 values ('two') with(appendonly = false),
subpartition sub3 values ('three') with(appendonly=true, orientation=column))
(default partition others, start(date '2008-01-01') end(date '2008-04-30') every(interval '1 month'));

SELECT 1  AS VisimapPresent FROM pg_appendonly WHERE visimapidxid is not NULL AND visimapidxid is not NULL AND relid=(SELECT oid  FROM pg_class WHERE relname='sto_uao_mx_p1_upd');

insert into sto_uao_mx_p1_upd(col1, col2, col3) values (1, '2008-01-02', 'one'), (2, '2008-01-02', 'two'), (3, '2008-01-02', 'three'), (4, '2008-01-02' ,'four');
insert into sto_uao_mx_p1_upd(col1, col2, col3) values (1, '2008-02-03', 'two'), (2, '2008-02-03', 'one'), (3, '2008-02-03', 'three'), (4, '2008-02-03' ,'four');
insert into sto_uao_mx_p1_upd(col1, col2, col3)  values(1, '2008-03-04', 'three'),(1, '2008-04-04', 'one'), (2, '2008-03-04', 'two'), (4, '2008-03-04' ,'four') ;
insert into sto_uao_mx_p1_upd(col1, col2, col3) values (1, '2008-04-05', 'one'), (2, '2008-04-05', 'two'), (3, '2008-04-05', 'three'), (4, '2008-04-05', 'four');
insert into sto_uao_mx_p1_upd(col1, col2, col3) values (1, '2008-05-06', 'one'), (2, '2008-05-06', 'two'), (3, '2008-05-06', 'three'), (4, '2008-05-06', 'four');

select count(*) from sto_uao_mx_p1_upd;

update sto_uao_mx_p1_upd set col4=22 where col3='two' and col1=2;
select count(*) AS only_visi_tups  from sto_uao_mx_p1_upd;
set gp_select_invisible = true;
select count(*) AS invisi_and_visi_tups  from sto_uao_mx_p1_upd;
set gp_select_invisible = false;

