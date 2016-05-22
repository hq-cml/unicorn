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

/*
 * ����: ����Transfer-Encoding: chunked
 * ����: @body_start, @analysis, @body_length
 * ˵��: 
 *     Chunked����Ļ��������ǽ�������ݷֽ�ɶ��С���ݣ�ÿ�鶼������ָ�����ȣ�������ʽ����(BNF�ķ�)��
 *     Chunked-Body   = *chunk                        //0�����chunk
 *     last-chunk                                     //���һ��chunk
 *     trailer                                        //β��
 *     CRLF                                           //������Ƿ�
 *     chunk  = chunk-size [ chunk-extension ] CRLF
 *     chunk-data CRLF
 *     chunk-size     = 1*HEX
 *     last-chunk     = 1*("0") [ chunk-extension ] CRLF
 *     chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
 *     chunk-ext-name = token
 *     chunk-ext-val  = token | quoted-string
 *     chunk-data     = chunk-size(OCTET)
 *     trailer        = *(entity-header CRLF)
 *  
 * ���ͣ�
 *     1. Chunked-Body��ʾ����chunked�����ı����塣��������Է�Ϊchunk, last-chunk��trailer�ͽ������Ĳ��֡�
 *        chunk�������ڱ����������ٿ���Ϊ0�������ޣ�
 *     2. ÿ��chunk�ĳ�������ָ���ģ�������ʼ�����ݱ�Ȼ��16�������ֵ��ַ������������chunk-data�ĳ��ȣ��ֽ�������
 *        ���16���Ƶ��ַ�����һ���ַ�����ǡ�0�������ʾchunk-sizeΪ0����chunkΪlast-chunk,��chunk-data���֡�
 *     3. ��ѡ��chunk-extension��ͨ��˫������ȷ������������߲�����������壬���Ժ��ԡ�
 *     4. trailer�Ǹ��ӵ���β���Ķ���ͷ��ͨ������һЩԪ���ݣ�metadata, meta means "about information"��
 *        ��Щͷ������ڽ���󸽼�������ͷ��֮��
 * 
 * ����:
 *      UNC_OK         0: ����һ�������İ�
 *      UNC_END       -5: ����һ�������İ�������ʾunicorn�������close����
 *      UNC_NEEDMORE  -4: ������������Ҫ��ܼ���read
 *      UNC_ERR       -1: ����δ֪��������
 */
int handle_chunked(const char *body_start, int body_length, conf_t *config)
{
    char *p1;
    int len;
    int final_body_length = 0;   //���ȼ�������0

    int chunk_size;


    chunk_size = get_chunk_size(body_start, &len, config);
    printf("%d\n", chunk_size);
    printf("%d\n", len);


    /*
    read chunk-size, chunk-extension (if any) and CRLF      
    while(chunk-size > 0 )  
    {  
        //��������last-chunk
        read chunk-data and CRLF            //��chunk-size��С��chunk-data,skip CRLF
        append chunk-data to entity-body     //���˿�chunk-data׷�ӵ�entity-body��
        length := length + chunk-size
        read chunk-size and CRLF          //��ȡ��chunk��chunk-size �� CRLF
    }
    read entity-header      //entity-header�ĸ�ʽΪname:valueCRLF,���Ϊ�ռ�ֻ��CRLF
    while ��entity-header not empty)   //��������ֻ��CRLF�Ŀ���
    {
        append entity-header to existing header fields
        read entity-header
    }
    Content-Length:=length      //�������������̽��������õ����±�����length����ΪContent-Length���ֵд�뱨����
    Remove "chunked" from Transfer-Encoding  //ͬʱ��Transfer-Encoding����ֵȥ��chunked������
    */
    return UNC_ERR;
}
