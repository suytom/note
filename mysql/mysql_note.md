# <center>MySQL 8.4官方文档阅读笔记</center>
# 1、优化，文档地址：https://dev.mysql.com/doc/refman/8.4/en/optimization.html
+ COUNT(*) on a single table without a WHERE is retrieved directly from the table information for MyISAM and MEMORY tables. This is also done for any NOT NULL expression when used with only one table.（当在MyISAM或者MEMORY存储引擎的表执行没有where条件的count时是直接查询的表信息。）  
  也就是说如果存储引擎是MyISAM或者MEMORY的话，表的数据行数会保存在表信息中（SHOW TABLE STATUS LIKE '表名';）。InnoDB因为支持事务的原因，则不会有这个信息。

+ If you use the SQL_SMALL_RESULT modifier, MySQL uses an in-memory temporary table.(如果使用SQL_SMALL_RESULT修饰符，MySQL会使用内存临时表。)  
  优化提示，告诉MySQL这个查询的结果集会很小，建议使用内存临时表。  
  在执行某些SQL查询（如 GROUP BY、DISTINCT、ORDER BY、UNION等）时，MySQL可能会创建临时表来存储中间计算结果，以便提高查询效率。  
  explain时Extra列中如果出现Using temporary，则表示使用了临时表，```sql SHOW GLOBAL STATUS LIKE 'Created_tmp%tables'; ```Created_tmp_tables：创建的内存临时表数量。Created_tmp_disk_tables：创建的磁盘临时表数量。  
  影响使用内存临时表的参数:  
  1、tmp_table_size 和 max_heap_table_size  
  如果临时表的大小超过tmp_table_size或max_heap_table_size的较小值，MySQL会将其转换为磁盘临时表。  
  2、max_tmp_tables  
  单个会话可创建的内存临时表数量。  
  3、min_examined_row_limit  
  最少需要存入临时表的行数，如果GROUP BY、DISTINCT或ORDER BY结果的行数少于min_examined_row_limit，MySQL可能不会创建临时表。  
  4、TEXT和BLOB类型  
  MEMORY存储引擎不支持TEXT/BLOB，如果查询包含这些数据类型，MySQL只能使用磁盘临时表。  

+ By default, MySQL employs hash joins whenever possible. It is possible to control whether hash joins are employed using one of the BNL and NO_BNL optimizer hints.  
  Memory usage by hash joins can be controlled using the join_buffer_size system variable; a hash join cannot use more memory than this amount. When the memory required for a hash join exceeds the amount available, MySQL handles this by using files on disk. If this happens, you should be aware that the join may not succeed if a hash join cannot fit into memory and it creates more files than set for open_files_limit. To avoid such problems, make either of the following changes:  
    Increase join_buffer_size so that the hash join does not spill over to disk.  
    Increase open_files_limit.  
  Join buffers for hash joins are allocated incrementally; thus, you can set join_buffer_size higher without small queries allocating very large amounts of RAM, but outer joins allocate the entire buffer. Hash joins are used for outer joins (including antijoins and semijoins) as well, so this is no longer an issue.  
  
  

  MySQL执行JOIN查询时，会根据不同的数据特性（索引、数据量、查询条件）选择不同的JOIN算法。MySQL主要有以下几种JOIN方式：  
  1、嵌套循环连接（Nested Loop Join，NLJ）  
  2、块嵌套循环连接（Block Nested Loop Join，BNL）  
  3、哈希连接（Hash Join）  
  4、排序归并连接（Sort-Merge Join，Merge Join）  







  











# InnoDB、MyISAM差异
+ 事务支持：InnoDB支持事务处理，这意味着它支持ACID事务特性（原子性、一致性、隔离性和持久性）。而MyISAM不支持事务。
+ 行级锁定与表级锁定：InnoDB支持行级锁定，这意味着在多用户并发访问时，它只锁定被访问的行，而不是整个表，这可以提高并发性能。而MyISAM使用表级锁定，当一个线程访问表时，其他线程必须等待。
+ 外键支持：InnoDB支持外键约束，这是保证数据完整性的重要手段。MyISAM则不支持外键。
+ 存储结构：MyISAM总共有三个文件***.frm（表结构）、***.myd（表数据）、***.myi（表索引），而InnoDB只要两个文件***.frm（表结构）、***.ibd（表索引加数据）。此外，MyISAM的索引是基于B+树的，但叶子节点存储的是数据地址，而InnoDB的叶子节点直接存储数据。
+ Buffer Pool实际上是InnoDB存储引擎特有的一个组件，它用于缓存数据和索引，以加速对数据的访问。InnoDB通过将经常访问的数据和索引加载到Buffer Pool中，减少了磁盘I/O操作，从而提高了数据库的性能。MyISAM 存储引擎并不使用Buffer Pool来缓存数据。MyISAM有自己的缓存机制，但它并不使用与InnoDB相同的Buffer Pool概念。MyISAM更多地依赖于操作系统的文件系统缓存来缓存数据。

