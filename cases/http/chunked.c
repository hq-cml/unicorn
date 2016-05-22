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
 *                ����HTTP��chunked����߼�ʵ��
 * 
 *     Version :  1.0.0 
 * 
 *      Author :  HQ 
 *
 **/

#include "http.h"

/*
 * ����: ʮ�������ַ���ת��ʮ��������
 * ����: @hex, @len
 * 
 * ����:
 *      �ɹ�: ʮ��������
 *      ʧ��: -1
 */
int HexStr2Dec(const char *hex, int len)
{
    int dec = 0;
    int tmp = 0;
    int i;
    printf("len:%d\n", len);
    for(i=0; i< len; i++)
    {printf("%c\n", hex[i]);
        if(hex[i]>='a' && hex[i]<='f'){
            tmp=hex[i] - 'a' + 10;
        }else if(hex[i]>='A' && hex[i]<='F'){
            tmp=hex[i] - 'A' + 10;
        }else if(hex[i]>='0' && hex[i]<='9'){
            tmp=hex[i] - '0';
        }else{
            printf("%c\n", hex[i]);
            return UNC_ERR;
        }
        dec = dec*16+tmp;
    }
    return dec;
}



