create table t1(
  ts BIG INT,
  dur BIG INT,
  part BIG INT,
  PRIMARY KEY (part, ts)
) without rowid;

create table t2(
  ts BIG INT,
  dur BIG INT,
  part BIG INT,
  PRIMARY KEY (part, ts)
) without rowid;

-- Insert a single row into t1.
INSERT INTO t1(ts, dur, part)
VALUES (500, 100, 10);

-- t2 is empty.

create virtual table sp using span_outer_join(t1 PARTITIONED part, 
                                             t2 PARTITIONED part);

select * from sp;
