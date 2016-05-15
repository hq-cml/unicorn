/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *     Filename:  unicorn.c 
 * 
 *  Description:  ������߼��� 
 * 
 *      Version:  1.0.0 
 * 
 *       Author:  HQ 
 *
 **/

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include "unc_ae.h"
#include "unc_dlist.h"
#include "unc_string.h"
#include "unc_anet.h"
*/
#include "unc_core.h"

//��������
static void client_done(client_t *c, int server_close);
static int show_qps(unc_ae_event_loop *el, long long id, void *priv);
static void usage(int status);
static void write_handler(unc_ae_event_loop *el, int fd, void *priv, int mask);
static void create_multi_clients(int num);
static void read_handler(unc_ae_event_loop *el, int fd, void *priv, int mask);
static void free_all_clients();
static void show_final_report(void);
static void free_client(client_t *c);

//ȫ�־������
static conf_t g_conf;

//��̬��������
unc_so_func_t    g_so;
void *g_handle;

/* ����syms��g_so���������ı�����ʼ�� */
static unc_so_symbol_t syms[] = 
{
    /* symbol_name,                      function pointer,                            optional */
    {"unc_handle_init",            (void **)&g_so.handle_init,              1}, /* ��ѡ */
    {"unc_handle_finish",          (void **)&g_so.handle_finish,            1}, /* ��ѡ */
    {"unc_request_pre",            (void **)&g_so.request_pre,              1}, /* ��ѡ */
    {"unc_request_post",           (void **)&g_so.request_post,             1}, /* ��ѡ */
    {"unc_generate_request",       (void **)&g_so.generate_request,         1}, /* ��ѡ */
    {"unc_check_full_response",    (void **)&g_so.check_full_response,      0}, /* ��ѡ */
    {"unc_handle_server_close",    (void **)&g_so.handle_server_close,      1}, /* ��ѡ */
    {NULL, NULL, 0}
};

/* 
 * ��õ�ǰ΢���� 
 */
static long long ustime() 
{
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long)tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

/* 
 * ��õ�ǰ������ 
 */
static long long mstime() 
{
    struct timeval tv;
    long long mst;

    gettimeofday(&tv, NULL);
    mst = ((long)tv.tv_sec) * 1000;
    mst += tv.tv_usec / 1000;
    return mst;
}

/*
 * ȫ�־��Ĭ��ֵ��ʼ��
 */
static void init_conf() 
{
    g_conf.title = NULL;
    g_conf.requests_sended   = 0;
    g_conf.requests_done     = 0;
    g_conf.requests_finished = 0;
    g_conf.num_clients = 1;
    g_conf.requests = 1;
    g_conf.live_clients = 0;
    g_conf.keep_alive = 1;
    g_conf.loop = 0;
    g_conf.epipe = 0;
    g_conf.quiet = 0;
    g_conf.el = unc_ae_create_event_loop();
    unc_ae_create_time_event(g_conf.el, 3000, show_qps, NULL, NULL);
    g_conf.clients = unc_dlist_init();
    g_conf.hostip = "127.0.0.1";
    g_conf.hostport = 9527;
    g_conf.so_file = "./libfunc.so";
    g_conf.request_file = NULL;
    g_conf.response.is_get = 0;
    g_conf.response.res_body = NULL;
    g_conf.request_body = NULL;
    g_conf.done_if_srv_close = 1;
    return;
}

/* 
 * ʱ���¼�: ��ӡ��ǰ��QPS(�ӿ�ʼ����ǰ���ۼ�ֵ) 
 */
static int show_qps(unc_ae_event_loop *el, long long id, void *priv) 
{
    float dt = (float)(mstime() - g_conf.start) / 1000.0;
    float rps = (float)g_conf.requests_done / dt;
    fprintf(stdout, "%s: %.2f r/s\n", g_conf.title, rps);
    return 3000; /* every 3000ms */
}

/*
 * ��������
 */