# 索引的分类
+ 主键索引：针对表中主键所建索引，只能有一个。
+ 唯一索引
+ 普通索引
+ 全文索引

# 聚簇索引
+ 聚簇索引（Clustered Index），也叫簇类索引、聚集索引或聚类索引，是一种对磁盘上实际数据重新组织以按指定的一个或多个列的值排序的索引方式。其特点在于，聚簇索引的索引页面指针指向数据页面，因此使用聚簇索引查找数据通常比使用非聚簇索引更快。
  
# 慢查询
+ 查看慢查询日志是否开启：show variables like 'slow_query_log';
+ MySQL的慢查询日志默认是没有开启的，需要在MySQL的配置文件（etc/my.cnf）中配置如下信息：
  - slow_query_log=1    开启慢查询日志开关
  - long_query_time=2   设置慢查询日志的时长，SQL语句执行时间超过2秒则会被视为慢查询，从而被记录。
  - 慢查询日志地址在/var/lib/mysql/localhost-slow.log。
  
# profile
+ 查看MySQL是否支持profile操作 select @@have_profiling;
+ 默认profile是关闭的，select @@profiling;（查看是否开启）set (global/session) profiling=1;（设置全局/会话开启）。
+ show profiles; 查看开启后每条SQL语句的执行耗时。

# 最左前缀法则
+ 使用联合索引时，最左边的列必须存在。比如联合索引（A、B、C），使用（A、B、C）、（A、B）、(A)会使用联合索引，使用（A、C）也会使用联合索引但其索引长度跟（A）一样，也就是说字段C的索引并没有用，也就是部分失效。
+ order by使用联合索引时，必须完全按照联合索引的顺序。
+ explain实例如下所示：
+ ![left_rule](./picture/left_rule.png)
  
# 索引失效
+ 对索引进行运算。
+ 使用字符串索引时没加单引号。
+ 模糊查询前面加百分号。
+ 使用or对条件进行组合时，只要有一个条件没有索引，那另外一个就算有索引也不会使用。
+ order by使用联合索引时，没有完全按照联合索引的顺序。

# 前缀索引
+ 当字段类型为字符串时，有时候需要索引很长的字符串，这会让索引变得很大，查询时，浪费大量的磁盘IO，影响查询性能，此时只对字符串的一部分前缀，建立索引，这样可以大大节约索引空间，从而提升性能。
+ create index idx_xxx on table_name(column(n)); n表示几个字符。
  
# 全局锁
+ 应用场景：数据备份（锁住后写入操作将会阻塞）。
+ 全局锁数据备份示例如下：
  - flush tables with read lock;
  - mysqldump -hxxx -uxxx -pxxx databasename > databasename.sql;（非SQL语句）
  - unlock tables;
  
# 表锁
+ lock tables xxx read/write;
+ unlock tables;
  
# 元数据锁（表级锁）
+ ![metadata](./picture/metadata.JPG)
+ 系统自动控制，无需主动显示调用，当一张表涉及到未提交的事务时，元数据锁会阻止对该表的结构进行修改。如下图所示，此时如果修改表结构则会阻塞。
+ ![metadata_lock](./picture/metadata_lock.png)
  
