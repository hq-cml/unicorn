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

//全局句柄 
typedef struct config {
    unc_ae_event_loop   *el;                /* ae句柄 */
    char                *hostip;            /* 测试目标IP */
    int                  hostport;          /* 测试目标端口 */
    int                  num_clients;       /* 预计同一时间客户端数 */
    int                  live_clients;      /* 实际目前活跃的客户数 */
    int                  requests;          /* 期望总请求个数，程序启动时指定 */
    int                  requests_issued;   /* 已经发出去请求总数 */
    int                  requests_finished; /* 实际完成的请求总数 */
    int                  quiet;             /* 是否只显示qps，默认否 */
    int                  keep_alive;        /* 1 = keep alive, 0 = reconnect (default 1) */
    int                  loop;              /* 程序是否无终止循环:否 */
    long long            start;             /* 程序开始时间 */
    long long            total_latency;     /* 程序总耗时(毫秒) */
    char                *title;             /* 程序名称 */
    unc_dlist_t         *clients;           /* client链表 */
} conf_t;

//每个Client结构
typedef struct client_st {
    int             fd;         /* client的fd */
    unc_str_t      *obuf;       /* client的sendbuf */
    unsigned int    written;    /* bytes of 'obuf' already written */
    unsigned int    read;       /* bytes already be read */
    long long       start;      /* start time of request */
    long long       latency;    /* request latency */
} client_t;

static conf_t g_conf;

static void client_done(client_t *c);

/* 获得当前微妙数 */
static long long ustime() 
{
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long)tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

/* 获得当前毫秒数 */
static long long mstime() 
{
    struct timeval tv;
    long long mst;

    gettimeofday(&tv, NULL);
    mst = ((long)tv.tv_sec) * 1000;
    mst += tv.tv_usec / 1000;
    return mst;
}

/* 打印最终测试报告 */
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

/* 时间事件: 打印当前的QPS(从开始到当前的累计值) */
static int show_qps(unc_ae_event_loop *el, long long id, void *priv) 
{
    float dt = (float)(mstime() - g_conf.start) / 1000.0;
    float rps = (float)g_conf.requests_finished / dt;
    printf("%s: %.2f\n", g_conf.title, rps);
    return 3000; /* every 3000ms */
}

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