static void parse_options(int argc, char **argv) 
{
    char c;

    while ((c = getopt(argc, argv, "h:p:c:s:f:n:k:w:qlHe")) != -1) 
    {
        switch (c) {
        case 'h':
            g_conf.hostip = strdup(optarg);
            break;
        case 'p':
            g_conf.hostport = atoi(optarg);
            break;
        case 'c':
            g_conf.num_clients = atoi(optarg);
            break;
        case 'n':
            g_conf.requests = atoi(optarg);
            break;
        case 's':
            g_conf.so_file= strdup(optarg);
            break;
        case 'f':
            g_conf.request_file = strdup(optarg);
            break;
        case 'k':
            g_conf.keep_alive = atoi(optarg);
            break;
        case 'w':
            g_conf.done_if_srv_close = atoi(optarg);
            break;           
        case 'q':
            g_conf.quiet = 1;
            break;
        case 'l':
            g_conf.loop = 1;
            break;
        case 'e':
            g_conf.epipe = 1;
            break;
        case 'H':
            usage(0);
            break;
        default:
            usage(1);
        }
    }
}

/*
 * ��ӡʹ�ð�������
 */
static void usage(int status) 
{
    puts("Usage: ./Unicorn [-h <host>] [-p <port>] "
            "[-c <clients>] [-n requests]> [-k <boolean>]\n");
    puts(" -h <hostname>      server hostname (default 127.0.0.1)");
    puts(" -p <port>          server port (default 9527)");
    puts(" -c <clients>       number of parallel connections (default 1)");
    puts(" -n <requests>      total number of requests (default 1)");   
    puts(" -s <so file>       so file (default ./libfunc.so)");
    puts(" -f <request file>  request file(default NULL)");
    puts(" -k <boolean>       1 = keep alive, 0 = reconnect (default 1)");
    puts(" -q                 quiet. Just show QPS values");
    puts(" -l                 loop. Run the tests forever. For persistent test");
    puts(" -w                 whether define done if server close connection.(default 1)");
    puts(" -e                 try to handle EPIPE while server short connection.(default null)");
    puts(" -H                 show help information\n");
    exit(status);
}

/*
 * ��ȡrequest_file�ļ�������request_body
 */
static void read_quest_file()
{
    int fd;  
    int bytes_read;  
    char buffer[UNC_IOBUF_SIZE];
    
    if(!g_conf.request_body)
    {
        g_conf.request_body = unc_str_new_empty();
    }
    else
    {
        fprintf(stderr, "Some Thing Wrong!! Request body is not null\n");  
        exit(1);
    }
    
    //��Դ�ļ�
    if ((fd = open(g_conf.request_file, O_RDONLY)) == -1) 
    {  
        fprintf(stderr, "FATAL!! Open %s Failed. Error: %s\n", g_conf.request_file, strerror(errno));  
        exit(1);
    }  
  
    //һ������Ķ�ȡ�ļ��Ĵ���
    memset(buffer, 0, UNC_IOBUF_SIZE);
    while ((bytes_read = read(fd, buffer, UNC_IOBUF_SIZE))) 
    {  
        //����һ�������Ĵ���
        if ((bytes_read == -1) && (errno != EINTR))
        {
            fprintf(stderr, "FATAL!! Read file %s failed.Error: %s\n", g_conf.request_file, strerror(errno));
            exit(1);
        }
        else if (bytes_read > 0) 
        {  
            unc_str_cat(&(g_conf.request_body), buffer);
        }
        memset(buffer, 0, UNC_IOBUF_SIZE);
    }  
    close(fd);  
    return;    
}

/* 
 * ����ѹ������ 
 */
static void start_request(char *title, char *content) 
{
    if(!g_conf.title)
    {
        g_conf.title = title;
    }
    //����requests_issued��requests_finished
    g_conf.requests_sended = 0;
    g_conf.requests_done   = 0;
    g_conf.requests_finished = 0;

    if(!g_conf.request_body)
    {
        g_conf.request_body = unc_str_new(content);
    }
    
    /* ����ָ����Ŀ��client */
    create_multi_clients(g_conf.num_clients);

    g_conf.start = mstime();
    unc_ae_main_loop(g_conf.el);
    g_conf.total_latency = mstime() - g_conf.start;
    free_all_clients();
}

