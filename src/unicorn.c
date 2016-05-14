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
 **/

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include "unc_ae.h"
#include "unc_dlist.h"
#include "unc_string.h"
#include "unc_anet.h"
*/
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
    g_conf.requests_issued   = 0;
    g_conf.requests_done     = 0;
    g_conf.requests_finished = 0;
	g_conf.num_clients = 1;
	g_conf.requests = 1;
	g_conf.live_clients = 0;
	g_conf.keep_alive = 1;
	g_conf.loop = 0;
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

    while ((c = getopt(argc, argv, "h:p:c:s:f:n:k:w:qlH")) != -1) 
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
    puts(" -q                 quiet. Just show QPS values");
    puts(" -l                 loop. Run the tests forever. For persistent test");
    puts(" -w                 whether define done if server close connection.(default 1)");
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
    g_conf.requests_issued = 0;
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
 * 一直写(通过Ae)，直到将该写的(client->obuf)全部写完了，才启动read回调
 */
static void write_handler(unc_ae_event_loop *el, int fd, void *priv, int mask) 
{
    client_t *c = (client_t *)priv;

    /* Initialize request when nothing was written. */
    if (c->written == 0) 
    {   
        //if (g_conf.requests_issued++ >= g_conf.requests) 
        if (g_conf.requests_issued >= g_conf.requests) 
        {
            /*
               * 这个地方原来存在两个bug: 
               * 1. requests_issued++的位置应该下面，否则，由于write是可能被多次调用的，导致requests_issued远远超过requests
               * 2. 程序结束的判断是通过requests_issued进行，这导致程序过早结束，因为其他client可能正在读，服务端日志查看发现
               *    client都提前close了连接，问题就出在此处，应该放在read里面判断是否结束程序
               * 3. 一个可能的潜在问题，在read里面判断完成请求数，会不会因为某次请求异常未完成而导致整个程序达不到退出条件呢?
               *    经测试: 会!
               */
            //free_client(c);
            //unc_ae_stop(g_conf.el);
            return;
        }

        c->start = ustime();
        c->latency = -1;
    }

    if (c->sendbuf->len > c->written) 
    {
        char *ptr = c->sendbuf->buf + c->written;
        int nwritten = write(c->fd, ptr, c->sendbuf->len - c->written);
        if (nwritten == -1) 
        {
            /* When a process writes to a socket that has received an RST, the SIGPIPE signal is sent to the process. */
            if (errno != EPIPE) 
            {
                fprintf(stderr, "write failed:%s\n", strerror(errno));
            }
            free_client(c);
            return;
        }
        c->written += nwritten;

        if (c->sendbuf->len == c->written) 
        {
            g_conf.requests_issued++;
            /* 删除写事件 */
            unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
            /* 启动读事件 */
            unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_READABLE, read_handler, c);
        }
    }

}

/* 
 * 读取回调函数 
 */
static void read_handler(unc_ae_event_loop *el, int fd, void *priv, int mask) 
{
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
        //server端关闭连接
        if (g_so.handle_server_close) g_so.handle_server_close(&g_conf, c, NULL);
        client_done(c, 1);//广义的完成请求
        return;
    }
    buffer[nread] = '\0';
    c->read += nread;
    unc_str_cat(&(c->recvbuf), buffer); //append
    
    //当读取到的数据，和写出去的数据相等时，算是完成了一次请求
    //if (c->read == c->sendbuf->len && 0==memcmp(c->recvbuf->buf, c->sendbuf->buf, c->sendbuf->len)) 
    check = g_so.check_full_response(&g_conf, c, NULL);
    if(check == UNC_OK)
    {
        client_done(c, 0);
    }
    else if(check == UNC_NEEDMORE)
    {
        //fprintf(stdout, "Part response:%s\n", c->recvbuf->buf);
        return;
    }
    else
    {
       fprintf(stderr, "Something wrong in server!\n"); 
       exit(1);
    }
}

/* 
 * 重置client,然后开启新一轮写/读流程 
 */
static void reset_client(client_t *c) 
{
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_READABLE);
    unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE, write_handler, c);
    
    unc_str_clear(c->recvbuf);
    c->written = 0;
    c->read = 0;
}

/* 
 * 当client完成了一次写/读请求之后调用 
 * 参数:
 *    1. client
 *    2. server_close，server端是否关闭了连接
 *
 */
static void client_done(client_t *c, int server_close) 
{
	int num;

    //服务端没有关闭连接，或者服务端关闭连接但是done_if_srv_close是1，则完成数++，并且记录服务端返回
    if(server_close != 1 
       || (server_close == 1 && g_conf.done_if_srv_close))
    {
        c->latency = ustime() - c->start;
        ++g_conf.requests_done;
        if(!g_conf.response.is_get)
        {
            g_conf.response.is_get = 1;
            g_conf.response.res_body = unc_str_dup(c->recvbuf);
        }
    }

    ++g_conf.requests_finished;   
    //如果达到总预计请求数，则程序停止，用广义的requests_done保证程序能够结束
    if (g_conf.requests_finished == g_conf.requests) 
    {
        //free_client(c);
        unc_ae_stop(g_conf.el); //全局Ae直接停止
        return;
    }

    // 如果keep_alive且server端没有关闭连接，则重新开始client的写/读流程
    // 否则，释放client，然后重启client,然后开始写/读流程
    if (g_conf.keep_alive && !server_close) 
    {
        reset_client(c);
    } 
    else 
    {
        //先释放当前client，内部live_clients会自减
        free_client(c); 

        //补齐
        num = g_conf.num_clients - g_conf.live_clients;
        if(num > 0)
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
        fprintf(stdout, "*   All requests sended    : %-10d               *\n", g_conf.requests_issued);        
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
    signal(SIGPIPE, SIG_IGN);

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
