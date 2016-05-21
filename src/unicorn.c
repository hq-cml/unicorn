/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *     Filename:  unicorn.c 
 * 
 *  Description:  框架主逻辑。 
 * 
 *      Version:  1.0.0 
 * 
 *       Author:  HQ 
 *
 * 一个潜在的坑:
 *   如果server是长连接服务，那么没有任何问题。但是，如果server是短连接服务，即:
 *   server返回的内容满足让check_full_response函数返回ok, 并且随后立刻主动释放了连接。
 *   在这种情况下，unicorn的统计将失效，并导致发送总请求数变少。

 *   问题的原因是TCP的EPIPE机制 ：
 *   对于对方已经close的fd，第一次调用write会导致收到RST包，但却是可以成功的。要调用第二次write才能感知到EPIPE错误！！！！
 * 
 *   server关闭连接的时候，此时unicorn没有read(返回内容满足check_full_response，则unicorn转向继续写)，无法感知连接关闭，
 *   这个时候调用了reset_client,重新开启了write_handler函数，则write会认为能够成功(除非凑巧要触发多次write)，
 *   此时unicorn认为发送了一次完整的request(其实没有，因为服务端已关闭连接，是收不到也不会去收request的)。然后uncorn
 *   启动read_handler才能感知到服务端close，此时request_sended仍然自增一次。最终导致服务端收到的总请求数减半！！
 *   
 *   一个tricky解决办法，write_handler函数，将sendbuf拆成两块，分为两次write，可以感知到EPIPE错误，此时可以
 *   不自增sended数量，并且free_client。但是，这种方法不是特别靠谱:
 *   1. 在长连接的场景的时候，如果用这种方案会极大降低unicorn的效率
 *   2. 如果request的数据只有一个字节，则无法拆分。
 *   3. 这个方案在Mossad上测试，没有问题，但是在Apache上测试，没法解决问题！！Tcpdump抓包后发现，当请求结束后,
 *      Apache和Mossad都会发送F包表示关闭连接，此时write数据，Mossad可以返回RST包，进而触发unicorn在第二次write的时候
 *      感知EPIPE错误，但是为什么Apache不返回RST包，unicorn还是无法感知EPIPE，所以这种方案对于Apache就失效了。个人猜测
 *      Apache可能是用shutdown(SHUT_WR)而不是close来处理断开连接。
 *   
 *   暂时没想到特别完善的兼容办法，好在一般的server都不会随便主动close，诸如Apache这类可能会主动close的server
 *   也会用Connection:close头来告知client。当检测到Connection:close，则:
 *   1.应该以server关闭连接作为完整返回的标志。
 *   2.自己主动关闭连接
 **/

#include "unc_core.h"

//函数声明
static void client_done(client_t *c, int server_close);
static int show_qps(unc_ae_event_loop *el, long long id, void *priv);
static void usage(int status);
static void write_handler(unc_ae_event_loop *el, int fd, void *priv, int mask);
static void create_multi_clients(int num);
static void read_handler(unc_ae_event_loop *el, int fd, void *priv, int mask);
static void free_all_clients();
static void show_final_report(void);
static void free_client(client_t *c);

//全局句柄定义
static conf_t g_conf;

//动态库句柄定义
unc_so_func_t    g_so;
void *g_handle;

/* 利用syms对g_so各个函数的遍历初始化 */
static unc_so_symbol_t syms[] = 
{
    /* symbol_name,                      function pointer,                            optional */
    {"unc_handle_init",            (void **)&g_so.handle_init,              1}, /* 可选 */
    {"unc_handle_finish",          (void **)&g_so.handle_finish,            1}, /* 可选 */
    {"unc_request_pre",            (void **)&g_so.request_pre,              1}, /* 可选 */
    {"unc_request_post",           (void **)&g_so.request_post,             1}, /* 可选 */
    {"unc_generate_request",       (void **)&g_so.generate_request,         1}, /* 可选 */
    {"unc_check_full_response",    (void **)&g_so.check_full_response,      0}, /* 必选 */
    {"unc_handle_server_close",    (void **)&g_so.handle_server_close,      1}, /* 可选 */
    {NULL, NULL, 0}
};

/* 
 * 获得当前微妙数 
 */
static long long ustime() 
{
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long)tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

/* 
 * 获得当前毫秒数 
 */