/* 
 * ����һ��client
 * ����: content,��������
 * ����: client_tָ��
 */
static client_t *create_one_client() 
{
    char buf[2048];

	//��ʼ��client_t
    client_t *c = (client_t *)malloc(sizeof(*c));
	//��������ʽ������server������
    c->fd = unc_anet_tcp_nonblock_connect(buf, g_conf.hostip, g_conf.hostport);
    if (c->fd == UNC_ERR) 
    {
        fprintf(stderr, "Connect to %s:%d failed.Detail:%s\n", g_conf.hostip, g_conf.hostport, buf);
        exit(1);
    }
    c->sendbuf = unc_str_dup(g_conf.request_body);
    c->recvbuf = unc_str_new_empty();
    c->written = 0;
    c->read = 0;
	
	//ע��д�¼�
    unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE, write_handler, c);
	//�������client����
    unc_dlist_add_node_tail(g_conf.clients, c);
	//client������
    ++g_conf.live_clients;
	
    return c;
}

/* 
 * �������client 
 * ����: num,     ����client�ĸ���
 *       content, ��������
 */
static void create_multi_clients(int num) 
{
    int n = 0;
	int i = 0;
	
    while (i++ < num)
    {
        create_one_client();
        /* listen backlog is quite limited on most system */
        if (++n > 64) 
        {
            usleep(50000);
            n = 0;
        }
    }
}

/* 
 * д�ص�����(client��������)
 */
static void write_handler(unc_ae_event_loop *el, int fd, void *priv, int mask) 
{
    client_t *c = (client_t *)priv;
    char *ptr;
    int nwritten;
    int first;

    /* Initialize request when nothing was written. */
    if (c->written == 0) 
    {   
        //if (g_conf.requests_issued++ >= g_conf.requests) 
        if (g_conf.requests_sended >= g_conf.requests) 
        {
            /*
               * ����ط�ԭ����������bug: 
               * 1. requests_sended++��λ��Ӧ�����棬��������write�ǿ��ܱ���ε��õģ�����requests_sendedԶԶ����requests
               * 2. ԭ������������ж���ͨ��requests_sended���У��⵼�³�������������Ϊ����client�������ڶ����������־�鿴����
               *    client����ǰclose�����ӣ�����ͳ��ڴ˴���Ӧ�÷���read�����ж��Ƿ��������
               * 3. һ�����ܵ�Ǳ�����⣬��read�����ж�������������᲻����Ϊĳ�������쳣δ��ɶ�������������ﲻ���˳�������?
               *    ������: ��!
               */
            //free_client(c);
            //unc_ae_stop(g_conf.el);
            return;
        }

        c->start = ustime();
        c->latency = -1;
    }

    if (c->sendbuf->len > c->written) 
    {
        ptr = c->sendbuf->buf + c->written;

        /* 
          * ��������EPIPE�����⣬ÿ������ĵ�һ��write��ʱ����Ҫ���EPIPE
          * 1. ������͵�����ֻ��һ���ֽڣ����޷���֡�
          * 2. ���������Mossad�ϲ��ԣ�û�����⣬������Apache�ϲ��ԣ�û��������⣡��Tcpdumpץ�����֣������������,
          *      Apache��Mossad���ᷢ��F����ʾ�ر����ӣ���ʱwrite���ݣ�Mossad������ȷ����RST������������unicorn��EPIPE
          *      ���󣬵��ǲ�֪��ΪʲôApache������RST����unicorn�޷���֪EPIPE���������ַ�������Apache��ʧЧ�ˡ�
          */
        first = 0;
        printf("%s\n", c->sendbuf->buf);
        if(g_conf.epipe && c->written == 0 && c->sendbuf->len > 1)
        {
            
            first = write(fd, ptr, 1); //дһ���ֽ�
        }
        nwritten = write(fd, ptr+first, c->sendbuf->len - c->written - first);
        if (nwritten == -1) 
        {
            fprintf(stderr, "write failed:%s\n", strerror(errno));
            //����TCP���ƣ��˴���EPIPE�����������޷�����������Ϊ��Ҫ����write���ܸ�֪��
            //TODO ��ʱδ�ҵ���֪EPIPE�����Ű취�����ǽ�sendbuf�������write����
            if (errno != EPIPE) 
            {
                fprintf(stderr, "write failed:%s\n", strerror(errno));
            }
            //free_client(c);
            client_done(c, SERVER_CLOSE_WHEN_WRITE);
            return;
        }
        c->written += (nwritten+first);

        if (c->sendbuf->len == c->written) 
        {
            g_conf.requests_sended++;
            /* ɾ��д�¼� */
            unc_ae_delete_file_event(g_conf.el, fd, UNC_AE_WRITABLE);
            /* �������¼� */
            unc_ae_create_file_event(g_conf.el, fd, UNC_AE_READABLE, read_handler, c);
        }
    }

}

