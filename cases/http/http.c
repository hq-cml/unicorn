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

static int analysis_body_header(char *header_start, int header_length, int *body_length, conf_t *config);
static int handle_body(const char *body_start, int analysis, int body_length, conf_t *config);
static int handle_body_content_length(const char *body_start, int analysis, int body_length, conf_t *config);
static int handle_body_chunked(const char *body_start, int analysis, int body_length, conf_t *config);


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
        //ƴ��һ�������request
        p_conf->request_body = unc_str_new("GET /info.php HTTP/1.0\r\nHost:127.0.0.1:80\r\n\r\n");
    }

    return UNC_OK;
}

/**
 * ����: �жϵ�ǰ��response���ݣ��Ƿ���һ��������resoponse
 * ����: @conf, @client, @args
 * ˵��: 
 *       1. ��ѡ����
 *       2. ����HTTPЭ���׼:
 *          һ����׼��http response����:
 *          STATUS LINE\r\n
 *          HTTP HEADER\r\n
 *          \r\n
 *          HTTP BODY
 * ����:
 *      UNC_OK         0: ����һ�������İ�
 *      UNC_END       -5: ����һ�������İ������ǰ�ʾ��������ر�����(��������������رգ�����unicorn���������������)
 *      UNC_NEEDMORE  -4: ������������Ҫ��ܼ���read
 *      UNC_ERR       -1: ����δ֪����
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

    //�����ҵ�status line��header�߽�
    if(!(p_end = strstr(recvbuf, "\r\n\r\n")))
    {
        //���δ�ҵ��߽磬��ֱ�ӷ���needmore��������ܼ���read
        if(config->debug) fprintf(stdout, " [DEBUG] Unc_check_full_response return: %d.\n", UNC_NEEDMORE);
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
    header_analysis = analysis_body_header(header_start, header_length, &body_length, config);

    //����body
    ret = handle_body(body_start, header_analysis, body_length, config);

    if(config->debug) fprintf(stdout, " [DEBUG] Unc_check_full_response return: %d.\n", ret);
    return ret;
}

/**
 * ����: ����˶Ͽ����ӵ�ʱ�����
 * ����: @conf, @client, @args
 * ˵��: 
 *       1. ��ѡ����
 *       2. �е�web��������ֻ����close��Ҳû��Content-Length����Transfer-Encoding����������Ϳ������ھ���������response
 *
 * ����: �ɹ�:0; ʧ��:-x
 **/
int unc_handle_server_close(void *conf, void *client, void *args) 
{
    conf_t *config = (conf_t *)conf;
    client_t *p_client =(client_t *) client;
    char *recvbuf = p_client->recvbuf->buf;

    char *p_end;
    char *body_start;

    //�����ҵ�header�߽�
    if(!(p_end = strstr(recvbuf, "\r\n\r\n")))
    {
        //���δ�ҵ��߽磬˵�����������⣬����˲���������close
        if(config->debug) fprintf(stdout, " [DEBUG] Unc_handle_server_close return: UNC_ERR.\n");
        return UNC_ERR;
    }
    
    //��λheader�߽�
    body_start = p_end + 4; //+4��/r/n/r/n
    if(!g_http_response_body)
    {
        g_http_response_body = unc_str_new(body_start); 
        if(config->debug) fprintf(stdout, " [DEBUG] Unc_handle_server_close fill the g_http_response_body, Len: %d.\n",g_http_response_body->len);
    }
    return UNC_OK;
}



/*
 * ����: ����header, ����body����
 * ����: @header_start, @header_length
 * ˵��: 
 *       1. ����Connection:close������ҵ����ʾ��˻�Ͽ����ӣ���Ӧ�ð�ʾ��������Ͽ�������������������
 *       2. ����Transfer-Encoding: chunked
 *       3. ����Content-Length������ҵ���¼����
 *       4. ��û�ҵ�������
 *
 * ����:
 *       ��ļ���
 *       HTTP_HEADER_CLOSE     0001: ��ʾ��������Ͽ�����
 *       HTTP_HEADER_CHUNKED   0010: ��ʾTransfer-Encoding: chunked
 *       HTTP_HEADER_LENGTH    0100: ��ʾContent-Length
 *       HTTP_HEADER_ERR       1000: δ֪����
 */
