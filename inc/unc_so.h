/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *     Filename:  Unc_so.h 
 * 
 *  Description:  A dynamic library implemention. 
 * 
 *      Version:  1.0.0
 * 
 *       Author:  HQ 
 *
 **/
#ifndef __UNC_SO_H_INCLUDED__ 
#define __UNC_SO_H_INCLUDED__

/*
#define UNC_OK  0
#define UNC_ERR -1
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
*/
 
typedef struct unc_symbol_struct 
{
    char    *sym_name; /* ��̬����ĳ������������ */
    void    **sym_ptr; /* ��̬����ĳ�������ĵ�ַ(����ָ��) */
    int     no_error;  /* ���Ϊ1���򲻹���dlsym�Ƿ�ʧ��(�Ǳ�ѡ��so����)
                        * ��Ϊ0����dlsymʧ�ܺ���ͷž��(��ѡ��so����)
                        */
} unc_so_symbol_t;

int unc_load_so(void **phandle, unc_so_symbol_t *syms, const char *filename);
void unc_unload_so(void **phandle);

#endif /* __UNC_SO_H_INCLUDED__ */