/* 
 * ��ȡ�ص����� 
 */
static void read_handler(unc_ae_event_loop *el, int fd, void *priv, int mask) 
{
    client_t *c = (client_t *)priv;
    int nread, check = UNC_ERR;
    char buffer[UNC_IOBUF_SIZE];
    memset(buffer, 0, UNC_IOBUF_SIZE);

    //�ӷ���˶�ȡ���ݣ�������ȡ������append��recvbuf��
    nread = read(fd, buffer, UNC_IOBUF_SIZE-1);
    if (nread == -1) 
    {
        if (errno == EAGAIN) 
        {
            /* ��ʱ���ɶ�����ȡnonblock��fdʱ�����ܻ����� */
            return;
        }
        fprintf(stderr, "Read Error: %s\n", strerror(errno));
        exit(1);
    } 
    else if (nread == 0) 
    {

        //server�˹ر�����
        if (g_so.handle_server_close) g_so.handle_server_close(&g_conf, c, NULL);
        client_done(c, SERVER_CLOSE_WHEN_READ);//������������
        return;
    }
    buffer[nread] = '\0';
    c->read += nread;
    unc_str_cat(&(c->recvbuf), buffer); //append
    
    //�ж϶�ȡ���������Ƿ�����
    check = g_so.check_full_response(&g_conf, c, NULL);
    if(check == UNC_OK)
    {
        client_done(c, SERVER_NOT_CLOSE);
        return;
    }
    else if(check == UNC_NEEDMORE)
    {
        //fprintf(stdout, "Part response:%s\n", c->recvbuf->buf);
        return;
    }
    else
    {
       fprintf(stderr, "Something wrong in server!\n"); 
       exit(1);
    }
}

/* 
 * ����client,Ȼ������һ��д/������ 
 * 
 * һ��Ǳ�ڵĿ�:
 *   ���������ǳ����ӣ�û���κ����⣬���server�Ƕ����ӷ��񣬼�
 *   server���ص�����������check_full_response��������ok, ����������������ͷ������ӡ�
 *   ����������£�unicorn��ͳ�ƽ�ʧЧ�������·��������������١������ԭ����TCP��EPIPE���� ��
 *
 *   �����Ѿ��رյ�fd����һ�ε���write�ᵼ���յ�RST������ȴ�ǿ��Գɹ��ġ�Ҫ��������write���ܸ�֪��EPIPE���󣡣�����
 * 
 *   ��server�ر����ӵ�ʱ�����ڴ�ʱclientû��read(������������check_full_response)���޷���֪�ر����ӣ�
 *   ���ʱ�������reset_client,���¿�����write_handler��������write����Ϊ�ܹ��ɹ�(���Ǵ���Ҫ�������write)��
 *   ��ʱunicorn��Ϊ������һ��������request����ʵû�У���Ϊ������ѹر����ӣ����ղ���Ҳ����ȥ�յġ�Ȼ������
 *   read_handler���ܸ�֪�������close����ʱrequest_sended��Ȼ����һ�Ρ����յ��·�����յ��������������룡��
 *   
 *   һ��tricky����취��write_handler��������sendbuf������飬��Ϊ����write�����Ը�֪��EPIPE���󣬴�ʱ����
 *   ������sended����������free_client�����ǣ����ַ���ʵ��̫triky�����Ҳ����ر���:
 *   1. ������͵�����ֻ��һ���ֽڣ����޷���֡�
 *   2. ���������Mossad�ϲ��ԣ�û�����⣬������Apache�ϲ��ԣ�û��������⣡��Tcpdumpץ�����֣������������,
 *      Apache��Mossad���ᷢ��F����ʾ�ر����ӣ���ʱwrite���ݣ�Mossad������ȷ����RST������������unicorn��EPIPE
 *      ���󣬵��ǲ�֪��ΪʲôApache������RST����unicorn�޷���֪EPIPE���������ַ�������Apache��ʧЧ�ˡ�
 *   
 *   ��ʱû�뵽�ر����Ƶļ��ݰ취������һ��ķ������������������close������Apache������ܻ�����close��server
 *   Ҳ����Connection:closeͷ����֪client������⵽Connection:close����Ӧ����server�ر�������Ϊ�������صı�־��
 */