static long long mstime() 
{
    struct timeval tv;
    long long mst;

    gettimeofday(&tv, NULL);
    mst = ((long)tv.tv_sec) * 1000;
    mst += tv.tv_usec / 1000;
    return mst;
}

/*
 * 全局句柄默认值初始化
 */
static void init_conf() 
{
    g_conf.title = NULL;
    g_conf.requests_sended   = 0;
    g_conf.requests_done     = 0;
    g_conf.requests_finished = 0;
    g_conf.num_clients = 1;
    g_conf.requests = 1;
    g_conf.live_clients = 0;
    g_conf.keep_alive = 1;
    g_conf.loop = 0;
    g_conf.epipe = 0;
    g_conf.debug = 0;
    g_conf.quiet = 0;
    g_conf.el = unc_ae_create_event_loop();
    unc_ae_create_time_event(g_conf.el, 3000, show_qps, NULL, NULL);
    g_conf.clients = unc_dlist_init();
    g_conf.hostip = "127.0.0.1";
    g_conf.hostport = 9527;
    g_conf.so_file = "./libfunc.so";
    g_conf.request_file = NULL;
    g_conf.response.is_get = 0;
    g_conf.response.res_body = NULL;
    g_conf.request_body = NULL;
    g_conf.done_if_srv_close = 1;
    return;
}

/* 
 * 时间事件: 打印当前的QPS(从开始到当前的累计值) 
 */
static int show_qps(unc_ae_event_loop *el, long long id, void *priv) 
{
    float dt = (float)(mstime() - g_conf.start) / 1000.0;
    float rps = (float)g_conf.requests_done / dt;
    fprintf(stdout, "%s: %.2f r/s\n", g_conf.title, rps);
    return 3000; /* every 3000ms */
}

/*
 * 参数解析
 */
static void parse_options(int argc, char **argv) 
{
    char c;

    while ((c = getopt(argc, argv, "h:p:c:s:f:n:k:w:qlHED")) != -1) 
    {
        switch (c) {
        case 'h':
            g_conf.hostip = strdup(optarg);
            break;
        case 'p':
            g_conf.hostport = atoi(optarg);
            break;
        case 'c':
            g_conf.num_clients = atoi(optarg);
            break;
        case 'n':
            g_conf.requests = atoi(optarg);
            break;
        case 's':
            g_conf.so_file= strdup(optarg);
            break;
        case 'f':
            g_conf.request_file = strdup(optarg);
            break;
        case 'k':
            g_conf.keep_alive = atoi(optarg);
            break;
        case 'w':
            g_conf.done_if_srv_close = atoi(optarg);
            break;           
        case 'q':
            g_conf.quiet = 1;
            break;
        case 'l':
            g_conf.loop = 1;
            break;
        case 'E':
            g_conf.epipe = 1;
            break;
        case 'D':
            g_conf.debug = 1;
            break;
        case 'H':
            usage(0);
            break;
        default:
            usage(1);
        }
    }
}

/*
 * 打印使用帮助函数
 */
static void usage(int status) 
{
    puts("Usage: ./Unicorn [-h <host>] [-p <port>] "
            "[-c <clients>] [-n requests]> [-k <boolean>]\n");
    puts(" -h <hostname>      server hostname (default 127.0.0.1)");
    puts(" -p <port>          server port (default 9527)");
    puts(" -c <clients>       number of parallel connections (default 1)");
    puts(" -n <requests>      total number of requests (default 1)");   
    puts(" -s <so file>       so file (default ./libfunc.so)");
    puts(" -f <request file>  request file(default NULL)");
    puts(" -k <boolean>       1 = keep alive, 0 = reconnect (default 1)");
    puts(" -w <boolean>       whether define done if server close connection.(default 1)");
    puts(" -q                 quiet. Just show QPS values");
    puts(" -l                 loop. Run the tests forever. For persistent test");
    //puts(" -E                 try to handle EPIPE while server short connection.(default null)");//隐藏-E选项，因为不靠谱
    puts(" -D                 print the debug info.(default null)");
    puts(" -H                 show help information\n");
    exit(status);
}

/*
 * 读取request_file文件，生成request_body
 */
