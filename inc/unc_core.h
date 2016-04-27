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
#include <dlfcn.h>
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

/*
#include "unc_so.h"
#include "unc_daemon.h"
#include "unc_thread.h"
#include "unc_master.h"
#include "unc_plugin.h"
#include "unc_signal.h"
*/

/* -----------------PUBLIC MACRO---------------- */
#define UNC_OK       0
#define UNC_ERR     -1
#define UNC_FAILED  -2
#define UNC_NONEED  -3
#define UNC_END     -4


#define UNC_PROG_TITLE   "Unicorn"
#define UNC_PROG_NAME    "unicorn"
#define UNICORN_VERSION   "1.0"

#endif

