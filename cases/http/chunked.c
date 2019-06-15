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
 *                负责HTTP的chunked相关逻辑处理
 * 
 *     Version :  1.0.0 
 * 
 *      Author :  HQ 
 *
 **/

#include "http.h"

static int get_chunk_size(const char *start, int *len, conf_t *config);
extern unc_str_t *g_http_response_body;

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

    for(i=0; i< len; i++)
    {
        if(hex[i]>='a' && hex[i]<='f'){
            tmp=hex[i] - 'a' + 10;
        }else if(hex[i]>='A' && hex[i]<='F'){
            tmp=hex[i] - 'A' + 10;
        }else if(hex[i]>='0' && hex[i]<='9'){
            tmp=hex[i] - '0';
        }else{
            return UNC_ERR;
        }
        dec = dec*16+tmp;
    }
    return dec;
}

/*
 * 功能: 处理Transfer-Encoding: chunked
 * 参数: @body_start, @analysis
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
 *      UNC_NEEDMORE  -4: 包长不够，需要框架继续read
 *      UNC_ERR       -1: 出现未知解析错误
 */
int handle_chunked(const char *body_start, conf_t *config)
{
    char *ptr = (char *)body_start;
    char *p;
    int len;
    int result;
    int body_sentry = strlen(body_start); //body哨兵
    int chunk_size;
    unc_str_t *tmp = unc_str_new_empty(); //TODO 防止内存泄漏

    if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked begin.\n");

    //读取chunk-size, chunk-extension和CRLF
    do{ 
        chunk_size = get_chunk_size(ptr, &len, config); 
        ptr += len;
        if(chunk_size== UNC_ERR)
        {
            if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked Err: parse error.\n");
            unc_str_free(tmp);
            return UNC_ERR;
        }
        else if(chunk_size== UNC_NEEDMORE)
        {
            if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked Need more\n");
            unc_str_free(tmp);
            return UNC_NEEDMORE;
        }

        if(chunk_size > 0)
        {
            if(ptr+chunk_size+2 >= body_start+body_sentry)
            {
                //由于chunk_size>0, 所以不是最后一个chunk，理论上此刻数据肯定不够，还需要继续read
                if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked Need (%ld) more\n", ((ptr+chunk_size+2) - (body_start+body_sentry)));
                unc_str_free(tmp);
                return UNC_NEEDMORE;
            }

            //读chunk-size大小的chunk-data,skip CRLF，并将此块chunk-data追加到entity-body后
            unc_str_cat_len(&tmp, ptr, chunk_size);

            //读取新chunk的chunk-size 和 CRLF
            ptr = ptr+chunk_size+2;
        }
    }while( chunk_size > 0 ); //chunk_size > 0表明不是last-chunk

    if(chunk_size != 0)
    {
        //chunk_size == 0表明已处理完last-chunk，此刻ptr指向last-chunk之后，可能存在trailer，也可能不存在
        if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked Err: last-chunk-size should be 0.\n");
        unc_str_free(tmp);
        return UNC_ERR;
    }
    
    if(!g_http_response_body)
    {
         g_http_response_body = unc_str_dup(tmp);
         if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked fill the g_http_response_body, Len: %d.\n",g_http_response_body->len);
    }

    //TODO 关于trailer的处理，这里不关心
    /**
     * read entity-header      //entity-header的格式为name:valueCRLF,如果为空即只有CRLF
     * while （entity-header not empty)   //即，不是只有CRLF的空行
     * {
     *     append entity-header to existing header fields
     *     read entity-header
     *  }
     * Content-Length:=length      //将整个解码流程结束后计算得到的新报文体length，作为Content-Length域的值写入报文中
     * Remove "chunked" from Transfer-Encoding  //同时从Transfer-Encoding中域值去除chunked这个标记
     */

    unc_str_free(tmp);
    if(strlen(ptr) && memcmp(ptr, "\r\n", 2)==0)
    {
        //没有trailer
        result = UNC_OK;
    }
    else
    {
        //如果有trailer，尝试找到trailer结尾和最终结尾
        p = strstr(ptr, "\r\n\r\n");
        if(p == NULL)
        {
            result = UNC_NEEDMORE;
        }
        else
        {
            if(strlen(p) == 4)
            {
                if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked find a trailler.\n");
                result = UNC_OK;  //如果找到的就是最终结尾，则返回ok
            }
            else
            {
                result = UNC_ERR; //如果不是最终结尾，说明肯定存在错误
            }
        }
    }
    
    if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked return: %d.\n", result);
    return result;
}

/*
 * 功能: 读取chunk-size, chunk-extension和CRLF
 * 参数: @str, @p_len, @config
 * 
 * 说明: 
 *      p_len告诉外层，消耗了多少个字节长度，便于外层处理
 * 返回:
 *      成功: chunk-size，忽略chunk-extension
 *      失败: UNC_ERR 出现未知解析错误
 *      UNC_NEEDMORE: 让框架继续读取
 */
static int get_chunk_size(const char *str, int *p_len, conf_t *config)
{
    char *p1, *p2;
    int len, chunk_size=0;
    char buf[1024] = {0};

    p1 = strstr(str, "\r\n");
    if(!p1)
    {
        if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked Failed: can not find \\r\\n. Read more.\n");
        return UNC_NEEDMORE;
    }

    len = p1-str;
    *p_len = len+2;//+2 表示\r\n占用的长度
    
    snprintf(buf, len+1, "%s", str); //snprintf参数长度包括\0

    //如果存在chunk-extension，则忽略之
    p2 = strstr(buf, ";");
    if(p2)
    {
        *p2 = '\0';
    }
    
    chunk_size = HexStr2Dec(buf, len);
    if(!config->quiet && config->debug) fprintf(stdout, " [DEBUG] Get_chunk_size: Hex(%s), Dec(%d).\n", buf, chunk_size);
    return chunk_size;
}

