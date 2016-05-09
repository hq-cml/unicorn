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
 * Description :  ����Unicorn���ܵ�HTTPѹ���ͻ���
 *                ʵ��unc_plugin.h�еĺ��������ɽ��û��߼�Ƕ��unicorn����
 *
 *     Version :  1.0.0
 *
 *      Author :  HQ
 *
 **/

#include "unc_core.h"

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
    p_conf->title = "TCP PRESSURE TEST";
    printf("************* WELCOME TO UNICORN TCP PRESSURE TEST ***************\n\n");
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
    if(p_conf->response.is_get)
    {
        printf("====== THE SERVER RESPONSE ======\n");
        printf("%s\n", p_conf->response.res_body->buf);
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
 * ����: �������ص�
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

int unc_generate_request(void *conf, void *args)
{
    conf_t *p_conf =(conf_t *) conf;
    //TODO read from a file
    /*
    GET /www/hello.php
    Connection:Keep-Alive
    */
    p_conf->request_body = unc_str_new("GET /www/hello.php 1.0\r\nConnection:Keep-Alive\r\n\r\n");
    return UNC_OK;
}

int unc_check_full_response(void *conf, void *client, void *args)
{
    client_t *p_client =(client_t *) client;

    int pos = strstr(p_client->recvbuf->buf, "world");
    if(pos != NULL)
    {
        return UNC_OK;
    }
    else
    {
        return UNC_NEEDMORE;
    }
}
