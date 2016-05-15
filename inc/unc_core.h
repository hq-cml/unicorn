/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *     Filename:  Unc_core.h 
 * 
 *  Description:  The unicorn's public header file. 
 * 
 *      Version:  0.0.1 
 * 
 *       Author:  HQ 
 *
 **/

#ifndef __UNC_CORE_H_INCLUDED__ 
#define __UNC_CORE_H_INCLUDED__

/* --------SYSTEM HEADER FILE---------- */
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <dlfcn.h>


/*
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <strings.h>
#include <libgen.h>
#include <dirent.h>
#include <fnmatch.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <getopt.h>
#include <sys/prctl.h>
#include <sys/resource.h> 
#include <netinet/in.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/cdefs.h>
#include <execinfo.h>

*/
 
/* -------------------CONFIG------------------- */
/* Multiplexing I/O mode */
#ifdef __UNC_EPOLL_MODE__
    #define UNC_EPOLL_MODE              //epoll  
#elif defined __UNC_SELECT_MODE__           
    #define UNC_SELECT_MODE             //select
#endif


/* ---------------PROJECT HEADER FILE----------- */
/*
#include "unc_lock.h"
#include "unc_log.h"
#include "unc_hash.h"
#include "unc_conf.h"
*/

#include "unc_ae.h"
#include "unc_vector.h"
#include "unc_string.h"
#include "unc_dlist.h"
#include "unc_anet.h"
#include "unc_so.h"
#include "unc_plugin.h"

/*
#include "unc_daemon.h"
#include "unc_thread.h"
#include "unc_master.h"
#include "unc_signal.h"
*/

/* -----------------PUBLIC MACRO---------------- */
#define UNC_OK        0
#define UNC_ERR      -1
#define UNC_FAILED   -2
#define UNC_NONEED   -3
#define UNC_NEEDMORE -4


#define UNC_PROG_TITLE   "Unicorn"
#define UNC_PROG_NAME    "unicorn"
#define UNICORN_VERSION   "1.0"

#define UNC_IOBUF_SIZE 40960

#define SERVER_NOT_CLOSE        0 //服务器没有关闭连接
#define SERVER_CLOSE_WHEN_READ  1 //read的时候发现服务器断开了连接
#define SERVER_CLOSE_WHEN_WRITE 2 //write的时候发现服务器断开了连接( EPIPE )


/* ---------------服务器返回内容 ---------------*/
typedef struct response {
    int          is_get;    /* 是否得到了完整的返回 */
    unc_str_t   *res_body;       /* 具体的返回内容 */
}response_t;

/* ----------------全局句柄-------------------- */
typedef struct config {
    char                *title;             /* 程序名称 */
    unc_ae_event_loop   *el;                /* ae句柄 */
    char                *hostip;            /* 测试目标IP */
    int                  hostport;          /* 测试目标端口 */
    int                  num_clients;       /* 预计同一时间客户端数(并发数) */
    int                  live_clients;      /* 实际目前活跃的客户数(实时并发数) */
    int                  requests;          /* 期望总请求个数，程序启动时指定 */
    int                  requests_sended;   /* 已经发出去请求总数 */
    int                  requests_done;     /* 实际完成的请求总数 */
    int                  requests_finished; /* 广义的完成请求数，比如服务端断开连接等异常也算完成 */
    int                  quiet;             /* 是否只显示qps，默认否 */
    int                  keep_alive;        /* 是否维持长连接，1 = keep alive, 0 = reconnect (default 1) */
    int                  loop;              /* 程序是否无终止循环:否 */
    int                  epipe;             /* 程序是否尽量避免EPIPE，这种办法不完全靠谱，默认: 否 */
    long long            start;             /* 程序开始时间 */
    long long            total_latency;     /* 程序总耗时(毫秒) */
    unc_dlist_t         *clients;           /* client链表 */
    char                *so_file;           /* 库文件 */
    char                *request_file;      /* 请求body内容文件 */
    int                  done_if_srv_close; /* 如果服务端关闭连接，是否算是一个完整请求(default 1) */
    int                  debug;             /* debug 开关(default 0) */
    unc_str_t           *request_body;      /* 请求内容 */
    response_t           response;          /* 服务器返回内容 */
    
} conf_t;

/* ---------------Client结构----------------- */ 
typedef struct client_st {
    int             fd;         /* client的fd */
    unc_str_t      *sendbuf;    /* sendbuf */
    unc_str_t      *recvbuf;    /* recvbuf */
    unsigned int    written;    /* 已发送字节数 */
    unsigned int    read;       /* 一接受字节数 */
    long long       start;      /* 请求开始时间点 */
    long long       latency;    /* 请求好事 */
} client_t;

/* --------------动态库容器句柄-----------------*/
typedef struct unc_so_func_struct 
{
    int (*handle_init)(void *, void *);                 /* 当进程初始化的时候均会调用，参数是全局变量g_conf，此函数可选 */ 
    int (*handle_finish)(void *, void *);               /* 当进程结束的时候均会调用，此函数可选 */
    int (*request_pre)(void *, void *);                 /* 请求开始前调用，此函数可选 */
    int (*request_post)(void *, void *);                /* 请求结束后调用，此函数可选 */
    int (*generate_request)(void *, void *);            /* 生成request请求 */
    int (*check_full_response)(void *, void *, void *); /* 判断完整的response，必选函数 */
    int (*handle_server_close)(void *, void *, void *); /* 服务端关闭连接的时候调用，可选函数 */
} unc_so_func_t;

#endif

