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
#ifndef __UNC_DLL_H_INCLUDED__ 
#define __UNC_DLL_H_INCLUDED__

/*
#define UNC_OK  0
#define UNC_ERR -1
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
*/
 
typedef struct unc_symbol_struct 
{
    char    *sym_name; /* 动态库中某个函数的名称 */
    void    **sym_ptr; /* 动态库中某个函数的地址(二级指针) */
    int     no_error;  /* 如果为1，则不关心dlsym是否失败(非必选的so函数)
                        * 若为0，则dlsym失败后会释放句柄(必选的so函数)
                        */
} unc_so_symbol_t;

int unc_load_so(void **phandle, unc_so_symbol_t *syms, const char *filename);
void unc_unload_so(void **phandle);

#endif /* __UNC_DLL_H_INCLUDED__ */

