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

#include "unc_core.h"

unc_str_t *g_http_response_line;
unc_str_t *g_http_response_header;
unc_str_t *g_http_response_body;

static int cal_body_length(char *header_start, int header_length);

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
 * ����:�ɹ�:0; ʧ��:-x
 *
 * ����HTTPЭ���׼:
 * һ����׼��http response����:
 *     STATUS LINE\r\n
 *     HTTP HEADER\r\n
 *     \r\n
 *     HTTP BODY
 *
 * BODY�����жϷ���:
 * 1. ���header�а���chunked������chunkedģʽȷ��body����(���ȼ�����Content-Length)
 * 2. ���header�а���Content-Length������Content-Length��Ϊbody����
 * 3. �����û�У������÷�����close�����������ж�body����
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

    //�����ҵ�status line��header�߽�
    if(!(p_end = strstr(recvbuf, "\r\n\r\n")))
    {
        //���δ�ҵ��߽磬��ֱ�ӷ���needmore��������ܼ���read
        return UNC_NEEDMORE;
    }

    //��λ״̬�б߽�
    status_line_start = recvbuf;
    header_start = strstr(recvbuf, "\r\n") +2; //+2��/r/n
    status_line_length = header_start - status_line_start; 
    if(!g_http_response_line)
    {
        g_http_response_line = unc_str_newlen(status_line_start, status_line_length);
    }
    
    //��λheader�߽�
    body_start = p_end + 4; //+4��/r/n/r/n
    header_length = body_start - header_start; 
    if(!g_http_response_header)
    {
         g_http_response_header = unc_str_newlen(header_start, header_length);    
    }

    //����body���ȵ�����
    body_length = cal_body_length(header_start, header_length);
    printf("len%d\n", body_length);
    
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
    printf("************* WELCOME TO UNICORN HTTP PRESSURE TEST ***************\n\n");

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
 * ����: ����ǰ�ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
 **/
int unc_request_pre(void *conf, void *args) 
{
    printf("*************************** TEST START ***************************\n");
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
    printf("*************************** TEST END ***************************\n\n\n\n");
    return UNC_OK;
}

/*
 * ����: ����body����
 * ����: @header_start, @header_length
 * ˵��: 
 *       1. ����Transfer-Encoding: chunked������ҵ����򷵻�-1
 *       2. ����Content-Length������ҵ����س���
 *       3. ����Connection:close������ҵ����ʾ��˶Ͽ��������жϳ��ȣ�����-2
 *       4. ��û�ҵ�������
 * ����:
 *       -1   ��ʾTransfer-Encoding: chunked
 *       ���� ��ʾContent-Length
 *       -2   ��ʾ��������Ͽ�����
 *       -3   λ�ô���
 */
static int cal_body_length(char *header_start, int header_length)
{
    unc_str_t *body;
    char *ptr;
    int len;
    body = unc_str_newlen(header_start, header_length);  
    //TODO �Ա�g_http_response_header�������Ƿ�ᷢ���仯

    if ((ptr = strstr(body->buf, "Content-Length:"))) 
    {
        len = strtol(ptr + strlen("Content-Length:"), NULL, 10);
        if (len  == 0) 
        {
            printf("Invalid http protocol");
            exit(1);
        }
        return len;
    } 
    return 0;
    unc_str_free(body);
}


