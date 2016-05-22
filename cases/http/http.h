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
 * Description :  ����Unicorn��ܵ�HTTPѹ��ͻ���
 *                ʵ��unc_plugin.h�еĺ��������ɽ��û��߼�Ƕ��unicorn���
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
#define HTTP_HEADER_CLOSE       1 //0001: header���ҵ���Connection:close
#define HTTP_HEADER_CHUNKED     2 //0010: header���ҵ���Transfer-Encoding: chunked
#define HTTP_HEADER_LENGTH      4 //0100: header���ҵ���Content-Length:
#define HTTP_HEADER_ERR         8 //0100: δ֪����

int HexStr2Dec(const char *hex, int len);
int handle_chunked(const char *body_start, conf_t *config);

#endif