static void reset_client(client_t *c) 
{
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_READABLE);
    unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE, write_handler, c);
    
    unc_str_clear(c->recvbuf); //��ղ��ͷ�
    c->written = 0;
    c->read = 0;
}

/* 
 * ��client�����һ��д/������֮����� 
 * ����:
 *    @ client
 *    @ server_close��server���Ƿ�ر������� : 
 *      SERVER_NOT_CLOSE        -- ������û�йر�����
 *      SERVER_CLOSE_WHEN_READ  -- read��ʱ���ַ������Ͽ�������
 *      SERVER_CLOSE_WHEN_WRITE -- write��ʱ���ַ������Ͽ�������( EPIPE )
 *
 */
static void client_done(client_t *c, int server_close) 
{
	int num;

    //�����û�йر����� || read����˹ر����ӵ���done_if_srv_close��1���������++�����Ҽ�¼����˷���
    if(server_close == SERVER_NOT_CLOSE 
       || (server_close == SERVER_CLOSE_WHEN_READ && g_conf.done_if_srv_close))
    {
        c->latency = ustime() - c->start;
        ++g_conf.requests_done;
        if(!g_conf.response.is_get)
        {
            g_conf.response.is_get = 1;
            g_conf.response.res_body = unc_str_dup(c->recvbuf);
        }
    }

    //д��ʱ���ַ����close��������������
    if(server_close != SERVER_CLOSE_WHEN_WRITE)
    {
        ++g_conf.requests_finished;   
    }
    //����ﵽ��Ԥ���������������ֹͣ���ù����requests_done��֤�����ܹ�����
    if (g_conf.requests_finished == g_conf.requests) 
    {
        //free_client(c);
        unc_ae_stop(g_conf.el); //ȫ��Aeֱ��ֹͣ
        return;
    }

    // ���keep_alive��server��û�йر����ӣ������¿�ʼclient��д/������
    // �����ͷ�client��Ȼ������client,Ȼ��ʼд/������
    if (g_conf.keep_alive && server_close == SERVER_NOT_CLOSE) 
    {
        reset_client(c);
    }
    else 
    {
        //���ͷŵ�ǰclient���ڲ�live_clients���Լ�
        free_client(c); 

        //����
        num = g_conf.num_clients - g_conf.live_clients;
        if(num > 0)
        {
            create_multi_clients(num);
        }
    }
}

/* 
 * �ͷ�һ��client 
 */
static void free_client(client_t *c) 
{
    unc_dlist_node_t *node;
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_READABLE);
    close(c->fd);
    unc_str_free(c->sendbuf);
    unc_str_free(c->recvbuf);
    --g_conf.live_clients;
    node = unc_dlist_search_key(g_conf.clients, c);
    assert(node != NULL);
    unc_dlist_delete_node(g_conf.clients, node);
    free(c);
}

/* 
 * �ͷ�ȫ��client 
 */
static void free_all_clients() {
    unc_dlist_node_t *node = g_conf.clients->head;
    unc_dlist_node_t *next = NULL;
    while (node) 
    {
        next = node->next;
        free_client((client_t *)node->value);
        node = next;
    }
}

