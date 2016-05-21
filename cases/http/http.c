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

#include "http.h"

static int analysis_body_header(char *header_start, int header_length, int *body_length, conf_t *config);
static int handle_body(const char *body_start, int analysis, int body_length, conf_t *config);
static int handle_body_content_length(const char *body_start, int analysis, int body_length, conf_t *config);

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
        //拼凑一个最最简单request
        p_conf->request_body = unc_str_new("GET /info.php HTTP/1.0\r\nHost:127.0.0.1:80\r\n\r\n");
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
 *      UNC_END: 符合一个完整的包，但是请求框架主动关闭连接(处理服务器主动关闭，导致unicorn请求数减半的问题)
 *      UNC_NEEDMORE: 包长不够，需要框架继续read
 *      UNC_ERR: 出现未知错误
 *
 * 按照HTTP协议标准:
 * 一个标准的http response包含:
 *     STATUS LINE\r\n
 *     HTTP HEADER\r\n
 *     \r\n
 *     HTTP BODY
 **/
int unc_check_full_response(void *conf, void *client, void *args) 
{
    conf_t *config = (conf_t *)conf;
    client_t *p_client =(client_t *) client;
    char *recvbuf = p_client->recvbuf->buf;

    char *p_end;
    char *statusline_start;
    char *header_start;
    char *body_start;
    int statusline_length;
    int header_length;
    int body_length;
    int ret, header_analysis;

    if(config->debug) fprintf(stdout, " [DEBUG] Unc_check_full_response begin.\n");

    //尝试找到status line和header边界
    if(!(p_end = strstr(recvbuf, "\r\n\r\n")))
    {
        //如果未找到边界，则直接返回needmore，交给框架继续read
        if(config->debug) fprintf(stdout, " [DEBUG] Unc_check_full_response return: %d.\n", UNC_NEEDMORE);
        return UNC_NEEDMORE;
    }

    //定位状态行边界
    statusline_start = recvbuf;
    header_start = strstr(recvbuf, "\r\n") +2; //+2是/r/n
    statusline_length = header_start - statusline_start; 
    if(!g_http_response_line)
    {
        g_http_response_line = unc_str_newlen(statusline_start, statusline_length);
    }
    
    //定位header边界
    body_start = p_end + 4; //+4是/r/n/r/n
    header_length = body_start - header_start; 
    if(!g_http_response_header)
    {
         g_http_response_header = unc_str_newlen(header_start, header_length);    
    }

    //确定body的长度
    body_length = 0;
    header_analysis = analysis_body_header(header_start, header_length, &body_length, config);

    //处理body
    ret = handle_body(body_start, header_analysis, body_length, config);

    if(config->debug) fprintf(stdout, " [DEBUG] Unc_check_full_response return: %d.\n", ret);
    return ret;
}

/**
 * 功能: 服务端断开连接的时候调用
 * 参数: @conf, @client, @args
 * 说明: 
 *       1. 可选函数
 *       2. 有的web服务器会只返回close，也没有Content-Length或者Transfer-Encoding。这个函数就可以用于尽量保留下response
 *
 * 返回: 成功:0; 失败:-x
 **/
int unc_handle_server_close(void *conf, void *client, void *args) 
{
    conf_t *config = (conf_t *)conf;
    client_t *p_client =(client_t *) client;
    char *recvbuf = p_client->recvbuf->buf;

    char *p_end;
    char *body_start;

    //尝试找到header边界
    if(!(p_end = strstr(recvbuf, "\r\n\r\n")))
    {
        //如果未找到边界，说明出现了问题，服务端不是正常的close
        if(config->debug) fprintf(stdout, " [DEBUG] Unc_handle_server_close return: UNC_ERR.\n");
        return UNC_ERR;
    }
    
    //定位header边界
    body_start = p_end + 4; //+4是/r/n/r/n
    if(!g_http_response_body)
    {
        g_http_response_body = unc_str_new(body_start); 
        if(config->debug) fprintf(stdout, " [DEBUG] Unc_handle_server_close fill the g_http_response_body, Len: %d.\n",g_http_response_body->len);
    }
    return UNC_OK;
}



/*
 * 功能: 分析header, 计算body长度
 * 参数: @header_start, @header_length
 * 说明: 
 *       1. 查找Connection:close，如果找到则表示务端会断开连接，则应该暗示框架主动断开，避免总请求数变少
 *       2. 先找Transfer-Encoding: chunked
 *       3. 查找Content-Length，如果找到记录长度
 *       4. 都没找到，报错
 *
 * 返回:
 *       或的集合
 *       HTTP_HEADER_CHUNKED   : 表示Transfer-Encoding: chunked
 *       HTTP_HEADER_LENGTH    : 表示Content-Length
 *       HTTP_HEADER_CLOSE     : 表示服务器会断开连接
 *       HTTP_HEADER_ERR       : 未知错误
 */
