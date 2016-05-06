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
static void client_done(client_t *c);
static int show_qps(unc_ae_event_loop *el, long long id, void *priv);
static void usage(int status);
static void write_handler(unc_ae_event_loop *el, int fd, void *priv, int mask);
static void create_multi_clients(int num, char *content);
static void read_handler(unc_ae_event_loop *el, int fd, void *priv, int mask);
static void free_all_clients();
static void show_final_report(void);
static void free_client(client_t *c);




//ȫ�־������
static conf_t g_conf;

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
	g_conf.num_clients = 1;
	g_conf.requests = 1;
	g_conf.live_clients = 0;
	g_conf.keep_alive = 1;
	g_conf.loop = 0;
	g_conf.quiet = 0;
	g_conf.el = unc_ae_create_event_loop();
	unc_ae_create_time_event(g_conf.el, 3000, show_qps, NULL, NULL);
	g_conf.clients = unc_dlist_init();
	g_conf.hostip = "127.0.0.1";
	g_conf.hostport = 9527;
	
	return;
}

/* 
 * ʱ���¼�: ��ӡ��ǰ��QPS(�ӿ�ʼ����ǰ���ۼ�ֵ) 
 */
static int show_qps(unc_ae_event_loop *el, long long id, void *priv) 
{
    float dt = (float)(mstime() - g_conf.start) / 1000.0;
    float rps = (float)g_conf.requests_finished / dt;
    printf("%s: %.2f\n", g_conf.title, rps);
    return 3000; /* every 3000ms */
}

/*
 * ��������
 */
static void parse_options(int argc, char **argv) 
{
    char c;

    while ((c = getopt(argc, argv, "h:p:c:n:k:qlH")) != -1) 
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
        case 'k':
            g_conf.keep_alive = atoi(optarg);
            break;
        case 'q':
            g_conf.quiet = 1;
            break;
        case 'l':
            g_conf.loop = 1;
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
    puts("Usage: benchmark [-h <host>] [-p <port>] "
            "[-c <clients>] [-n requests]> [-k <boolean>]\n");
    puts(" -h <hostname>    server hostname (default 127.0.0.1)");
    puts(" -p <port>        server port (default 9527)");
    puts(" -c <clients>     number of parallel connections (default 1)");
    puts(" -n <requests>    total number of requests (default 1)");    
    puts(" -k <boolean>     1 = keep alive, 0 = reconnect (default 1)");
    puts(" -q               quiet. Just show QPS values");
    puts(" -l               loop. Run the tests forever");
    puts(" -H               show help information\n");
    exit(status);
}

/* 
 * ����ѹ������ 
 */
static void start(char *title, char *content) 
{
    g_conf.title = title;
    g_conf.requests_issued = 0;
    g_conf.requests_finished = 0;

    /* ����ָ����Ŀ��client */
    create_multi_clients(g_conf.num_clients, content);

    g_conf.start = mstime();
    unc_ae_main_loop(g_conf.el);
    g_conf.total_latency = mstime() - g_conf.start;

    show_final_report();
    free_all_clients();
}

/* 
 * ����һ��client
 * ����: content,��������
 * ����: client_tָ��
 */
