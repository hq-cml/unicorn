/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_ae.h
 * 
 * Description :  A simple event-driven programming library. It's from Redis.
 *                The default multiplexing layer is select. If the system 
 *                support epoll, you can define the epoll macro.
 *
 *                #define UNC_EPOLL_MODE
 *
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/
#ifndef __UNC_AE_H_INCLUDED__
#define __UNC_AE_H_INCLUDED__

/*
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
       
#define UNC_OK 0
#define UNC_ERR -1

#define UNC_EPOLL_MODE
//#define UNC_SELECT_MODE
*/

#ifdef UNC_EPOLL_MODE
#define UNC_AE_SETSIZE     (1024 * 100)    /* Max number of fd supported 因为所有线程都是同一个进程，所以此数值决定了mossad的最大连接数量 */
#else
#define UNC_AE_SETSIZE     1024            /* select 最多监听1024个fd */
#endif

#define UNC_AE_NONE         0
#define UNC_AE_READABLE     1
#define UNC_AE_WRITABLE     2

#define UNC_AE_FILE_EVENTS  1
#define UNC_AE_TIME_EVENTS  2
#define UNC_AE_ALL_EVENTS   (UNC_AE_FILE_EVENTS | UNC_AE_TIME_EVENTS)
#define UNC_AE_DONT_WAIT    4

#define UNC_AE_NOMORE       -1

/* Macros，不使用，啥也不干 */
#define UNC_AE_NOTUSED(V)   ((void)V)

struct unc_ae_event_loop;

/* Type and data structures */
typedef void unc_ae_file_proc(struct unc_ae_event_loop *el, int fd, 
        void *client_data, int mask);
typedef int unc_ae_time_proc(struct unc_ae_event_loop *el, long long id,
        void *client_data);
typedef void unc_ae_event_finalizer_proc(struct unc_ae_event_loop *el,
        void *client_data);
typedef void unc_ae_before_sleep_proc(struct unc_ae_event_loop *el);

/* File event structure */
typedef struct unc_ae_file_event {
    int                  mask; /* one of AE_(READABLE|WRITABLE) */
    unc_ae_file_proc    *r_file_proc;
    unc_ae_file_proc    *w_file_proc;
    /* Ae bug， 读写函数用同一个参数，会出现覆盖 */
    //void                 *client_data;
    void                *read_client_data;
    void                *write_client_data;
    
} unc_ae_file_event;

/* Time event structure */
typedef struct unc_ae_time_event {
    long long id;                                /* 时间事件注册id */
    long when_sec;                               /* 事件发生的绝对时间的秒数 */
    long when_ms;                                /* 事件发生的绝对时间的毫秒数 */
    unc_ae_time_proc    *time_proc;              /* 超时回调函数 */
    unc_ae_event_finalizer_proc *finalizer_proc; /* 时间事件的析构回调 */
    void *client_data;                           /* 超时回调函数/析构函数的参数 */
    struct unc_ae_time_event *next;
} unc_ae_time_event;

/* A fired event */
typedef struct unc_ae_fired_event {
    int fd;
    int mask;
} unc_ae_fired_event;

#ifdef UNC_EPOLL_MODE

typedef struct unc_ae_api_state 
{
    int epfd;
    struct epoll_event events[UNC_AE_SETSIZE];
} unc_ae_api_state;
#else

/* select最大fd只支持1024! */
typedef struct unc_ae_api_state 
{
    fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to
       reuse FD sets after select(). */
    fd_set _rfds, _wfds;
} unc_ae_api_state;
#endif

/* State of an event base program */
typedef struct unc_ae_event_loop {
    int maxfd;                                  /* highest file descriptor currently registered */
    long long           time_event_next_id;     /* 当有新的时间事件注册时，应该赋予它的注册id */
    time_t              last_time;              /* Used to detect system clock skew */
    unc_ae_file_event   events[UNC_AE_SETSIZE]; /* 文件事件数组，fd作为索引，它和api_data是对应的 */
    unc_ae_fired_event  fired[UNC_AE_SETSIZE];  /* Fired events，这个不是用fd作为索引的，从0开始 */
    unc_ae_time_event   *time_event_head;
    int stop;
    unc_ae_api_state    *api_data;              /* fd为索引，与events一一对应，用于selec/epoll函数轮询 */
    unc_ae_before_sleep_proc    *before_sleep;
} unc_ae_event_loop;

/* Prototypes */
unc_ae_event_loop   *unc_ae_create_event_loop(void);
void unc_ae_free_event_loop(unc_ae_event_loop *el);
void unc_ae_stop(unc_ae_event_loop *el);
int unc_ae_create_file_event(unc_ae_event_loop *el, int fd, int mask,
        unc_ae_file_proc *proc, void *client_data);
void unc_ae_delete_file_event(unc_ae_event_loop *el, int fd, int mask);
int unc_ae_get_file_event(unc_ae_event_loop *el, int fd);
long long unc_ae_create_time_event(unc_ae_event_loop *el, long long milliseconds,
        unc_ae_time_proc *proc, void *client_data,
        unc_ae_event_finalizer_proc *finalizer_proc);
int unc_ae_delete_time_event(unc_ae_event_loop *el, long long id);
int unc_ae_process_events(unc_ae_event_loop *el, int flags);
int unc_ae_wait(int fd, int mask, long long milliseconds);
void unc_ae_main_loop(unc_ae_event_loop *el);
char *unc_ae_get_api_name(void);
void unc_ae_set_before_sleep_proc(unc_ae_event_loop *el, 
        unc_ae_before_sleep_proc *before_sleep);
 
#endif /*__UNC_AE_H_INCLUDED__ */