static int analysis_body_header(char *header_start, int header_length, int *body_length, conf_t *config)
{
    unc_str_t *body;
    char *ptr;
    unsigned int result = 0;
    
    body = unc_str_newlen(header_start, header_length);  
    //TODO 对比g_http_response_header，看看是否会发生变化

    //查找Connection:close
    if((strcasestr(body->buf, "Connection: close"))
        || (strcasestr(body->buf, "Connection:close"))) 
    {
        result |= HTTP_HEADER_CLOSE;
    }

    //查找Content-Length:
    if((ptr = strcasestr(body->buf, "Content-Length:"))) 
    {
        *body_length = strtol(ptr + strlen("Content-Length:"), NULL, 10);
        if (*body_length  == 0) 
        {
            fprintf(stderr, "Invalid http protocol");
            exit(1);
        }
        result |= HTTP_HEADER_LENGTH;
    }
    
    //TODO 查找chunked else if(chunked)

    if(result == 0)
    {
        result |= HTTP_HEADER_ERR;
    }

    unc_str_free(body);
    if(config->debug) fprintf(stdout, " [DEBUG] Analysis_body_header return: %d.\n", result);
    return result;
}

/*
 * 功能: 根据body_length，处理body
 * 参数: @body_start, @analysis, @body_length
 * 说明: 
 *      @analysis，事先分析好的header的结果
 * 返回:
 *      UNC_OK         : 符合一个完整的包
 *      UNC_END        : 符合一个完整的包，但暗示unicorn框架主动close连接
 *      UNC_NEEDMORE   : 包长不够，需要框架继续read
 *      UNC_ERR        : 出现未知错误
 */
static int handle_body(const char *body_start, int analysis, int body_length, conf_t *config)
{
    if(analysis & HTTP_HEADER_LENGTH)
    {
        return handle_body_content_length(body_start, analysis, body_length, config);
    }
    else if(analysis & HTTP_HEADER_CHUNKED)
    {
            //Connection: Keep-Alive
            //Transfer-Encoding: chunked
            //TODO return handle_body_chunked(body_start, analysis, body_length);
            printf("Something Wrong1");
            return UNC_ERR;
    }
    else if(analysis & HTTP_HEADER_CLOSE)
    {
        //某些特殊情况，web server只返回Connection:close，却没有content-lenght或者chunked等
        //这种情况下，应该让unicorn框架一直读取，直到server端close。框架感知到之后会自己close
        if(config->debug) fprintf(stdout, " [DEBUG] Handle_body return: %d.\n", UNC_NEEDMORE);
        return UNC_NEEDMORE;
    }
    else
    {
        if(config->debug) fprintf(stdout, " [DEBUG] Handle_body return: %d.\n", UNC_ERR);
        return UNC_ERR;
    }
}

/*
 * 功能: 处理Content-Length
 * 参数: @body_start, @analysis, @body_length
 * 说明: 
 *       如果body的长度不够，则让框架继续读取；
 *       如果body的长度过多，则认为是一种异常；
 *       如果body的长度正好，则根据服务端是否会close，决定让unicorn框架是否主动close
 * 
 * 返回:
 *      UNC_OK         : 符合一个完整的包
 *      UNC_END        : 符合一个完整的包，但暗示unicorn框架主动close连接
 *      UNC_NEEDMORE   : 包长不够，需要框架继续read
 *      UNC_ERR        : 出现未知错误
 */
static int handle_body_content_length(const char *body_start, int analysis, int body_length, conf_t *config)
{
    int len = strlen(body_start);
    if(len == body_length)
    {
        if(!g_http_response_body)
        {
             g_http_response_body = unc_str_newlen(body_start, body_length);   
             if(config->debug) fprintf(stdout, " [DEBUG] Handle_body_content_length fill the g_http_response_body, Len: %d.\n",g_http_response_body->len);
        }
        if(config->debug) fprintf(stdout, " [DEBUG] Handle_body_content_length return: %d.\n", (analysis & HTTP_HEADER_CLOSE)? UNC_END : UNC_OK);
        return (analysis & HTTP_HEADER_CLOSE)? UNC_END : UNC_OK;
    }
    else if(len > body_length)
    {
        fprintf(stderr, "Get Too More. Recvbuf len:%d, Expected body len:%d\n", len, body_length);
        if(config->debug) fprintf(stdout, " [DEBUG] Handle_body_content_length return: %d.\n", UNC_ERR);
        return UNC_ERR;
    }
    else
    {
        if(config->debug) fprintf(stdout, " [DEBUG] Handle_body_content_length return: %d.\n", UNC_NEEDMORE);
        return UNC_NEEDMORE;
    }    
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
    fprintf(stdout, "************************** TEST START ****************************\n");
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
    fprintf(stdout, "*************************** TEST END *****************************\n\n\n");
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
    fprintf(stdout, "************* WELCOME TO UNICORN HTTP PRESSURE TEST ***************\n\n");

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
        fprintf(stdout, "====== THE RESPONSE STATUS LINE ( Length:%5d ) =====\n", g_http_response_line->len);
        fprintf(stdout, "%s\n", g_http_response_line->buf);
    }

    if(g_http_response_header)
    {
        fprintf(stdout, "======== THE RESPONSE HEADER ( Length:%5d ) ========\n", g_http_response_header->len);
        fprintf(stdout, "%s\n", g_http_response_header->buf);
    }

    if(g_http_response_body)
    {
        fprintf(stdout, "========= THE RESPONSE BODY ( Length:%5d ) =========\n", g_http_response_body->len);
        //fprintf(stdout, "%s\n", g_http_response_body->buf);
    }
    
    if(p_conf->response.is_get)
    {
        //TODO
    }
    fprintf(stdout, "**************** THANK YOU FOR USE UNICORN. BYE! ****************\n\n\n");

    return UNC_OK;
}