static void read_quest_file()
{
    int fd;  
    int bytes_read;  
    char buffer[UNC_IOBUF_SIZE];
    
    if(!g_conf.request_body)
    {
        g_conf.request_body = unc_str_new_empty();
    }
    else
    {
        fprintf(stderr, "Some Thing Wrong!! Request body is not null\n");  
        exit(1);
    }
    
    //打开源文件
    if ((fd = open(g_conf.request_file, O_RDONLY)) == -1) 
    {  
        fprintf(stderr, "FATAL!! Open %s Failed. Error: %s\n", g_conf.request_file, strerror(errno));  
        exit(1);
    }  
  
    //一个经典的读取文件的代码
    memset(buffer, 0, UNC_IOBUF_SIZE);
    while ((bytes_read = read(fd, buffer, UNC_IOBUF_SIZE))) 
    {  
        //发生一个致命的错误
        if ((bytes_read == -1) && (errno != EINTR))
        {
            fprintf(stderr, "FATAL!! Read file %s failed.Error: %s\n", g_conf.request_file, strerror(errno));
            exit(1);
        }
        else if (bytes_read > 0) 
        {  
            unc_str_cat(&(g_conf.request_body), buffer);
        }
        memset(buffer, 0, UNC_IOBUF_SIZE);
    }  
    close(fd);  
    return;    
}

/* 
 * 启动压力测试 
 */
static void start_request(char *title, char *content) 
{
    if(!g_conf.title)
    {
        g_conf.title = title;
    }
    //重置requests_issued和requests_finished
    g_conf.requests_sended = 0;
    g_conf.requests_done   = 0;
    g_conf.requests_finished = 0;

    if(!g_conf.request_body)
    {
        g_conf.request_body = unc_str_new(content);
    }
    
    /* 创建指定数目的client */
    create_multi_clients(g_conf.num_clients);

    g_conf.start = mstime();
    unc_ae_main_loop(g_conf.el);
    g_conf.total_latency = mstime() - g_conf.start;
    free_all_clients();
}

/* 
 * 创建一个client
 * 参数: content,发送内容
 * 返回: client_t指针
 */
static client_t *create_one_client() 
{
    char buf[2048];

	//初始化client_t
    client_t *c = (client_t *)malloc(sizeof(*c));
	//非阻塞方式建立与server的连接
    c->fd = unc_anet_tcp_nonblock_connect(buf, g_conf.hostip, g_conf.hostport);
    if (c->fd == UNC_ERR) 
    {
        fprintf(stderr, "Connect to %s:%d failed.Detail:%s\n", g_conf.hostip, g_conf.hostport, buf);
        exit(1);
    }
    c->sendbuf = unc_str_dup(g_conf.request_body);
    c->recvbuf = unc_str_new_empty();
    c->written = 0;
    c->read = 0;
	
	//注册写事件
    unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE, write_handler, c);
	//自身加入client队列
    unc_dlist_add_node_tail(g_conf.clients, c);
	//client数自增
    ++g_conf.live_clients;
    
	if(g_conf.debug) fprintf(stdout, " [DEBUG] Create a Client(Fd:%d).\n", c->fd);
    return c;
}

/* 
 * 创建多个client 
 * 参数: num,     创建client的个数
 *       content, 发送内容
 */
static void create_multi_clients(int num) 
{
    int n = 0;
	int i = 0;
	
    while (i++ < num)
    {
        create_one_client();
        /* listen backlog is quite limited on most system */
        if (++n > 64) 
        {
            usleep(50000);
            n = 0;
        }
    }
}

/* 
 * 写回调函数(client发送请求)
 */
