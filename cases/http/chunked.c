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
 *                负责HTTP的chunked相关逻辑实现
 * 
 *     Version :  1.0.0 
 * 
 *      Author :  HQ 
 *
 **/

#include "http.h"

/*
 * 功能: 十六进制字符串转成十进制整形
 * 参数: @hex, @len
 * 
 * 返回:
 *      成功: 十进制数字
 *      失败: -1
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
 * 功能: 处理Transfer-Encoding: chunked
 * 参数: @body_start, @analysis, @body_length
 * 说明: 
 *     Chunked编码的基本方法是将大块数据分解成多块小数据，每块都可以自指定长度，其具体格式如下(BNF文法)：
 *     Chunked-Body   = *chunk                        //0至多个chunk
 *     last-chunk                                     //最后一个chunk
 *     trailer                                        //尾部
 *     CRLF                                           //结束标记符
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
 * 解释：
 *     1. Chunked-Body表示经过chunked编码后的报文体。报文体可以分为chunk, last-chunk，trailer和结束符四部分。
 *        chunk的数量在报文体中最少可以为0，无上限；
 *     2. 每个chunk的长度是自指定的，即，起始的数据必然是16进制数字的字符串，代表后面chunk-data的长度（字节数）。
 *        这个16进制的字符串第一个字符如果是“0”，则表示chunk-size为0，该chunk为last-chunk,无chunk-data部分。
 *     3. 可选的chunk-extension由通信双方自行确定，如果接收者不理解它的意义，可以忽略。
 *     4. trailer是附加的在尾部的额外头域，通常包含一些元数据（metadata, meta means "about information"）
 *        这些头域可以在解码后附加在现有头域之后
 * 
 * 返回:
 *      UNC_OK         0: 符合一个完整的包
 *      UNC_END       -5: 符合一个完整的包，但暗示unicorn框架主动close连接
 *      UNC_NEEDMORE  -4: 包长不够，需要框架继续read
 *      UNC_ERR       -1: 出现未知解析错误
 */
int handle_chunked(const char *body_start, int body_length, conf_t *config)
{
    char *p1;
    int len;
    int final_body_length = 0;   //长度计数器置0

    int chunk_size;


    chunk_size = get_chunk_size(body_start, &len, config);
    printf("%d\n", chunk_size);
    printf("%d\n", len);


    /*
    read chunk-size, chunk-extension (if any) and CRLF      
    while(chunk-size > 0 )  
    {  
        //表明不是last-chunk
        read chunk-data and CRLF            //读chunk-size大小的chunk-data,skip CRLF
        append chunk-data to entity-body     //将此块chunk-data追加到entity-body后
        length := length + chunk-size
        read chunk-size and CRLF          //读取新chunk的chunk-size 和 CRLF
    }
    read entity-header      //entity-header的格式为name:valueCRLF,如果为空即只有CRLF
    while （entity-header not empty)   //即，不是只有CRLF的空行
    {
        append entity-header to existing header fields
        read entity-header
    }
    Content-Length:=length      //将整个解码流程结束后计算得到的新报文体length，作为Content-Length域的值写入报文中
    Remove "chunked" from Transfer-Encoding  //同时从Transfer-Encoding中域值去除chunked这个标记
    */
    return UNC_ERR;
}
