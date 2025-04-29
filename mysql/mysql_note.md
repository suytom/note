# <center>MySQL 8.4官方文档阅读笔记</center>
+ `COUNT(*) on a single table without a WHERE is retrieved directly from the table information for MyISAM and MEMORY tables. This is also done for any NOT NULL expression when used with only one table.`  
  也就是说如果存储引擎是MyISAM或者MEMORY的话，表的数据行数会保存在表信息中（SHOW TABLE STATUS LIKE '表名';）。InnoDB因为支持事务的原因，则不会有这个信息。

+ `If you use the SQL_SMALL_RESULT modifier, MySQL uses an in-memory temporary table.`  
  优化提示，告诉MySQL这个查询的结果集会很小，建议使用内存临时表。  
  在执行某些SQL查询（如 GROUP BY、DISTINCT、ORDER BY、UNION等）时，MySQL可能会创建临时表来存储中间计算结果，以便提高查询效率。  
  explain时Extra列中如果出现Using temporary，则表示使用了临时表，```sql SHOW GLOBAL STATUS LIKE 'Created_tmp%tables'; ```Created_tmp_tables：创建的内存临时表数量。Created_tmp_disk_tables：创建的磁盘临时表数量。  
  影响使用内存临时表的参数:  
  1、`tmp_table_size` 和 `max_heap_table_size`  
  如果临时表的大小超过tmp_table_size或max_heap_table_size的较小值，MySQL会将其转换为磁盘临时表。  
  2、`max_tmp_tables`  
  单个会话可创建的内存临时表数量。  
  3、`min_examined_row_limit`  
  最少需要存入临时表的行数，如果GROUP BY、DISTINCT或ORDER BY结果的行数少于`min_examined_row_limit`，MySQL可能不会创建临时表。  
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

+ Innodb Buffer Pool  
  + `On dedicated servers, up to 80% of physical memory is often assigned to the buffer pool.`专属服务器上，推荐百分之八十以上内存当作buff pool。
  + 影响Buffer Pool的一些配置字段：  
    + `innodb_buffer_pool_size`：Buff Pool大小，默认大小128M。
    + `innodb_buffer_pool_chunk_size`：每个chunk的大小，Buffer Pool是由若干chunk组成的。
    + `innodb_buffer_pool_instances`：将InnoDB缓冲池划分为几个实例，每个instance有独立的缓存页链表和LRU。页号跟instance所对应，多个instance能够提升并发性能。过多也会造成内存浪费。
    + `innodb_old_blocks_pct`：旧数据所占Buff Pool百分比，默认值37。
    + `innodb_old_blocks_time`：旧数据移动到新数据最少等待时间。因为数据被加载到buff pool，并不一定是用户行为，有可能是mysql的预读机制导致。数据被加载到buff pool时，先放在old area,在innodb_old_blocks_time期间再被访问也不会移到new area，在innodb_old_blocks_time之后再次被访问，才会被移动到new area。

+ Linear Read Ahead和Random Read Ahead  
  + 线性预读和随机预读，InnoDB没有使用系统的page cache机制，而是自己实现了一套缓存机制。InnoDB默认使用线性预读机制。
  + 影响预读的一些配置字段：
    + `innodb_read_ahead_threshold`：取值范围0-64，这个单位是MySQL页的单位（MySQL的页默认大小为16K），一个extent有64个页。当一个extent已经被加载到缓存中的页的数量达到限制时，会触发线性预读，加载下一整个extent到buff pool中。  
    PS：linux虚拟内存管理的基本单位是页，一般大小为4K。文件管理系统的基本单位是块，通常块和页的大小相等。扇区是磁盘的基本单位，块对应多个扇区。虽然SSD在物理设备中不存在扇区的概念，但为了兼容传统设备接口，在逻辑层依然维护扇区的抽象。
    + `innodb_random_read_ahead`：是否开启随机预读，如果一个extent中的连续13个页都被加载到buff pool中，会触发随机预读，将这个extent的所有页都加载到buff pool中。

+ 系统表空间（ibdata*文件）  
  + Change Buffer
  + Undo log

+ Change Buffer  
  + 对非唯一索引进行insert、update、delete操作时，如果对应的数据不在buffer pool中时，不会立即从磁盘中加载数据到buffer pool中，而是将对应页的操作记录保存在change buffer。之后如果对应的页被加载到buffer pool时会进行合并操作。如果对应页一直未被加载到buffer pool，change buffer的数据在MySQL服务空闲时或者MySQL进程关闭时进行合并操作。
  + 配置字段：
    + `innodb_change_buffering`：默认值`none`，不会缓存任何操作。  
      `none`:默认值，不会缓存任何操作。  
      `inserts`:缓存插入操作。  
      `deletes`:缓存删除标记操作。  
      `changes`:缓存插入和删除标记操作。  
      `purges`:缓存真正的删除操作。  
      `all`:缓存所有变更操作。
    + `innodb_change_buffer_max_size`：Change Buffer所占BUffer Pool最大百分比。默认值25，最大值50。
  + 我觉得弃用的根本原因是SSD对于是随机IO还是顺序IO性能是一样的。

+ Adaptive Hash Index
  + 如果某一页多次访问，被当成是热点数据，那么AHI会主动建立哈希项。热点数据的多种访问路径就会建立多个哈希项，也就是说当作热点数据后，通过主键访问那么会建立主键所对应的哈希项，再通过二级索引访问就会建立二级索引对应的哈希项。
  + 配置字段：  
    + `innodb_adaptive_hash_index`：是否开启，默认关闭。
    + `innodb_adaptive_hash_index_parts`：分成多少区，减少锁竞争，提升并发能力。

+ Doublewrite Buffer
  + 假设有一个脏页准备从Buffer Pool刷新到磁盘，首先会将脏页从Buffer Pool复制到Doublewrite Buffer，然后InnoDB再从Doublewrite Buffer中把这些页写入到对应表空间的物理页位置。然后再将
  + `innodb_doublewrite`：是否开启Doublewrite Buffer。  
      `ON`：默认值开启  
      `OFF`：关闭  
      `DETECT_AND_RECOVER`：`DETECT_AND_RECOVER is the same as ON.`与`ON`相同  
      `DETECT_ONLY`：`With DETECT_ONLY, only metadata is written to the doublewrite buffer.`只保留元数据，InnoDB每个数据页开头都有一些结构化字段，比如：校验和、页编号等等。元数据应该指的就是这些数据，也就是说该选项只判断数据是否完成，不做数据恢复。
  + `innodb_doublewrite_dir`:doublewrite buffer磁盘文件路径。
  + `innodb_doublewrite_files`:每个innodb buffer pool实例的doublewrite buffer的磁盘文件数量。默认值是2。
  + `innodb_doublewrite_pages`:`Defines the maximum number of doublewrite pages per thread for a batch write.`每个线程批量写入的最大页数。默认值128。那么一次写入的大小为128*16K=2M。