static int analysis_body_header(char *header_start, int header_length, int *body_length, conf_t *config)
{
    unc_str_t *body;
    char *ptr;
    unsigned int result = 0;
    
    *body_length = 0;
    body = unc_str_newlen(header_start, header_length);  
    //TODO �Ա�g_http_response_header�������Ƿ�ᷢ���仯

    //����Connection:close
    if((strcasestr(body->buf, "Connection: close"))
        || (strcasestr(body->buf, "Connection:close"))) 
    {
        result |= HTTP_HEADER_CLOSE;
    }

    //����Content-Length:
    if((ptr = strcasestr(body->buf, "Content-Length:"))) 
    {
        *body_length = strtol(ptr + strlen("Content-Length:"), NULL, 10);
        if (*body_length  <= 0) 
        {
            fprintf(stderr, "Invalid http protocol");
            exit(1);
        }
        result |= HTTP_HEADER_LENGTH;
    }
    
    //����Transfer-Encoding: chunked
    if((strcasestr(body->buf, "Transfer-Encoding: chunked"))
        || (strcasestr(body->buf, "Transfer-Encoding:chunked"))) 
    {
        result |= HTTP_HEADER_CHUNKED;
    }

    if(result == 0)
    {
        result |= HTTP_HEADER_ERR;
    }

    unc_str_free(body);
    if(config->debug) fprintf(stdout, " [DEBUG] Analysis_body_header return: %d.\n", result);
    return result;
}

/*
 * ����: ����body_length������body
 * ����: @body_start, @analysis, @body_length
 * ˵��: 
 *      @analysis�����ȷ����õ�header�Ľ��
 * ����:
 *      UNC_OK         0: ����һ�������İ�
 *      UNC_END       -5: ����һ�������İ�������ʾunicorn�������close����
 *      UNC_NEEDMORE  -4: ������������Ҫ��ܼ���read
 *      UNC_ERR       -1: ����δ֪����
 */
static int handle_body(const char *body_start, int analysis, int body_length, conf_t *config)
{
    if(analysis & HTTP_HEADER_LENGTH)
    {
        return handle_body_content_length(body_start, analysis, body_length, config);
    }
    else if(analysis & HTTP_HEADER_CHUNKED)
    {
        return handle_body_chunked(body_start, analysis, body_length, config);
    }
    else if(analysis & HTTP_HEADER_CLOSE)
    {
        //ĳЩ���������web serverֻ����Connection:close��ȴû��content-lenght����chunked��
        //��������£�Ӧ����unicorn���һֱ��ȡ��ֱ��server��close����ܸ�֪��֮����Լ�close
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
 * ����: ����Content-Length
 * ����: @body_start, @analysis, @body_length
 * ˵��: 
 *       ���body�ĳ��Ȳ��������ÿ�ܼ�����ȡ��
 *       ���body�ĳ��ȹ��࣬����Ϊ��һ���쳣��
 *       ���body�ĳ������ã�����ݷ�����Ƿ��close��������unicorn����Ƿ�����close
 * 
 * ����:
 *      UNC_OK         0: ����һ�������İ�
 *      UNC_END       -5: ����һ�������İ�������ʾunicorn�������close����
 *      UNC_NEEDMORE  -4: ������������Ҫ��ܼ���read
 *      UNC_ERR       -1: ����δ֪����
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

/*
 * ����: ����Transfer-Encoding: chunked
 * ����: @body_start, @analysis, @body_length
 * ˵��: 
 * 
 * ����:
 *      UNC_OK         0: ����һ�������İ�
 *      UNC_END       -5: ����һ�������İ�������ʾunicorn�������close����
 *      UNC_NEEDMORE  -4: ������������Ҫ��ܼ���read
 *      UNC_ERR       -1: ����δ֪����
 */
static int handle_body_chunked(const char *body_start, int analysis, int body_length, conf_t *config)
{
    int result = handle_chunked(body_start, body_length, config);
    exit(1);
    return (analysis & HTTP_HEADER_CLOSE)? UNC_NEEDMORE : UNC_ERR;
    /*
    len = strlen(body_start);
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
    */
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



