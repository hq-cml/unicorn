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
#define HTTP_HEADER_CLOSE       1 //0001: header中找到了Connection:close
#define HTTP_HEADER_CHUNKED     2 //0010: header中找到了Transfer-Encoding: chunked
#define HTTP_HEADER_LENGTH      4 //0100: header中找到了Content-Length:
#define HTTP_HEADER_ERR         8 //0100: 未知错误

#endif


