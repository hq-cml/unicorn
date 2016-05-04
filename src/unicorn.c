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

int main(int argc, char **argv) 
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    
    g_conf.num_clients = 50;
    g_conf.requests = 10000;
    g_conf.live_clients = 0;
    g_conf.keep_alive = 1;
    g_conf.loop = 0;
    g_conf.quiet = 0;
    g_conf.el = unc_ae_create_event_loop();
    unc_ae_create_time_event(g_conf.el, 3000, show_throughput, NULL, NULL);
    g_conf.clients = unc_dlist_init();
    g_conf.hostip = "127.0.0.1";
    g_conf.hostport = 9527;
    
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
