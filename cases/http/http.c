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

static int cal_body_length(char *header_start, int header_length);
static int handle_body(const char *body_start, int body_length);

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
 * 返回:
 *      UNC_OK: 符合一个完整的包
 *      UNC_NEEDMORE: 包长不够，需要框架继续read
 *      UNC_ERR: 出现未知错误
 *
 * 按照HTTP协议标准:
 * 一个标准的http response包含:
 *     STATUS LINE\r\n
 *     HTTP HEADER\r\n
 *     \r\n
 *     HTTP BODY
 *
 * BODY长度判断方法:
 * 1. 如果header中包含chunked，则用chunked模式确定body长度(优先级高于Content-Length)
 * 2. 如果header中包含Content-Length，则用Content-Length作为body长度
 * 3. 如果都没有，则利用服务器close连接来进行判断body长度
 **/
int unc_check_full_response(void *conf, void *client, void *args) 
{
    client_t *p_client =(client_t *) client;
    char *recvbuf = p_client->recvbuf->buf;

    char *p_end;
    char *status_line_start;
    char *header_start;
    char *body_start;
    int status_line_length;
    int header_length;
    int body_length;

    //尝试找到status line和header边界
    if(!(p_end = strstr(recvbuf, "\r\n\r\n")))
    {
        //如果未找到边界，则直接返回needmore，交给框架继续read
        return UNC_NEEDMORE;
    }

    //定位状态行边界
    status_line_start = recvbuf;
    header_start = strstr(recvbuf, "\r\n") +2; //+2是/r/n
    status_line_length = header_start - status_line_start; 
    if(!g_http_response_line)
    {
        g_http_response_line = unc_str_newlen(status_line_start, status_line_length);
    }
    
    //定位header边界
    body_start = p_end + 4; //+4是/r/n/r/n
    header_length = body_start - header_start; 
    if(!g_http_response_header)
    {
         g_http_response_header = unc_str_newlen(header_start, header_length);    
    }

    //处理body长度的问题
    body_length = cal_body_length(header_start, header_length);

    return handle_body(body_start, body_length);
    

    
    return UNC_OK;
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

    if(g_http_response_body)
    {
        printf("====== THE SERVER BODY ======\n");
        printf("%s\n", g_http_response_body->buf);
    }
    
    if(p_conf->response.is_get)
    {
        //TODO
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

/*
 * 功能: 计算body长度
 * 参数: @header_start, @header_length
 * 说明: 
 *       1. 先找Transfer-Encoding: chunked，如果找到，则返回-1
 *       2. 查找Content-Length，如果找到返回长度
 *       3. 查找Connection:close，如果找到则表示务端断开连接来判断长度，返回-2
 *       4. 都没找到，报错
 * 返回:
 *       -1   表示Transfer-Encoding: chunked
 *       整数 表示Content-Length
 *       -2   表示服务器会断开连接
 *       -3   未知错误
 */
static int cal_body_length(char *header_start, int header_length)
{
    unc_str_t *body;
    char *ptr;
    int len;
    body = unc_str_newlen(header_start, header_length);  
    //TODO 对比g_http_response_header，看看是否会发生变化

    //TODO 查找chunked
    if ((ptr = strstr(body->buf, "Content-Length:"))) 
    {
        len = strtol(ptr + strlen("Content-Length:"), NULL, 10);
        if (len  == 0) 
        {
            printf("Invalid http protocol");
            exit(1);
        }
        unc_str_free(body);
        return len;
    } 

    //TODO 查找Connection: Close

    
    unc_str_free(body);
    //返回未知错误
    return -3;
}

/*
 * 功能: 根据body_length，处理body
 * 参数: @header_start, @header_length
 * 说明: 
 *       1. 先找Transfer-Encoding: chunked，如果找到，则返回-1
 *       2. 查找Content-Length，如果找到返回长度
 *       3. 查找Connection:close，如果找到则表示务端断开连接来判断长度，返回-2
 *       4. 都没找到，报错
 * 返回:
 *       -1   表示Transfer-Encoding: chunked
 *       整数 表示Content-Length
 *       -2   表示服务器会断开连接
 *       -3   未知错误
 */
static int handle_body(const char *body_start, int body_length)
{
    if(body_length == -1)
    {
        //TODO 处理chunked
                printf("Something Wrong1");
        exit(1);
    }
    else if(body_length > 0)
    {
        //Content-Length
        //TODO > 如何解决?
        if(strlen(body_start) >= body_length)
        {
            if(!g_http_response_body)
            {
                 g_http_response_body = unc_str_newlen(body_start, body_length);    
            }
            return UNC_OK;
        }
        else
        {
            return UNC_NEEDMORE;
        }
    }
    else if(body_length== -2)
    {
        //TODO 处理Connection: Close
                printf("Something Wrong2");
        exit(1);
    }
    else
    {
        //TODO return err
        printf("Something Wrong3");
        exit(1);
    }
}



