近期项目引入了skynet框架，为此我上周专门研读了其源码。虽然skynet在业界名声很大，但我个人对开源服务器框架一直持相对保守的态度——我认为底层框架最好能够自主实现，这样才能准确把控系统的瓶颈所在，并根据实际业务需求，在复杂度和性能之间找到最佳的平衡点。  

在阅读源码后，我还是没有太大的兴趣做非常细的记录，仅就当前的理解做一些关键点的梳理，后续如有更深层的认识再做补充。  

skynet核心包含五种专用线程：main thread、timer thread、monitor thread、socket thread和work thread。其中：
+ timer thread负责更新服务器时间并处理定时任务，到期时向对应的skynet context消息队列中投递消息。
+ socket thread处理所有网络IO，其自定义的socket结构体中记录了接收到的数据应归属的skynet context，进而将数据传入对应消息队列。
+ work thread负责从各个skynet context的消息队列中取出消息并执行相应的业务逻辑。

skynet启动流程是，skynet首先以snlua为模板创建名为bootstrap的skynet context，并由主线程向其消息队列压入初始化消息。bootstrap启动后，会继续以snlua为模板创建名为launch的skynet context，并向其发送加载业务主逻辑（如main.lua）的指令。工作线程处理该指令时，会以snlua为模板创建名为main的skynet context，实际业务逻辑的初始化就在main skynet context的start函数中完成——例如启动监听socket或接收连接。此后，网络线程负责处理网络IO，将数据封装成消息投递至main skynet context的消息队列，最终由工作线程执行具体业务处理。  

skynet在github上的readme中，第一句话是“Skynet is a multi-user Lua framework supporting the actor model, often used in games.”。我觉得这样描述可能有点片面了，因为单单从上面我所描述的来看，这是一个非常典型的reactor模型，socket thread处理网络IO，接收消息后创建message放入对应的skynet context的message queue中，work thread处理各个skynet context的message，实现了网络IO与业务逻辑的彻底解耦。这种设计既避免了网络IO阻塞业务处理，又允许通过配置多个工作线程充分利用多核cpu性能。但是对于各个skynet context的关系来说是actor模型，当skynet context之间要互相调用时，是构建一条message，然后放入对应skynet context的message queue中，从这个角度看，skynet是典型的actor模型。“底层是reactor模型，而业务逻辑是actor模型。”，我觉得这样描述可能更确切。

然而，我在肯定其架构优势的同时，也发现了两点值得关注的性能隐患：
+ skynet未提供设置cpu亲和性的接口，当线程在不同cpu核心之间切换时，会导致缓存命中率下降，增加不必要的性能开销。
+ 在网络线程中申请的内存缓冲区，其地址会被包裹在消息结构中，由工作线程进行处理。在numa架构下，若两个线程分属不同的numa节点，将引发跨节点内存访问，导致显著的性能下降。