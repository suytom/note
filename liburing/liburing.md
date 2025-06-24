# 前言
  现在需要实现一个录像服，需求是接收ds发过来的快照信息，并保存到磁盘。当客户端请求某局战斗时，将对应的快照数据文件发送给客户端。这里不写录像服的整体架构，只是讨论单台物理机上如何能实现最大的吞吐量。  

# 前情提要
  块设备，以Nvme SSD为例，该类型块设备拥有多个处理队列，系统初始化时，内核会在block layer为其处理队列创建对应的blk_mq_hw_ctx，每个blk_mq_hw_ctx会与某个CPU所绑定。内核会为该SSD创建一个专属的write back内核线程。对于block layer如何处理request会有多个因素影响，当块设备挂载了IO Scheduler（cat /sys/block/<device>/queue/scheduler）或者block layer层配置了开启合并request的配置（cat /sys/block/<device>/queue/nomerges）时，request不会立马尝试提交到blk_mq_hw_ctx。当上述两种情况都不存在时，block layer会立马尝试将request提交到blk_mq_hw_ctx，如果此时该blk_mq_hw_ctx已满，request会被保存到一个临时队列，在之后的某个时机再尝试提交到blk_mq_hw_ctx。  
  原文：“When the request arrives at the block layer, it will try the shortest path possible: send it directly to the hardware queue. However, there are two cases that it might not do that: if there’s an IO scheduler attached at the layer or if we want to try to merge requests. In both cases, requests will be sent to the software queue.”  
  block layer内核文档：https://www.kernel.org/doc/html/latest/block/blk-mq.html  

# Buffered I/O
## 写文件
  大多数时候write都是将user的内存数据memcpy到page cache就返回了，不会阻塞user thread，之后触发脏页刷盘的时候，由write back内核线程调用block layer的接口创建bio->构建request。之后的步骤则是上面所述。这里可能存在多个误阻塞的场景，第一，这个脏页不一定都是该user thread造成的，因为脏页刷盘的策略面向的是块设备，而不是文件。第二，假设现在有两个线程A、B，A线程write时触发了脏页刷盘，而page cache的脏页需要DMA page cache的数据到块设备后，由块设备驱动在回调中将脏页回收。那么在这一步骤前线程B的write操作也会被阻塞。
## 读文件
  如果缓存在page cache，则memecpy到user memory直接返回。如果cache miss，则调用block layer的接口创建request（此时还是在user thread）。

# Network I/O
  系统初始化时，NIC驱动会通过DMA机制为NIC分配一块主内存区域，并将其映射到设备上，使NIC能够直接读写主内存中的数据。  
  当网络数据包到达时，NIC首先将数据写入其内部缓冲区。随后，NIC将数据直接DMA到之前映射好的主内存。一旦DMA传输完成，NIC驱动将通过中断或轮询机制通知内核协议栈，数据随后会被协议栈解析和处理，最终拷贝到对应 socket的接收缓冲区中，供用户空间读取。  
  当发送网络数据包时，首先从user memory memecpy到socket的send buffer，然后立马被协议栈处理（未开启纳格算法或者数据包大小超过MSS），NIC驱动将该数据包挂入TX ring buffer，配置好DMA映射后，由NIC发起DMA，从主内存读取数据并将其发送到网卡接口上。  
  NIC的RX和TX操作都依赖主内存进行DMA，NIC本身的硬件缓冲主要用于RX临时缓存，TX几乎完全绕过NIC内存，直接从主内存拉数据发送。

# io_uring_setup() flags
+ IORING_SETUP_IOPOLL  
  将块设备以轮询（polling）方式执行该io_uring实例的I/O操作。  
+ IORING_SETUP_SQPOLL  
  创建一个内核线程来轮询SQ，避免每次需要使用enter/sumbit切换到内核。  
+ IORING_SETUP_SQ_AFF  
  用于配合IORING_SETUP_SQPOLL，将内核轮询线程绑定到指定CPU上运行。  
+ IORING_SETUP_ATTACH_WQ  
  允许多个io_uring实例共享同一个io_wq线程池，从而降低内核线程资源消耗，提升资源利用率。  
+ IORING_SETUP_R_DISABLED  
  创建io_uring时，初始状态下SQ是“不可用”的，需要显式调用io_uring_register_ring_fd()才能启用。  
+ IORING_SETUP_SUBMIT_ALL  
  如果在创建io_uring实例时设置了IORING_SETUP_SUBMIT_ALL，那么所有提交操作（io_uring_enter()）必须一次性提交所有SQ，否则失败，不提交任何一个。  
+ IORING_SETUP_COOP_TASKRUN  
  当io_uring的内核线程（io_wq worker）不够用时，用户线程在调用io_uring_enter()提交请求时，可以“主动”协助内核执行部分未完成的I/O任务。
+ IORING_SETUP_TASKRUN_FLAG  
  只在用户线程显式请求时（通过IORING_ENTER_TASKRUN）才在io_uring_enter()中协作执行请求任务，而不是像COOP_TASKRUN那样自动执行。
+ IORING_SETUP_SINGLE_ISSUER  
  所有对io_uring的提交操作（即写 Submission Queue）都由同一个线程完成，内核据此省略多线程同步机制，从而加快SQE提交速度。
+ IORING_SETUP_DEFER_TASKRUN  
  
+ IORING_SETUP_NO_MMAP  
  
+ IORING_SETUP_REGISTERED_FD_ONLY  
  
+ IORING_SETUP_NO_SQARRAY  
  
+ IORING_SETUP_HYBRID_IOPOLL  
  

# io_uring的操作码
+ IORING_OP_READV：从一个fd中读取数据，并填充用户提供的iovec数组（即多个缓冲区）。  
  IORING_OP_WRITEV：将多个buffer（iovecs）中数据写入目标fd。  
  IORING_OP_READ：从一个fd中读取数据。  
  IORING_OP_WRITE：将一个用户态buffer中的数据写入一个fd。  
  上述操作码，对于普通文件描述符，如果不使用O_DIRECT，

+ IORING_OP_READ_FIXED：一种使用固定缓冲区的异步读操作，它跳过页表 pinning，提升性能。  
  IORING_OP_WRITE_FIXED:一种使用固定缓冲区的异步写操作。
  pin/unpin相关函数：  
  io_uring_register_buffers：  
  io_uring_unregister_buffers：  
  
  