/* 
 * ��ӡ���ղ��Ա��� 
 */
static void show_final_report(void) 
{
    float qps;
    char buf[11]; int idx=0, r; memset(buf, ' ', 11);

    qps = (float)g_conf.requests_done / ((float)g_conf.total_latency / 1000);
    r = (int)qps;
    
    if (!g_conf.quiet) 
    {
        fprintf(stdout, "============== %s REPORT ==============\n", g_conf.title);
        fprintf(stdout, "*   All requests           : %-10d               *\n", g_conf.requests);  
        fprintf(stdout, "*   All requests sended    : %-10d               *\n", g_conf.requests_sended);        
        fprintf(stdout, "*   All requests completed : %-10d               *\n", g_conf.requests_done);
        //���done_if_srv_close����done��finished��ͬ������ֻ�з�done_if_srv_closeʱ��ӡ
        if(!g_conf.done_if_srv_close) fprintf(stdout, "*   All requests finished  : %-10d               *\n", g_conf.requests_finished);
        fprintf(stdout, "*   Use time of seconds    : %-10.2f               *\n", (float)g_conf.total_latency/1000);
        fprintf(stdout, "*   Parallel clients       : %-10d               *\n", g_conf.num_clients);
        fprintf(stdout, "*   Keep alive             : %-10d               *\n", g_conf.keep_alive);
        fprintf(stdout, "*                                                     *\n");
        fprintf(stdout, "*   Complete rate          : %-6.2f %%                 *\n", 100*((float)g_conf.requests_done/(float)g_conf.requests));
        while(r>0) { idx++; r /= 10; } idx = 10-idx; buf[idx] = '\0'; //�������һ�ж���
        fprintf(stdout, "*   Average QPS            : %.2f r/s        %s*\n", qps, buf);
        fprintf(stdout, "============ %s REPORT END ============\n\n", g_conf.title);
    } 
    else 
    {
        fprintf(stdout, "%s:%.2f requests per second\n\n", g_conf.title, qps);
    }
}

int main(int argc, char **argv) 
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);  //����SIGPIPE�źţ�ֻ���ں��Ե�����£�����write�رյ�fd���Ż�õ�EPIPE����

	//ȫ�־��Ĭ��ֵ��ʼ��
	init_conf();

	//��������
    parse_options(argc, argv);
    if (optind < argc) 
    {
        usage(1);
    }

    //��̬�����
    if (unc_load_so(&g_handle, syms, g_conf.so_file) < 0) 
    {
        fprintf(stderr, "Load file %s failed\n", g_conf.so_file);
        exit(1);
    }
  
    if (g_so.handle_init) g_so.handle_init(&g_conf, NULL);
    
    if (!g_conf.keep_alive) 
    {
        puts("WARNING:\n"
            " keepalive disabled, at linux, you probably need    \n"
            " 'echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse' and \n"
            " 'sudo sysctl -w net.inet.tcp.msl=1000' for Mac OS X\n"
            " in order to use a lot of clients/requests\n");
    }

    //�������request_file�����Զ�ȡ����request body
    if(g_conf.request_file)
    {
        read_quest_file();
    }
    
    //�����û����ӣ���request body����һ������
    if (g_so.generate_request) 
    {
        if(g_so.generate_request(&g_conf, NULL) != UNC_OK)
        {
            fprintf(stderr, "Generate request failed\n");
            exit(1);
        }
    }

    do {
        if (g_so.request_pre) g_so.request_pre(&g_conf, NULL);
        start_request("UNICORN CLIENT", "Hello World!");
        //TODO ÿ�ֲ�����Ϻ�ı���
        if (g_so.request_post) g_so.request_post(&g_conf, NULL);
    } while (g_conf.loop);

    if (g_so.handle_finish) g_so.handle_finish(&g_conf, NULL);

    //��ӡ���Ա���
    show_final_report();

    //�ͷŶ�̬��
    unc_unload_so(&g_handle);
    
    //TODO �ͷ�g_conf
    return 0;
}
