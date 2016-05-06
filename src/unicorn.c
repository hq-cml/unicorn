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
static void client_done(client_t *c);

//全局句柄定义
static conf_t g_conf;

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
	
	return;
}

/* 
 * 时间事件: 打印当前的QPS(从开始到当前的累计值) 
 */
static int show_qps(unc_ae_event_loop *el, long long id, void *priv) 
{
    float dt = (float)(mstime() - g_conf.start) / 1000.0;
    float rps = (float)g_conf.requests_finished / dt;
    printf("%s: %.2f\n", g_conf.title, rps);
    return 3000; /* every 3000ms */
}

/*
 * 参数解析
 */
static void parse_options(int argc, char **argv) 
{
    char c;

    while ((c = getopt(argc, argv, "h:p:c:n:k:qlH")) != -1) 
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
        case 'k':
            g_conf.keep_alive = atoi(optarg);
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
    puts("Usage: benchmark [-h <host>] [-p <port>] "
            "[-c <clients>] [-n requests]> [-k <boolean>]\n");
    puts(" -h <hostname>    server hostname (default 127.0.0.1)");
    puts(" -p <port>        server port (default 9527)");
    puts(" -c <clients>     number of parallel connections (default 1)");
    puts(" -n <requests>    total number of requests (default 1)");    
    puts(" -k <boolean>     1 = keep alive, 0 = reconnect (default 1)");
    puts(" -q               quiet. Just show QPS values");
    puts(" -l               loop. Run the tests forever");
    puts(" -H               show help information\n");
    exit(status);
}

/* 
 * 启动压力测试 
 */
static void start(char *title, char *content) 
{
    client_t  *c;
    g_conf.title = title;
    g_conf.requests_issued = 0;
    g_conf.requests_finished = 0;

    /* 创建指定数目的client */
    create_multi_clients(g_conf.num_clients, content);

    g_conf.start = mstime();
    unc_ae_main_loop(g_conf.el);
    g_conf.total_latency = mstime() - g_conf.start;

    show_final_report();
    free_all_clients();
}

/* 
 * 创建一个client
 * 参数: content,发送内容
 * 返回: client_t指针
 */
static client_t *create_one_client(const char *content) 
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
    c->obuf = unc_str_new(content);
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
static void create_multi_clients(int num, char *content) 
{
    int n = 0;
	int i = 0;
	
    while (i++ < num)
    {
        create_one_client((const char *)content);
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
        if (g_conf.requests_issued++ >= g_conf.requests) 
        {
            free_client(c);
            unc_ae_stop(g_conf.el);
            return;
        }

        c->start = ustime();
        c->latency = -1;
    }

    if (c->obuf->len > c->written) 
    {
        char *ptr = c->obuf->buf + c->written;
        int nwritten = write(c->fd, ptr, c->obuf->len - c->written);
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

        if (c->obuf->len == c->written) 
        {
            /* 删除写事件 */
            unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
            /* 启动读事件 */
            unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_READABLE, read_handler, c);
        }
    }

}
 * 重置client,然后开启新一轮写/读流程 
 */
static void reset_client(client_t *c) 
{
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_READABLE);
    unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE, write_handler, c);
    c->written = 0;
    c->read = 0;
}

/* 
 * 当client完成了一次写/读请求之后调用 
 */
static void client_done(client_t *c) 
{
	unc_str_t *pcontent;
	int num;
    // 如果达到总预计请求数，则程序停止
    if (g_conf.requests_finished == g_conf.requests) 
    {
        free_client(c);
        unc_ae_stop(g_conf.el); //全局Ae直接停止
        return;
    }

    // 如果keep_alive，则重新开始client的写/读流程
    // 如果!keep_alive,则重启client,然后开始写/读流程
    if (g_conf.keep_alive) 
    {
        reset_client(c);
    } 
    else 
    {
        pcontent = unc_str_new(c->obuf->buf);
    	//先释放当前client，内部live_clients会自减
		free_client(c); 

		//补齐
		num = g_conf.num_clients - g_conf.live_clients;
		if(num > 0)
		{
			create_multi_clients(num, pcontent->buf);
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
    unc_str_free(c->obuf);
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
    float reqpersec;

    /* 最终每秒处理请求出--QPS */
    reqpersec = (float)g_conf.requests_finished / ((float)g_conf.total_latency / 1000);

    if (!g_conf.quiet) 
    {
        printf("========== %s ==========\n", g_conf.title);
        printf(" All %d requests has send\n", g_conf.requests);        
        printf(" All %d requests completed\n", g_conf.requests_finished);
        printf(" Complete:%.8f%%\n", 100*((float)g_conf.requests_finished/(float)g_conf.requests));
        printf(" Use time %.2f seconds\n", (float)g_conf.total_latency/1000);
        printf(" Parallel %d clients\n", g_conf.num_clients);
        printf(" keep alive: %d\n", g_conf.keep_alive);
        printf("\n");

        printf("%.2f requests per second\n\n", reqpersec);
    } 
    else 
    {
        printf("%s:%.2f requests per second\n\n", g_conf.title, reqpersec);
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

    if (!g_conf.keep_alive) 
    {
        puts("WARNING:\n"
            " keepalive disabled, at linux, you probably need    \n"
            " 'echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse' and \n"
            " 'sudo sysctl -w net.inet.tcp.msl=1000' for Mac OS X\n"
            " in order to use a lot of clients/requests\n");
    }
     return 0;
}
