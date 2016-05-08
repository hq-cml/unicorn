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
 * Description :  ������Unicorn��ܵĳ����û�ֻ��Ҫʵ�ִ�ͷ�ļ��µĺ�������Щ�����������ʵ���ʱ������unicorn��ܵ��á�
 *
 *                ���ļ�����������Լ�����ҹ淶��so�����Ķ���͵��ò����ȡ�
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
 * ����: ��ʼ���ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 *       2. unicorn��ʼ���׶Σ����ô˺�����������һЩ��ʼ������
 * ����:�ɹ�:0; ʧ��:-x
 **/
int unc_handle_init(void *conf, void *args);

/**
 * ����: �����ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 **/
int unc_handle_finish(void *conf, void *args);

/**
 * ����: ����ǰ�ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
 **/
int unc_request_pre(void *conf, void *args);

/**
 * ����: �����ص�
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
 **/
int unc_request_post(void *conf, void *args);

/**
 * ����: ����Tcp����body
 * ����: @conf, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
 **/
int unc_generate_request(void *conf, void *args);

/**
 * ����: �жϵ�ǰ��response���ݣ��Ƿ���һ��������resoponse
 * ����: @conf, @client, @args
 * ˵��: 
 *       1. ��ѡ����
 * ����:�ɹ�:0; ʧ��:-x
 **/
int unc_check_full_request(void *conf, void *client, void *args);

__END_DECLS
#endif /* __UNC_PLUGIN_H_INCLUDED__ */

