/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  http.c
 * 
 * Description :  基于Unicorn框架的HTTP压测客户端
 *                实现unc_plugin.h中的函数，即可将用户逻辑嵌入unicorn框架
 * 
 *     Version :  1.0.0 
 * 
 *      Author :  HQ 
 *
 **/
 
#ifndef __UNC_HTTP_H_INCLUDED__ 
#define __UNC_HTTP_H_INCLUDED__

#include "unc_core.h"

/* -----------------MACRO---------------- */
#define HTTP_BODY_CHUNKED      -1
#define HTTP_BODY_CLOSE        -2
#define HTTP_BODY_ERR          -3



static int cal_body_length(char *header_start, int header_length);
static int handle_body(const char *body_start, int body_length);
static int handle_body_content_length(const char *body_start, int body_length);

#endif


