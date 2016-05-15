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

#include "http.h"

unc_str_t *g_http_response_line;
unc_str_t *g_http_response_header;
unc_str_t *g_http_response_body;

/**
 * ����: ����Tcp����body
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
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
 * ����: �жϵ�ǰ��response���ݣ��Ƿ���һ��������resoponse
 * ����: @conf, @client, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:
 *      UNC_OK: ����һ�������İ�
 *      UNC_NEEDMORE: ������������Ҫ��ܼ���read
 *      UNC_ERR: ����δ֪����
 *
 * ����HTTPЭ���׼:
 * һ����׼��http response����:
 *     STATUS LINE\r\n
 *     HTTP HEADER\r\n
 *     \r\n
 *     HTTP BODY
 **/
int unc_check_full_response(void *conf, void *client, void *args) 
{
    client_t *p_client =(client_t *) client;
    char *recvbuf = p_client->recvbuf->buf;

    char *p_end;
    char *statusline_start;
    char *header_start;
    char *body_start;
    int statusline_length;
    int header_length;
    int body_length;
    int ret;

    //�����ҵ�status line��header�߽�
    if(!(p_end = strstr(recvbuf, "\r\n\r\n")))
    {
        //���δ�ҵ��߽磬��ֱ�ӷ���needmore��������ܼ���read
        return UNC_NEEDMORE;
    }

    //��λ״̬�б߽�
    statusline_start = recvbuf;
    header_start = strstr(recvbuf, "\r\n") +2; //+2��/r/n
    statusline_length = header_start - statusline_start; 
    if(!g_http_response_line)
    {
        g_http_response_line = unc_str_newlen(statusline_start, statusline_length);
    }
    
    //��λheader�߽�
    body_start = p_end + 4; //+4��/r/n/r/n
    header_length = body_start - header_start; 
    if(!g_http_response_header)
    {
         g_http_response_header = unc_str_newlen(header_start, header_length);    
    }

    //ȷ��body�ĳ���
    body_length = cal_body_length(header_start, header_length);

    //����body
    ret = handle_body(body_start, body_length);
    //TODO ����UNC_ERR
    return ret;
    
    return UNC_OK;
}

/**
 * ����: ����˶Ͽ����ӵ�ʱ�����
 * ����: @conf, @client, @args
 * ˵��: 
 *       1. ��ѡ����
 *
 * ����: �ɹ�:0; ʧ��:-x
 **/
int unc_handle_server_close(void *conf, void *client, void *args) 
{
    
    client_t *p_client =(client_t *) client;
    char *recvbuf = p_client->recvbuf->buf;

    char *p_end;
    char *body_start;

    //�����ҵ�header�߽�
    if(!(p_end = strstr(recvbuf, "\r\n\r\n")))
    {
        //���δ�ҵ��߽磬˵�����������⣬����˲���������close
        return UNC_ERR;
    }
    
    //��λheader�߽�
    body_start = p_end + 4; //+4��/r/n/r/n
    if(!g_http_response_body)
    {
         g_http_response_body = unc_str_new(body_start);    
    }
    return UNC_OK;
}



/*
 * ����: ����body����
 * ����: @header_start, @header_length
 * ˵��: 
 *       0. Connection:close���ȼ���ߣ����Թ��unicorn����EPIPE������
 *       1. ����Connection:close������ҵ����ʾ��˶Ͽ��������жϳ��ȣ�����-2
 *       2. ����Transfer-Encoding: chunked������ҵ����򷵻�-1
 *       3. ����Content-Length������ҵ����س���
 *       4. ��û�ҵ�������
 * ����:
 *       HTTP_BODY_CHUNKED   : ��ʾTransfer-Encoding: chunked
 *       ����                : ��ʾContent-Length
 *       HTTP_BODY_CLOSE     : ��ʾ��������Ͽ�����
 *       HTTP_BODY_ERR       : δ֪����
 */
static int cal_body_length(char *header_start, int header_length)
{
    unc_str_t *body;
    char *ptr;
    int len;
    body = unc_str_newlen(header_start, header_length);  
    //TODO �Ա�g_http_response_header�������Ƿ�ᷢ���仯

    
    if((strcasestr(body->buf, "Connection: close"))
        || (strcasestr(body->buf, "Connection:close"))) 
    {
        //Connection: Close
        unc_str_free(body);
        return HTTP_BODY_CLOSE;
    }
    //TODO ����chunked else if(chunked)
    else if((ptr = strcasestr(body->buf, "Content-Length:"))) 
    {
        //Content-Length:
        len = strtol(ptr + strlen("Content-Length:"), NULL, 10);
        if (len  == 0) 
        {
            fprintf(stderr, "Invalid http protocol");
            exit(1);
        }
        unc_str_free(body);
        return len;
    }
    else
    {
        unc_str_free(body);
        return HTTP_BODY_ERR;
    }
}

/*
 * ����: ����body_length������body
 * ����: @body_start, @body_length
 * ˵��: 
 *
 * ����:
 *      UNC_OK         : ����һ�������İ�
 *      UNC_NEEDMORE   : ������������Ҫ��ܼ���read
 *      UNC_ERR        : ����δ֪����
 */
static int handle_body(const char *body_start, int body_length)
{
    if(body_length== HTTP_BODY_CLOSE)
    {
        //Connection: Close
        return UNC_NEEDMORE;
    }
    else if(body_length == HTTP_BODY_CHUNKED)
    {
        //Connection: Keep-Alive
        //Transfer-Encoding: chunked
        //TODO ����chunked
        printf("Something Wrong1");
        exit(1);
    }
    else if(body_length > 0)
    {
        //Connection: Keep-Alive
        //Content-Length: xxx
        return handle_body_content_length(body_start, body_length);
    }
    else
    {
        //TODO return err
        printf("Something Wrong3");
        exit(1);
    }
}

/*
 * ����: ���� Content-Length
 * ����: @body_start, @body_length
 * ˵��: 
 *
 * ����:
 *      UNC_OK         : ����һ�������İ�
 *      UNC_NEEDMORE   : ������������Ҫ��ܼ���read
 *      UNC_ERR        : ����δ֪����
 */
static int handle_body_content_length(const char *body_start, int body_length)
{
    int len = strlen(body_start);
    if(len == body_length)
    {
        if(!g_http_response_body)
        {
             g_http_response_body = unc_str_newlen(body_start, body_length);    
        }
        return UNC_OK;
    }
    else if(len > body_length)
    {
        fprintf(stderr, "Get Too More. Recvbuf len:%d, Expected body len:%d\n", len, body_length);
        return UNC_ERR;
    }
    else
    {
        return UNC_NEEDMORE;
    }    
}

/**
 * ����: ����ǰ�ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
 **/
int unc_request_pre(void *conf, void *args) 
{
    fprintf(stdout, "************************** TEST START ****************************\n");
    return UNC_OK;
}

/**
 * ����: �����ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
 **/
int unc_request_post(void *conf, void *args) 
{
    fprintf(stdout, "*************************** TEST END *****************************\n\n\n");
    return UNC_OK;
}

/**
 * ����: ��ʼ���ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
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
 * ����: �����ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
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



