/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_string.h 
 * 
 * Description :  Unc_string, a C dynamic strings library.
 *                Derived from redis.
 *
 *        ע�� :  �����ڲ�����unc_str_expand_room�ĺ��������ص�unc_str_t *����
 *                ��һ���µ�ַ��������Ҫ����ʽ�����£�
 *                g_instance->conf_path = unc_str_cpy(g_instance->conf_path,
 *                                                    "./mossad.conf");
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/

#ifndef __UNC_STRING_H_INCLUDED__ 
#define __UNC_STRING_H_INCLUDED__
 
/*
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
*/
#define UNC_STRING_MAX_PREALLOC (10*1024*1024)

typedef struct _unc_string_s
{
    int  len;    /* ʵ���ַ�������,������'\0' */
    int  free;   /* ���г���,������'\0' */
                 /* ���ϣ�����buf���ܿռ�=len+free+1 */
    char buf[];  /* ���ݿ�ʼָ�� */
}unc_str_t;    /* sizeof(unc_str_t) == 8 */

unc_str_t* unc_str_new(const void *init);
unc_str_t* unc_str_new_empty(void);
unc_str_t* unc_str_dup(unc_str_t *init);
void       unc_str_free(unc_str_t *pstr);
void       unc_str_clear(unc_str_t *pstr);
void       unc_str_incr_len(unc_str_t *pstr, int incr);
unc_str_t* unc_str_cat_len(unc_str_t **pstr, const void *t, size_t len);
unc_str_t* unc_str_cat(unc_str_t **pstr, const void *t);
unc_str_t* unc_str_cat_unc_str(unc_str_t **pstr, unc_str_t *pstr1);
unc_str_t* unc_str_cpy_len(unc_str_t **pstr, const void *t, size_t len);
unc_str_t* unc_str_cpy(unc_str_t **pstr, const void *t);
unc_str_t* unc_str_cat_sprintf(unc_str_t **pstr, const char *fmt, ...);
unc_str_t* unc_str_sprintf(unc_str_t **pstr, const char *fmt, ...);
void       unc_str_trim(unc_str_t *pstr, const char *cset);
int        unc_str_range(unc_str_t *pstr, int start, int end);
void       unc_str_tolower(char *s);
void       unc_str_toupper(char *s);
int        unc_str_cmp(const char *s1, const char *s2);
int        unc_str_explode(unsigned char *buf, unsigned char *field[], \
           int n, const unsigned char *ifs);
#endif
