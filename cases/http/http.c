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

#include "unc_core.h"

unc_str_t *g_http_response_line;
unc_str_t *g_http_response_header;
unc_str_t *g_http_response_body;

/**
 * 功能: 生成Tcp请求body
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_generate_request(void *conf, void *args) 
{
    conf_t *p_conf =(conf_t *) conf;
    if(!p_conf->request_body)
    {
        p_conf->request_body = unc_str_new("GET /hello.php 1.0\r\n\r\n");
    }

    return UNC_OK;
}

/**
 * 功能: 判断当前的response内容，是否是一个完整的resoponse
 * 参数: @conf, @client, @args
 * 说明: 
 *       1. 必选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_check_full_response(void *conf, void *client, void *args) 
{
    client_t *p_client =(client_t *) client;

    char *line_head_end;
    char *ptr;
    int line_len;
    int header_len;
    if((line_head_end = strstr(p_client->recvbuf->buf, "\r\n\r\n")))
    {
         //1. get status line
         ptr = strstr(p_client->recvbuf->buf, "\r\n"); 
         if(!g_http_response_line)
         {
             line_len = ptr+2-p_client->recvbuf->buf;
             g_http_response_line = unc_str_newlen(p_client->recvbuf->buf, line_len);
         }

         //2. get header 
         if(!g_http_response_header)
	 {
              header_len = line_head_end+4-(p_client->recvbuf->buf+line_len); 
              g_http_response_header = unc_str_newlen(p_client->recvbuf->buf+line_len, header_len);    
         }
         return UNC_OK;
    }

    return UNC_NEEDMORE;
}

/**
 * 功能: 初始化回调
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_handle_init(void *conf, void *args) 
{
    conf_t *p_conf =(conf_t *) conf;
    p_conf->title = "HTTP PRESSURE TEST";
    printf("************* WELCOME TO UNICORN HTTP PRESSURE TEST ***************\n\n");

    g_http_response_line = NULL;
    g_http_response_header = NULL;
    g_http_response_body = NULL;

    return UNC_OK;
}

/**
 * 功能: 结束回调
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 **/
int unc_handle_finish(void *conf, void *args) 
{
    conf_t *p_conf =(conf_t *) conf;
    if(g_http_response_line)
    {
        printf("====== THE SERVER STATUS LINE ======\n");
        printf("%s\n", g_http_response_line->buf);
    }

    if(g_http_response_header)
    {
        printf("====== THE SERVER HEADER ======\n");
        printf("%s\n", g_http_response_header->buf);
    }
    
    if(p_conf->response.is_get)
    {
        //TODO something
    }
    printf("**************** THANK YOU FOR USE UNICORN. BYE! ****************\n");

    return UNC_OK;
}

/**
 * 功能: 请求前回调
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_request_pre(void *conf, void *args) 
{
    printf("*************************** TEST START ***************************\n");
    return UNC_OK;
}

/**
 * 功能: 请求后回调
 * 参数: @conf, @args
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int unc_request_post(void *conf, void *args) 
{
    printf("*************************** TEST END ***************************\n\n\n\n");
    return UNC_OK;
}