static void write_handler(unc_ae_event_loop *el, int fd, void *priv, int mask) 
{
    if(g_conf.debug) fprintf(stdout, " [DEBUG] Begin to write(Fd:%d).\n", fd);
    client_t *c = (client_t *)priv;
    char *ptr;
    int nwritten;
    int first;

    /* Initialize request when nothing was written. */
    if (c->written == 0) 
    {   
        //if (g_conf.requests_issued++ >= g_conf.requests) 
        if (g_conf.requests_sended >= g_conf.requests) 
        {
            /*
               * 这个地方原来存在两个bug: 
               * 1. requests_sended++的位置应该下面，否则，由于write是可能被多次调用的，导致requests_sended远远超过requests
               * 2. 原来程序结束的判断是通过requests_sended进行，这导致程序过早结束，因为其他client可能正在读，服务端日志查看发现
               *    client都提前close了连接，问题就出在此处，应该放在read里面判断是否结束程序
               * 3. 一个可能的潜在问题，在read里面判断完成请求数，会不会因为某次请求异常未完成而导致整个程序达不到退出条件呢?
               *    经测试: 会!
               */
            //unc_ae_stop(g_conf.el);
            if(g_conf.debug) fprintf(stdout, " [DEBUG] Enought requests sended(%d). No write\n", g_conf.requests_sended);
            free_client(c);//已发送的请求总数大于，
            return;
        }

        c->start = ustime();
        c->latency = -1;
    }

    if (c->sendbuf->len > c->written) 
    {
        ptr = c->sendbuf->buf + c->written;

        /* 
          * 处理服务端EPIPE的问题，每个请求的第一次write的时候需要检测EPIPE，这个方法并不完全靠谱:
          * 1. 如果发送的数据只有一个字节，则无法拆分。
          * 2. 这个方案在Mossad上测试，没有问题，但是在Apache上测试，没法解决问题！！Tcpdump抓包后发现，当请求结束后,
          *      Apache和Mossad都会发送F包表示关闭连接，此时write数据，Mossad可以正确返回RST包，进而触发unicorn的EPIPE
          *      错误，但是不知道为什么Apache不返回RST包，unicorn无法感知EPIPE，所以这种方案对于Apache就失效了。
          */
        first = 0;
        if(g_conf.epipe && c->written == 0 && c->sendbuf->len > 1)
        {
            
            first = write(fd, ptr, 1); //写一个字节
            if(g_conf.debug) fprintf(stdout, " [DEBUG] Write first byte(Fd:%d):%c, len:%d\n", fd, *ptr, first);
        }
        nwritten = write(fd, ptr+first, c->sendbuf->len - c->written - first);

        if(g_conf.debug) fprintf(stdout, " [DEBUG] Write bytes num(Fd:%d):%d\n", fd, nwritten);
        if (nwritten == -1) 
        {
            //根据TCP机制，此处的EPIPE错误理论上无法触发到，因为需要两次write才能感知，
            //TODO 暂时未找到感知EPIPE的优雅办法，除非将sendbuf拆成两块write两次
            if (errno != EPIPE) 
            {
                fprintf(stderr, "Write failed:%s\n", strerror(errno));
                exit(1);
            }
            fprintf(stderr, "Write failed:%s\n", strerror(errno));
            client_done(c, SERVER_CLOSE_WHEN_WRITE);
            return;
        }
        c->written += (nwritten+first);

        if (c->sendbuf->len == c->written) 
        {
            g_conf.requests_sended++;
            /* 删除写事件 */
            unc_ae_delete_file_event(g_conf.el, fd, UNC_AE_WRITABLE);
            /* 启动读事件 */
            unc_ae_create_file_event(g_conf.el, fd, UNC_AE_READABLE, read_handler, c);
        }
    }

}

/* 
 * 读取回调函数 
 */
static void read_handler(unc_ae_event_loop *el, int fd, void *priv, int mask) 
{
    if(g_conf.debug) fprintf(stdout, " [DEBUG] Begin to read(Fd:%d).\n",fd);
    client_t *c = (client_t *)priv;
    int nread, check = UNC_ERR;
    char buffer[UNC_IOBUF_SIZE];
    memset(buffer, 0, UNC_IOBUF_SIZE);

    //从服务端读取数据，并将读取的内容append到recvbuf上
    nread = read(fd, buffer, UNC_IOBUF_SIZE-1);
    if (nread == -1) 
    {
        if (errno == EAGAIN) 
        {
            /* 暂时不可读，读取nonblock的fd时，可能会遇到 */
            return;
        }
        fprintf(stderr, "Read Error: %s\n", strerror(errno));
        exit(1);
    } 
    else if (nread == 0) 
    {
        if(g_conf.debug) fprintf(stdout, " [DEBUG] Server close conn(Fd:%d). Recv len:%d\n", fd, c->recvbuf->len);

        //server端关闭连接
        if (g_so.handle_server_close) g_so.handle_server_close(&g_conf, c, NULL);
        client_done(c, SERVER_CLOSE_WHEN_READ);//广义的完成请求
        return;
    }
    buffer[nread] = '\0';
    c->read += nread;
    unc_str_cat(&(c->recvbuf), buffer); //append

    if(g_conf.debug) fprintf(stdout, " [DEBUG] Read bytes num(Fd:%d):%d, total: %d\n", fd, nread, c->recvbuf->len);
    
    //判断读取到的内容是否完整
    check = g_so.check_full_response(&g_conf, c, NULL);
    switch(check){
        case UNC_OK:
            client_done(c, SERVER_NOT_CLOSE);
            break;
        case UNC_END:
            client_done(c, SERVER_HINT_CLOSE);
            break;
        case UNC_NEEDMORE:
            //让unicorn框架继续保持read
            //fprintf(stdout, "Part response:%s\n", c->recvbuf->buf);
            break;            
        default:
            fprintf(stderr, "Something wrong in server!\n"); 
            exit(1);
            break;
    }
    return;
}

