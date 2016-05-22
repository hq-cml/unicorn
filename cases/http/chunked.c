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
 *                ����HTTP��chunked����߼�����
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
 * ����: ����Transfer-Encoding: chunked
 * ����: @body_start, @analysis
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
 *      UNC_NEEDMORE  -4: ������������Ҫ��ܼ���read
 *      UNC_ERR       -1: ����δ֪��������
 */
int handle_chunked(const char *body_start, conf_t *config)
{
    char *ptr = (char *)body_start;
    char *p;
    int len;
    int result;
    int body_sentry = strlen(body_start); //body�ڱ�
    int chunk_size;
    unc_str_t *tmp = unc_str_new_empty(); //TODO ��ֹ�ڴ�й©

    if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked begin.\n");

    //��ȡchunk-size, chunk-extension��CRLF
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
                //����chunk_size>0, ���Բ������һ��chunk�������ϴ˿����ݿ϶�����������Ҫ����read
                if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked Need (%ld) more\n", ((ptr+chunk_size+2) - (body_start+body_sentry)));
                unc_str_free(tmp);
                return UNC_NEEDMORE;
            }

            //��chunk-size��С��chunk-data,skip CRLF�������˿�chunk-data׷�ӵ�entity-body��
            unc_str_cat_len(&tmp, ptr, chunk_size);

            //��ȡ��chunk��chunk-size �� CRLF
            ptr = ptr+chunk_size+2;
        }
    }while( chunk_size > 0 ); //chunk_size > 0��������last-chunk

    if(chunk_size != 0)
    {
        //chunk_size == 0�����Ѵ�����last-chunk���˿�ptrָ��last-chunk֮�󣬿��ܴ���trailer��Ҳ���ܲ�����
        if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked Err: last-chunk-size should be 0.\n");
        unc_str_free(tmp);
        return UNC_ERR;
    }
    
    if(!g_http_response_body)
    {
         g_http_response_body = unc_str_dup(tmp);
         if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked fill the g_http_response_body, Len: %d.\n",g_http_response_body->len);
    }

    //TODO ����trailer�Ĵ������ﲻ����
    /**
     * read entity-header      //entity-header�ĸ�ʽΪname:valueCRLF,���Ϊ�ռ�ֻ��CRLF
     * while ��entity-header not empty)   //��������ֻ��CRLF�Ŀ���
     * {
     *     append entity-header to existing header fields
     *     read entity-header
     *  }
     * Content-Length:=length      //�������������̽��������õ����±�����length����ΪContent-Length���ֵд�뱨����
     * Remove "chunked" from Transfer-Encoding  //ͬʱ��Transfer-Encoding����ֵȥ��chunked������
     */

    unc_str_free(tmp);
    if(strlen(ptr) && memcmp(ptr, "\r\n", 2)==0)
    {
        //û��trailer
        result = UNC_OK;
    }
    else
    {
        //�����trailer�������ҵ�trailer��β�����ս�β
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
                result = UNC_OK;  //����ҵ��ľ������ս�β���򷵻�ok
            }
            else
            {
                result = UNC_ERR; //����������ս�β��˵���϶����ڴ���
            }
        }
    }
    
    if(config->debug) fprintf(stdout, " [DEBUG] Handle_chunked return: %d.\n", result);
    return result;
}

/*
 * ����: ��ȡchunk-size, chunk-extension��CRLF
 * ����: @str, @p_len, @config
 * 
 * ˵��: 
 *      p_len������㣬�����˶��ٸ��ֽڳ��ȣ�������㴦��
 * ����:
 *      �ɹ�: chunk-size������chunk-extension
 *      ʧ��: UNC_ERR ����δ֪��������
 *      UNC_NEEDMORE: �ÿ�ܼ�����ȡ
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
    *p_len = len+2;//+2 ��ʾ\r\nռ�õĳ���
    
    snprintf(buf, len+1, "%s", str); //snprintf�������Ȱ���\0

    //�������chunk-extension�������֮
    p2 = strstr(buf, ";");
    if(p2)
    {
        *p2 = '\0';
    }
    
    chunk_size = HexStr2Dec(buf, len);
    if(!config->quiet && config->debug) fprintf(stdout, " [DEBUG] Get_chunk_size: Hex(%s), Dec(%d).\n", buf, chunk_size);
    return chunk_size;
}

