/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_anet.h 
 * 
 * Description :  Unc_anet, Basic TCP socket stuff. Derived from redis.
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/
#ifndef __UNC_ANET_H_INCLUDED__
#define __UNC_ANET_H_INCLUDED__
 
/*
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
      
#define UNC_OK      0
#define UNC_ERR     -1
*/

#define UNC_ANET_ERR_LEN    256
#define UNC_ANET_BUF_SIZE   256*1024 /* 256K */

#define UNC_ANET_CONNECT            0
#define UNC_ANET_CONNECT_NONBLOCK   1

int unc_anet_tcp_connect(char *err, char *addr, int port);
int unc_anet_tcp_nonblock_connect(char *err, char *addr, int port);
int unc_anet_read(int fd, char *buf, int count);
int unc_anet_resolve(char *err, char *host, char *ipbuf);
int unc_anet_tcp_server(char *err, char *bindaddr, int port);
int unc_anet_tcp_accept(char *err, int serversock, char *ip, int *port);
int unc_anet_write(int fd, char *buf, int count);
int unc_anet_nonblock(char *err, int fd);
int unc_anet_tcp_nodelay(char *err, int fd);
int unc_anet_tcp_keepalive(char *err, int fd);
int unc_anet_peer_tostring(char *err, int fd, char *ip, int *port);
int unc_anet_set_send_buffer(char *err, int fd, int buffsize);

//ÖÐÎÄ
#endif /* __UNC_ANET_H_INCLUDED__ */