/* 
 * 重置client,然后开启新一轮写/读流程 
 */
static void reset_client(client_t *c) 
{
    if(g_conf.debug) fprintf(stdout, " [DEBUG] Reset client(Fd:%d).\n\n", c->fd);
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_READABLE);
    unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE, write_handler, c);
    
    unc_str_clear(c->recvbuf); //清空不释放
    c->written = 0;
    c->read = 0;
}

/* 
 * 当client完成了一次写/读请求之后调用 
 * 参数:
 *    @ client
 *    @ server_close，server端是否关闭了连接 : 
 *      SERVER_NOT_CLOSE        -- 服务器没有关闭连接
 *      SERVER_CLOSE_WHEN_READ  -- read的时候发现服务器断开了连接
 *      SERVER_CLOSE_WHEN_WRITE -- write的时候发现服务器断开了连接( EPIPE )
 *      SERVER_HINT_CLOSE       -- server暗示client主动关闭
 */
static void client_done(client_t *c, int server_close) 
{
	int num;

    //服务端没有关闭连接 || 服务端暗示客户端关闭连接 || read服务端关闭连接但是done_if_srv_close是1，则完成数++，并且记录服务端返回
    if(server_close == SERVER_NOT_CLOSE 
       || server_close == SERVER_HINT_CLOSE 
       || (server_close == SERVER_CLOSE_WHEN_READ && g_conf.done_if_srv_close))
    {
        c->latency = ustime() - c->start;
        ++g_conf.requests_done;
        if(!g_conf.response.is_get)
        {
            if(g_conf.debug) fprintf(stdout, " [DEBUG] Fill the g_conf.response only once(Fd:%d). Len:%d\n", c->fd, c->recvbuf->len);
            g_conf.response.is_get = 1;
            g_conf.response.res_body = unc_str_dup(c->recvbuf);
        }
    }
    
    if(g_conf.debug) fprintf(stdout, " [DEBUG] Client(Fd:%d) done(Server_close: %d, %s).\n", c->fd, server_close, 
        server_close==SERVER_NOT_CLOSE? "Not close":(server_close==SERVER_CLOSE_WHEN_READ? 
        "Normal close":(server_close==SERVER_CLOSE_WHEN_WRITE? "Epipe close": "Hint close")));

    //广义完成数自增( 写的时候发现服务端close，不能算服务完成 )
    if(server_close != SERVER_CLOSE_WHEN_WRITE)
    {
        ++g_conf.requests_finished;   
    }
    
    //如果达到总预计请求数，则程序停止，用广义的requests_done保证程序能够结束
    if (g_conf.requests_finished == g_conf.requests) 
    {
        if(g_conf.debug) fprintf(stdout, " [DEBUG] Enough finished request. Begin to end!(Fd:%d)\n", c->fd);
        unc_ae_stop(g_conf.el); //全局Ae直接停止
        return;
    }

    // 如果keep_alive且server端没有关闭连接(也没有暗示客户端关闭)，则重新开始client的写/读流程
    // 否则，释放client，然后重启client,然后开始写/读流程
    if (g_conf.keep_alive && server_close == SERVER_NOT_CLOSE) 
    {
        reset_client(c);
    }
    else 
    {
        //先释放当前client，内部live_clients会自减
        free_client(c); 

        //如果已发送的请求数不够，补充
        num = g_conf.num_clients - g_conf.live_clients;
        if(num > 0 && g_conf.requests_sended < g_conf.requests )
        {
            create_multi_clients(num);
        }
    }
}

/* 
 * 释放一个client 
 */
static void free_client(client_t *c) 
{
    if(g_conf.debug) fprintf(stdout, " [DEBUG] Free client(Fd:%d).\n\n", c->fd);
    unc_dlist_node_t *node;
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_READABLE);
    close(c->fd);
    unc_str_free(c->sendbuf);
    unc_str_free(c->recvbuf);
    --g_conf.live_clients;
    node = unc_dlist_search_key(g_conf.clients, c);
    assert(node != NULL);
    unc_dlist_delete_node(g_conf.clients, node);
    free(c);
}

