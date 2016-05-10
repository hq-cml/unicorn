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

/* ---------------�������������� ---------------*/
typedef struct response {
    int          is_get;    /* �Ƿ�õ��������ķ��� */
    unc_str_t   *res_body;       /* ����ķ������� */
}response_t;

/* ----------------ȫ�־��-------------------- */
typedef struct config {
    char                *title;             /* �������� */
    unc_ae_event_loop   *el;                /* ae��� */
    char                *hostip;            /* ����Ŀ��IP */
    int                  hostport;          /* ����Ŀ��˿� */
    int                  num_clients;       /* Ԥ��ͬһʱ��ͻ�����(������) */
    int                  live_clients;      /* ʵ��Ŀǰ��Ծ�Ŀͻ���(ʵʱ������) */
    int                  requests;          /* �����������������������ʱָ�� */
    int                  requests_issued;   /* �Ѿ�����ȥ�������� */
    int                  requests_finished; /* ʵ����ɵ��������� */
    int                  quiet;             /* �Ƿ�ֻ��ʾqps��Ĭ�Ϸ� */
    int                  keep_alive;        /* �Ƿ�ά�ֳ����ӣ�1 = keep alive, 0 = reconnect (default 1) */
    int                  loop;              /* �����Ƿ�����ֹѭ��:�� */
    long long            start;             /* ����ʼʱ�� */
    long long            total_latency;     /* �����ܺ�ʱ(����) */
    unc_dlist_t         *clients;           /* client���� */
    char                *so_file;           /* ���ļ� */
    int                  done_when_close;   /* �������˹ر����ӣ��Ƿ�����һ����������(default 1) */
    unc_str_t           *request_body;      /* �������� */
    response_t           response;          /* �������������� */
    
} conf_t;

/* ---------------Client�ṹ----------------- */ 
typedef struct client_st {
    int             fd;         /* client��fd */
    unc_str_t      *sendbuf;    /* client��sendbuf */
    unc_str_t      *recvbuf;    /* client��recvbuf */
    unsigned int    written;    /* bytes of 'obuf' already written */
    unsigned int    read;       /* bytes already be read */
    long long       start;      /* start time of request */
    long long       latency;    /* request latency */
} client_t;

/* --------------��̬���������-----------------*/
typedef struct unc_so_func_struct 
{
    int (*handle_init)(void *, void *);                 /* �����̳�ʼ����ʱ�������ã�������ȫ�ֱ���g_conf���˺�����ѡ */ 
    int (*handle_finish)(void *, void *);               /* �����̽�����ʱ�������ã��˺�����ѡ */
    int (*request_pre)(void *, void *);                 /* ����ʼǰ���ã��˺�����ѡ */
    int (*request_post)(void *, void *);                /* �����������ã��˺�����ѡ */
    int (*generate_request)(void *, void *);            /* ����request���� */
    int (*check_full_response)(void *, void *, void *); /* �ж�������response����ѡ���� */
} unc_so_func_t;

#endif

