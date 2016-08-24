-- @Description Tests the error message of updatable cursors on AO tables.
-- 

BEGIN;
declare c cursor for select * from ao where a = 90;
FETCH c;
update ao set b = 30 where current of c;
ROLLBACK;