# 行锁、间隙锁（行级锁）、临键锁（行级锁）
+ $\color{#FF0000}{可以通过}$"performance_schema" $\color{#FF0000}{数据库的}$，"data_lock"$\color{#FF0000}{查看当前所持锁的信息。}$
+ 临键锁本质上其实就是行锁加间隙锁。只有在可重复读隔离级别下，如下所示情况下才会加间隙锁：
+ 1、字段是唯一索引时，只有更新某个范围记录或者一条不存在的记录的时候，才会加间隙锁，指定某条存在的记录时，只会加行锁，不会加间隙锁。
+ 2、字段是普通索引时，不管是单条还是多条都会加间隙锁。
+ $\color{#FF0000}{间隙锁的目的是为了防止其他事务在间隙中塞入数据，间隙锁之间是不互斥的，不同事务可以对相同的间隙加锁。（事务A有间隙锁后，事务B在间隙内插入会阻塞。）}$
![data](./picture/data.png)
+ 示例1：字段是唯一索引，更新不存在的记录。(update test_1 set score=100 where id=15;)
![gap_lock_1](./picture/gap_lock_1.png)
+ 示例2：字段是唯一索引，更新范围。(update test_1 set score=100 where id>=9 and id<15;)
![gap_lock_2](./picture/gap_lock_2.png)
+ 示例3：字段是普通索引，更新单行。（update test_1 set score=100 where name=10;）
![gap_lock_3](./picture/gap_lock_3.png)

# 每个表的数据量
+ 这并没有一个绝对值，网上不少说法都是说不能超过2000W条数据，这个数据的来源是：根节点的页可以存1200多个索引，那么第二层的索引个数是1200*1200=144W，如果每条数据的大小为1K，那么第三层每一页（16K）可以存16条数据，那么层高为3的B+树，可以存144W*16≈2300W条数据。其实每个表的最优数据条数得从多个方面来考量，比如索引效率，如果表的索引较多较复杂的话，数据条数过多的话，那么维护索引的性能就会越来越低。还有就是设置的buff poll大小，如下面介绍所示，数据库专属服务器buff poll建议设置为物理内存的百分之八十，其实简单的考量的话，buff poll的大小应该就等于单表数据量的大小，因为单表的数据可以都加载到内存中，那么性能也不会差。2000W条数据的来源可能就是基于这个，2000W条数据内存占用大概就是20G。

# buff pool
+ 官网介绍地址：https://dev.mysql.com/doc/refman/8.0/en/innodb-buffer-pool.html
+ On dedicated servers, up to 80% of physical memory is often assigned to the buffer pool.数据库专属服务器，建议分配百分之八十的物理内存作为buff pool。
![buff_pool](./picture/buffpool.png)
+ When InnoDB reads a page into the buffer pool, it initially inserts it at the midpoint (the head of the old sublist). A page can be read because it is required for a user-initiated operation such as an SQL query, or as part of a read-ahead operation performed automatically by InnoDB.
（当innodb读取数据到buff pool,先将其放到old sublist的头部，数据页被加载到缓存，那必然是玩家执行操作或者被innodb的预读机制加载。）
+ You can control the insertion point in the LRU list and choose whether InnoDB applies the same optimization to blocks brought into the buffer pool by table or index scans. The configuration parameter innodb_old_blocks_pct controls the percentage of “old” blocks in the LRU list. The default value of innodb_old_blocks_pct is 37, corresponding to the original fixed ratio of 3/8. The value range is 5 (new pages in the buffer pool age out very quickly) to 95 (only 5% of the buffer pool is reserved for hot pages, making the algorithm close to the familiar LRU strategy).
（主要是介绍innodb_old_blocks_pct参数，new sublist和old sublist的比例，默认是37。）
+ A data page is typically accessed a few times in quick succession and is never touched again. The configuration parameter innodb_old_blocks_time specifies the time window (in milliseconds) after the first access to a page during which it can be accessed without being moved to the front (most-recently used end) of the LRU list. The default value of innodb_old_blocks_time is 1000. Increasing this value makes more and more blocks likely to age out faster from the buffer pool.
（上面的意思是大部分时候数据被加载后可能只是短时间内访问，如果访问了就从old sublist挪到new sublist是不合理的，此时可以设置innodb_old_blocks_time，默认为1000毫秒，这个参数的意思是超过这个时间old sublist里的页还被访问就把它挪到new sublist。）

# undolog、redolog、binlog
+ undolog
  undolog的作用是保证事务的原子性以及实现多版本并发控制（MVCC）。undolog的数据格式是对应更新操作的相反语句，比如用户输入insert，那么在undolog中记录一条对应的delete。

+ redolog
  1、redolog它能保证对于已经COMMIT的事务产生的数据变更，即使是系统宕机崩溃也可以通过它来进行数据重做，达到数据的持久性，一旦事务成功提交后，不会因为异常、宕机而造成数据错误或丢失。
  2、当redolog空间满了之后又会从头开始以循环的方式进行覆盖式的写入。MySQL支持三种将redo log buffer写入redo log file的时机，可以通过innodb_flush_log_at_trx_commit参数配置，各参数含义如下：
  0（延迟写）：表示每次事务提交时都只是把redolog留在redolog buffer中，开启一个后台线程，每1s刷新一次到磁盘中;
  1（实时写，实时刷）：表示每次事务提交时都将redolog直接持久化到磁盘，真正保证数据的持久性；
  2（实时写，延迟刷）：表示每次事务提交时都只是把redolog写到page cache，具体的刷盘时机不确定。
  除了上面几种机制外，还有其它两种情况会把redo log buffer中的日志刷到磁盘。
  定时处理：有线程会定时(每隔 1 秒)把redo log buffer中的数据刷盘。
  根据空间处理：redo log buffer占用到了一定程度(innodb_log_buffer_size设置的值一半)占，这个时候也会把redo log buffer中的数据刷盘。
  3、redolog有两个阶段prepare和commit，在prepare阶段，事务的更改首先被写入redolog，并标记为“准备(prepare)”状态。这意味着更改尚未真正提交，只是暂时保存在redolog中。当事务提交后，进入commit阶段，MySQL会确保这些更改被持久化到磁盘中。$\color{#FF0000}{无论是用上面的何种写入配置，redolog的写入都是先从内存开始的。这通常涉及到将日志条目写入到InnoDB的redolog缓冲区中。这个缓冲区位于数据库服务器的内存中，以提供快速的写入性能。}$

+ binlog
  1、binlog记录了对MySQL数据库执行更改的所有的写操作，包括所有对数据库的数据、表结构、索引等等变更的操作。binlog的主要应用场景分别是主从复制和数据恢复，主从复制：在Master端开启binlog，然后将binlog发送到各个 Slave端，Slave端重放binlog来达到主从数据一致。数据恢复：通过使用mysqlbinlog工具来恢复数据。
  2、binlog日志有三种格式，分别为STATMENT、ROW和MIXED。
  STATMENT格式：保存执行的sql语句，但是如果语句中有随机数，会造成主从同步数据不一致。
  ROW格式：每次操作保存受影响的行，以及怎么变化。但是会产生大量的日志，比较浪费存储空间。
  MIXED格式：如果语句中没有随机数等，就保存sql语句，否则保存为row格式。
  3、通过 sync_binlog 参数控制 biglog 的刷盘时机，取值范围是 0-N：
  0：每次提交事务binlog不会马上写入到磁盘，而是先写到page cache。不去强制要求，由系统自行判断何时写入磁盘，在Mysql 崩溃的时候会有丢失日志的风险；
  1：每次提交事务都会执行fsync将binlog写入到磁盘；
  N：每次提交事务都先写到page cach，只有等到积累了N个事务之后才fsync将binlog写入到磁盘，在MySQL崩溃的时候会有丢失N个事务日志的风险。

# MVCC
![mvcc](./picture/mvcc.png)
+ undolog、read view以及三个隐藏列（DB_ROW_ID：记录的主键id（没有指定主键时才会有）。DB_TRX_ID：事务ID，当对某条记录发生修改时，就会将这个事务的Id记录其中。DB_ROLL_PTR︰回滚指针，版本链中的指针）
+ 在读已提交隔离级别下每次查询都会生成read view，即在该隔离级别下是当前读。
+ 在可重复读隔离级别下如果是普通的select，则只有在第一次执行select会生成read view，之后每次普通查询都不会再生成read view（即是当前读）。如果是加锁读（即select ... for update;或者select ... in share model;）则会生成read view。

# 主从复制
实现读写分离，降低主库的访问压力。从库只读需要修改配置，Mysql本身不提供路由功能，需要用插件或者自己来实现读写路由功能。
+ 工作原理
1. master记录二进制日志。在每个事务完成数据更新之前，master在二进制日志记录这些改变。MySQL将事务串行地写入二进制日志，完成后，master通知存储引擎提交事务。
2. slave将master的bin log拷贝到它自身的中继日志：
  首先，slave 开始一个工作线程：I/O 线程。
  然后，I/O 线程在 master 上打开一个普通的连接，然后开始binlog dump process。binlog dump process 从 master的二进制日志中读取事件（接收的单位是 event），如果已经跟上master，它会睡眠并等待master产生新的事件。
  最后，I/O线程将这些事件写入中继日志（relay log）。
3. slave重做中继日志
   Thread（SQL 从线程）是处理该过程的最后一步。SQL 线程从中继日志读取事件，并重放其中的事件（回放的单位也是 event）更新 slave 的数据，使其与 master 中的数据一致。只要该线程与 I/O 线程保持一致，中继日志通常会位于 OS 的缓存中，所以中继日志的开销很小。
+ 复制方式
1. 主从复制可分为异步复制、同步复制和半同步复制三种。
2. 异步复制：主库写binlog、从库I/O线程读binlog并写入relaylog、从库SQL线程重放事务这三步之间是异步的。异步复制的主库不需要关心备库的状态，主库不保证事务被传输到从库，如果主库崩溃，某些事务可能还未发送到从库，切换后可能导致事务的丢失。其优点是可以有更高的吞吐量，缺点是不能保持数据实时一致，不适合要求主从数据一致性要求较高的应用场景。
3. 同步复制：主库在提交事务前，必须确认事务在所有的备库上都已经完成提交。即主库是最后一个提交的，在提交前需要将事务传递给从库并完成重放、提交等一系列动作。其优点是任何时候主备库都是一致的，主库的崩溃不会丢失事务，缺点是由于主库需要等待备库先提交事务，吞吐量很低。
4. 半同步复制：主库在提交事务时先等待，必须确认至少一个从库收到了事件（从库将事件写入relay log，不需要重放和提交，并向主库发送一个确认信息ACK），主库收到确认信息后才会正式commit。

# 组复制
+ 架构图如下所示：
                              +------------------+
                              |  Application/API  |
                              +------------------+
                                      |
                                      |
                                      v
                              +------------------+
                              |  MySQL Server 1   |
                              +------------------+
                              |  MySQL Server 2   |
                              +------------------+
                              |  MySQL Server N   |
                              +------------------+

+ 在这个架构中：应用程序或API直接连接到MySQL服务器。所有MySQL服务器配置了组复制，并相互通信。一旦数据发生变化，会被复制到其他服务器。任何服务器都可以处理读取请求，提供了读取扩展性。$\color{#FF0000}{当主服务器失效时，其他服务器可以参与选举选出新的主服务器。}$组复制不涉及特定的代码，它是在MySQL服务器层面配置和管理的。

+ MySQL组复制架构本身并不自带读写路由功能。它主要关注的是数据的一致性和高可用性，通过在多个MySQL服务器之间同步数据，确保数据在所有服务器上的一致性，并在某个服务器发生故障时，其他服务器可以继续处理事务，从而保持服务的可用性。在实际应用中，为了实现读写分离（即将读操作和写操作分散到不同的服务器上），通常会结合使用其他工具或中间件来实现读写路由功能。这些工具或中间件可以根据业务需求，将读请求路由到从服务器，将写请求路由到主服务器，从而实现读写分离的目的。
  
+ 缺点：
  1、配置和管理复杂性：虽然MySQL组复制提供了简单的配置和管理界面，但在大型复杂环境中，正确配置和管理组复制可能仍然具有一定的挑战性。
  2、性能影响：虽然组复制的设计旨在最小化对性能的影响，但在高并发场景下，由于需要在多个服务器之间同步数据，可能会对性能产生一定的影响。
  3、网络依赖：组复制需要服务器之间保持稳定的网络连接。网络故障或延迟可能导致数据同步问题或性能下降。
  4、数据一致性的挑战：尽管组复制努力确保数据的一致性，但在某些极端情况下，例如网络分区或节点故障，仍可能出现数据一致性的问题。

# Innodb Cluster
+ InnoDB Cluster则是MySQL官方提供的一个完整的高可用性解决方案，它结合了组复制、InnoDB ReplicaSet以及MySQL Router等技术。InnoDB Cluster提供了自动故障转移功能，如果主节点出现故障，从节点会自动选举为主节点，MySQL Router检测到这一点后，会将客户端应用程序自动转发到新的主节点。此外，InnoDB Cluster还内置了数据一致性保证机制，确保了数据在集群中的一致性和完整性。
