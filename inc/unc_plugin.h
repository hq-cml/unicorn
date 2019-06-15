/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_plugin.h
 * 
 * Description :  依附于Unicorn框架的程序，用户只需要实现此头文件下的函数，这些函数，会在适当的时机，被unicorn框架调用。
 *
 *                该文件的作用在于约定并且规范化so函数的定义和调用参数等。
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/

#ifndef __UNC_PLUGIN_H_INCLUDED__
#define __UNC_PLUGIN_H_INCLUDED__
/*
#include <sys/cdefs.h>
#include "unc_core.h"
*/
__BEGIN_DECLS

/**
 * 功能: 初始化回调
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 *       2. unicorn初始化阶段，调用此函数，可以做一些初始化工作
 * 返回:成功:0; 失败:-x
 **/
int unc_handle_init(void *conf, void *args);

/**
 * 功能: 结束回调
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 **/
int unc_handle_finish(void *conf, void *args);

/**
 * 功能: 请求前回调
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_request_pre(void *conf, void *args);

/**
 * 功能: 请求后回调
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_request_post(void *conf, void *args);

/**
 * 功能: 生成Tcp请求body
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_generate_request(void *conf, void *args);

/**
 * 功能: 判断当前的response内容，是否是一个完整的resoponse
 * 参数: @conf, @client, @args
 * 说明: 
 *       1. 必选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_check_full_request(void *conf, void *client, void *args);

/**
 * 功能: 服务端关闭连接的时候调用
 * 参数: @conf, @client, @args
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_handle_server_close(void *conf, void *client, void *args);

__END_DECLS
#endif /* __UNC_PLUGIN_H_INCLUDED__ */