static client_t *create_one_client(const char *content) 
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
    c->obuf = unc_str_new(content);
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
static void create_multi_clients(int num, char *content) 
{
    int n = 0;
	int i = 0;
	
    while (i++ < num)
    {
        create_one_client((const char *)content);
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
 * һֱд(ͨ��Ae)��ֱ������д��(client->obuf)ȫ��д���ˣ�������read�ص�
 */
static void write_handler(unc_ae_event_loop *el, int fd, void *priv, int mask) 
{
    client_t *c = (client_t *)priv;

    /* Initialize request when nothing was written. */
    if (c->written == 0) 
    {
        if (g_conf.requests_issued++ >= g_conf.requests) 
        {
            free_client(c);
            unc_ae_stop(g_conf.el);
            return;
        }

        c->start = ustime();
        c->latency = -1;
    }

    if (c->obuf->len > c->written) 
    {
        char *ptr = c->obuf->buf + c->written;
        int nwritten = write(c->fd, ptr, c->obuf->len - c->written);
        if (nwritten == -1) 
        {
            /* When a process writes to a socket that has received an RST, the SIGPIPE signal is sent to the process. */
            if (errno != EPIPE) 
            {
                fprintf(stderr, "write failed:%s\n", strerror(errno));
            }
            free_client(c);
            return;
        }
        c->written += nwritten;

        if (c->obuf->len == c->written) 
        {
            /* ɾ��д�¼� */
            unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
            /* �������¼� */
            unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_READABLE, read_handler, c);
        }
    }

}

/* 
 * ��ȡ�ص����� 
 */
static void read_handler(unc_ae_event_loop *el, int fd, void *priv, int mask) 
{
    client_t *c = (client_t *)priv;
    int nread;
    char buffer[4096];
    memset(buffer, 0, 4096);
    nread = read(fd, buffer, 4096);
    if (nread == -1) 
    {
        if (errno == EAGAIN) 
        {
            /* ��ʱ���ɶ�����ȡnonblock��fdʱ�����ܻ����� */
            return;
        }
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    } 
    else if (nread == 0) 
    {
        fprintf(stderr, "Error: %s\n", "Server close connection.");
        exit(1);
    }
    c->read += nread;
    /* ����ȡ�������ݣ���д��ȥ���������ʱ�����������һ������ */
    if (c->read == c->obuf->len && 0==memcmp(buffer, c->obuf->buf, c->obuf->len)) 
    {
        c->latency = ustime() - c->start;
        ++g_conf.requests_finished;
    }
    client_done(c);
}

/* 
 * ����client,Ȼ������һ��д/������ 
 */
static void reset_client(client_t *c) 
{
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE);
    unc_ae_delete_file_event(g_conf.el, c->fd, UNC_AE_READABLE);
    unc_ae_create_file_event(g_conf.el, c->fd, UNC_AE_WRITABLE, write_handler, c);
    c->written = 0;
    c->read = 0;
}

/* 
 * ��client�����һ��д/������֮����� 
 */
static void client_done(client_t *c) 
{
	unc_str_t *pcontent;
	int num;
    // ����ﵽ��Ԥ���������������ֹͣ
    if (g_conf.requests_finished == g_conf.requests) 
    {
        free_client(c);
        unc_ae_stop(g_conf.el); //ȫ��Aeֱ��ֹͣ
        return;
    }

    // ���keep_alive�������¿�ʼclient��д/������
    // ���!keep_alive,������client,Ȼ��ʼд/������
    if (g_conf.keep_alive) 
    {
        reset_client(c);
    } 
    else 
    {
        pcontent = unc_str_new(c->obuf->buf);
    	//���ͷŵ�ǰclient���ڲ�live_clients���Լ�
		free_client(c); 

		//����
		num = g_conf.num_clients - g_conf.live_clients;
		if(num > 0)
		{
			create_multi_clients(num, pcontent->buf);
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
    unc_str_free(c->obuf);
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
    float reqpersec;

    /* ����ÿ�봦�������--QPS */
    reqpersec = (float)g_conf.requests_finished / ((float)g_conf.total_latency / 1000);

    if (!g_conf.quiet) 
    {
        printf("========== %s ==========\n", g_conf.title);
        printf(" All %d requests has send\n", g_conf.requests);        
        printf(" All %d requests completed\n", g_conf.requests_finished);
        printf(" Complete:%.8f%%\n", 100*((float)g_conf.requests_finished/(float)g_conf.requests));
        printf(" Use time %.2f seconds\n", (float)g_conf.total_latency/1000);
        printf(" Parallel %d clients\n", g_conf.num_clients);
        printf(" keep alive: %d\n", g_conf.keep_alive);
        printf("\n");

        printf("%.2f requests per second\n\n", reqpersec);
    } 
    else 
    {
        printf("%s:%.2f requests per second\n\n", g_conf.title, reqpersec);
    }
}

int main(int argc, char **argv) 
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

	//ȫ�־��Ĭ��ֵ��ʼ��
	init_conf();

	//��������
    parse_options(argc, argv);
    
    if (optind < argc) 
    {
        usage(1);
    }

    if (!g_conf.keep_alive) 
    {
        puts("WARNING:\n"
            " keepalive disabled, at linux, you probably need    \n"
            " 'echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse' and \n"
            " 'sudo sysctl -w net.inet.tcp.msl=1000' for Mac OS X\n"
            " in order to use a lot of clients/requests\n");
    }

    do {
        /* ѹ��Echo */
        start("Unicorn QPS benchmark", "hello mossad");
        /* ѹ��tosser */
        //char buf[256] = {0};
        //snprintf(buf, 256, "0000000106{\"type\": \"0\", \"host\": \"hq_test\", \"data\": [[\"999\", \"999\", \"1\"], [\"888\", \"888\", \"1\"]], \"time\": \"%d\"}", (int)time(NULL));
        //printf("strlen:%d, %s\n", strlen(buf), buf);
        //benchmark("Mossad QPS benchmark", buf);
    } while (g_conf.loop);
     return 0;
}