/* 
 * 释放全部client 
 */
static void free_all_clients() {
    unc_dlist_node_t *node = g_conf.clients->head;
    unc_dlist_node_t *next = NULL;
    while (node) 
    {
        next = node->next;
        free_client((client_t *)node->value);
        node = next;
    }
}

/* 
 * 打印最终测试报告 
 */
static void show_final_report(void) 
{
    float qps;
    char buf[11]; int idx=0, r; memset(buf, ' ', 11);

    qps = (float)g_conf.requests_done / ((float)g_conf.total_latency / 1000);
    r = (int)qps;
    
    if (!g_conf.quiet) 
    {
        fprintf(stdout, "============== %s REPORT ==============\n", g_conf.title);
        fprintf(stdout, "*   All requests           : %-10d               *\n", g_conf.requests);  
        fprintf(stdout, "*   All requests sended    : %-10d               *\n", g_conf.requests_sended);        
        fprintf(stdout, "*   All requests completed : %-10d               *\n", g_conf.requests_done);
        //如果done_if_srv_close，则done和finished相同，所以只有非done_if_srv_close时打印
        if(!g_conf.done_if_srv_close) fprintf(stdout, "*   All requests finished  : %-10d               *\n", g_conf.requests_finished);
        fprintf(stdout, "*   Use time of seconds    : %-10.2f               *\n", (float)g_conf.total_latency/1000);
        fprintf(stdout, "*   Parallel clients       : %-10d               *\n", g_conf.num_clients);
        fprintf(stdout, "*   Keep alive             : %-10d               *\n", g_conf.keep_alive);
        fprintf(stdout, "*                                                     *\n");
        fprintf(stdout, "*   Complete rate          : %-6.2f %%                 *\n", 100*((float)g_conf.requests_done/(float)g_conf.requests));
        while(r>0) { idx++; r /= 10; } idx = 10-idx; buf[idx] = '\0'; //计算最后一行对齐
        fprintf(stdout, "*   Average QPS            : %.2f r/s        %s*\n", qps, buf);
        fprintf(stdout, "============ %s REPORT END ============\n\n", g_conf.title);
    } 
    else 
    {
        fprintf(stdout, "%s:%.2f requests per second\n\n", g_conf.title, qps);
    }
}

int main(int argc, char **argv) 
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);  //忽略SIGPIPE信号，只有在忽略的情况下，两次write关闭的fd，才会得到EPIPE错误

	//全局句柄默认值初始化
	init_conf();

	//参数解析
    parse_options(argc, argv);
    if (optind < argc) 
    {
        usage(1);
    }

    //动态库加载
    if (unc_load_so(&g_handle, syms, g_conf.so_file) < 0) 
    {
        fprintf(stderr, "Load file %s failed\n", g_conf.so_file);
        exit(1);
    }
  
    if (g_so.handle_init) g_so.handle_init(&g_conf, NULL);
    
    if (!g_conf.keep_alive) 
    {
        puts("WARNING:\n"
            " keepalive disabled, at linux, you probably need    \n"
            " 'echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse' and \n"
            " 'sudo sysctl -w net.inet.tcp.msl=1000' for Mac OS X\n"
            " in order to use a lot of clients/requests\n");
    }

    //如果存在request_file，则尝试读取生成request body
    if(g_conf.request_file)
    {
        read_quest_file();
    }
    
    //调用用户钩子，对request body做进一步处理
    if (g_so.generate_request) 
    {
        if(g_so.generate_request(&g_conf, NULL) != UNC_OK)
        {
            fprintf(stderr, "Generate request failed\n");
            exit(1);
        }
    }

    do {
        if (g_so.request_pre) g_so.request_pre(&g_conf, NULL);
        start_request("UNICORN CLIENT", "Hello World!");
        //TODO 每轮测试完毕后的报告
        if (g_so.request_post) g_so.request_post(&g_conf, NULL);
    } while (g_conf.loop);

    if (g_so.handle_finish) g_so.handle_finish(&g_conf, NULL);

    //打印测试报告
    show_final_report();

    //释放动态库
    unc_unload_so(&g_handle);
    
    //TODO 释放g_conf
    return 0;
}
