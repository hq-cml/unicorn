/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_anet.c 
 * 
 * Description :  Unc_anet, Basic TCP socket stuff. Derived from redis.
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ  
 *
 **/
#include "unc_core.h"

/**
 * ����: ��ʽ��err��������������
 * ����: @err, @fmt
 **/
static void unc_anet_set_error(char *err, const char *fmt, ...) 
{
    va_list ap;
    if (!err) 
    {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(err, UNC_ANET_ERR_LEN, fmt, ap);
    va_end(ap);
}

/**
 * ����: ����TCP socket
 * ����: @err, ��������Ļ���
 *       @domain, Э����
 * ����: �ɹ��������ɵ�fd��ʧ�ܣ�-x
 **/
static int unc_anet_create_socket(char *err, int domain) 
{
    int s, on = 1;
    int flags;
    if ((s = socket(domain, SOCK_STREAM, 0)) == -1) 
    {
        unc_anet_set_error(err, "Create socket failed: %s", strerror(errno));
        return UNC_ERR;
    }

    /* �����׽ӿں�һ������ʹ���еĵ�ַ���� */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) 
    {
        unc_anet_set_error(err, "Setsockopt SO_REUSEADDR: %s", strerror(errno));
        return UNC_ERR;
    }
    
    /* ����CLOEXEC����execʱ�ͷŴ�fd */
    if ((flags = fcntl(s, F_GETFD)) < 0)
    {
        unc_anet_set_error(err, "Get fcntl failed: %s", strerror(errno));
        return UNC_ERR;        
    }
    else
    {
        if(fcntl(s, F_SETFD, flags | FD_CLOEXEC)<0)
        {
            unc_anet_set_error(err, "Set close-on-exec failed: %s", strerror(errno));
            return UNC_ERR;            
        }
    }
    
    return s;
}

/**
 * ����: ��bind��listen
 * ����: @err,@sockfd,@sa,��ַ�ṹ,@len,sa����
 * ����: �ɹ���ʧ�ܣ�
 **/
static int unc_anet_bind_listen(char *err, int sockfd, struct sockaddr *sa, socklen_t len) 
{
    if (bind(sockfd, sa, len) == -1) 
    {
        unc_anet_set_error(err, "bind failed: %s", strerror(errno));
        close(sockfd);
        return UNC_ERR;
    }

    /* The magic 511 constant is from nginx. 
     * Use a backlog of 512 entries. We pass 511 to the listen() call because
     * the kernel does: backlogsize = roundup_pow_of_two(backlogsize + 1);
     * which will thus give us a backlog of 512 entries */
    if (listen(sockfd, 511) == -1) 
    {
        unc_anet_set_error(err, "listen failed:%s", strerror(errno));
        close(sockfd);
        return UNC_ERR;
    }
    return UNC_OK;
}

/**
 * ����: Set the socket nonblocking.
 * ����: @err, @fd
 * ����:
 *      1. Note that fcntl(2) for F_GETFL and F_SETFL can't be interrupted by a signal.
 *      2. ���ڷ�������fd����accept���������˳�����������һ���������������I/O���ü���
 *      3. ����accept�ļ���fd���������Ƿ�������Ч��û�в��죬��Ϊ�˷�ֹ�쳣���������
 *         select�����ˣ��ڵ���accept֮ǰ��������ӱ��쳣��ֹ����ʱ accept ���ÿ��ܻ���
 *         ��û������ɵ����Ӷ�������ֱ���������ӽ�����
 *      4. ����accept���ص�fd����Ӧ������Ϊ����������ֹ�����߳�������ĳ��I/O��������ʹ
 *         ʹ����I/O���ã�Ҳ��Ҫ����������Ϊ��I/O����ֻ��˵�� socket �ɶ����߿�д������
 *         ˵���ܶ��������д���������ݡ����磬socket ��д��������10���ֽڵĿ��пռ䣬
 *         ��ʱ���ӵ� select ���أ�Ȼ���ڸ� socket �Ͻ���д�������������Ҫд��100�ֽڣ�
 *         ��� socket û�����÷����������� write �ͻ�����������ڶ�� socket �������
 *         ����дһ��socket ʱ��������Ӱ�쵽������ socket ��
 * ����: �ɹ���0��ʧ�ܣ�-x
 **/
int unc_anet_nonblock(char *err, int fd) 
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) 
    {
        unc_anet_set_error(err, "fcntl(F_GETFL) failed: %s", strerror(errno));
        return UNC_ERR;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        unc_anet_set_error(err, "fcntl(F_SETFL, O_NONBLOCK) failed: %s", strerror(errno));
        return UNC_ERR;
    }
    return UNC_OK;
}

/**
 * ����: ����no_delay
 * ����: @err, @fd
 * ����:
 *      1. �رս�ֹNagle�㷨���������������������������Ӱ��ϴ�
 * ����: �ɹ���0��ʧ�ܣ�-x
 **/
int unc_anet_tcp_nodelay(char *err, int fd) 
{
    int yes = 1;

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1) 
    {
        unc_anet_set_error(err, "setsockopt TCP_NODELAY failed: %s", strerror(errno));
        return UNC_ERR;
    }
    return UNC_OK;
}

/**
 * ����: ���û�������С
 * ����: @err, @fd, @send_buffsize, @recv_buffsize
 * ����:
 *      1. ��֧�ֽ�����������Ϊ0
 *      2. ��������fd��û��Ҫ���û����������ڸ��Ѵ��ڵ�����fd���������û�����
 * ����: �ɹ���0,ʧ�ܣ�-x
 **/
int unc_anet_set_buffer(char *err, int fd, int send_buffsize, int recv_buffsize) 
{
    if(!send_buffsize)
    {
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &send_buffsize, sizeof(send_buffsize)) == -1) 
        {
            unc_anet_set_error(err, "setsockopt SO_SNDBUF failed: %s", strerror(errno));
            return UNC_ERR;
        }
    }
    
    if(!recv_buffsize)
    {
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &recv_buffsize, sizeof(recv_buffsize)) == -1) 
        {
            unc_anet_set_error(err, "setsockopt SO_RCVBUF failed: %s", strerror(errno));
            return UNC_ERR;
        }
    }
    return UNC_OK;
}

/**
 * ����: ����keepalive
 * ����: @err, @fd
 * ����:
 *      1. 
 * ����: �ɹ���ʧ�ܣ�
 **/
int unc_anet_tcp_keepalive(char *err, int fd) 
{
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) 
    {
        unc_anet_set_error(err, "setsockopt SO_KEEPALIVE failed: %s", strerror(errno));
        return UNC_ERR;
    }
    return UNC_OK;
}

/**
 * ����: ����TCP������: create->bind->listen
 * ����: @err, @addr, @port
 * ����:
 *      1. ���addrΪ�գ����ȫ����ַ
 * ����: �ɹ���sockfd, ʧ�ܣ�-x
 **/
int unc_anet_tcp_server(char *err, char *addr, int port) 
{
    int sockfd;
    struct sockaddr_in sa;
    
    /* ����AF_INET */
    if ((sockfd = unc_anet_create_socket(err, AF_INET)) == UNC_ERR) 
    {
        return UNC_ERR;
    }
     
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY); /* ���������� */
    
    /* ���addr�ǿգ���bindָ����ip��ַ */
    if (addr && inet_aton(addr, &sa.sin_addr) == 0) 
    {
        unc_anet_set_error(err, "invalid bind address");
        close(sockfd);
        return UNC_ERR;
    }

    if (unc_anet_bind_listen(err, sockfd, (struct sockaddr*)&sa, sizeof(sa)) == UNC_ERR) 
    {
        return UNC_ERR;
    }
    return sockfd;
}

/**
 * ����: ͨ��accept
 * ����: @err, sockfd, sa, len
 * ����:
 *      1. sa/len--ֵ/������� 
 *      2. ���������У������ź��жϣ������
 * ����: �ɹ����µ�fd��ʧ�ܣ�-x
 **/
static int unc_anet_generic_accept(char *err, int sockfd, struct sockaddr *sa, socklen_t *len) 
{
    int fd;
    while (1) 
    {
        fd = accept(sockfd, sa, len);
        if (fd < 0) 
        {
            /* ���������У������ź��жϣ������ */
            if (errno == EINTR) 
            {
                continue;
            } 
            else 
            {
                unc_anet_set_error(err, "accept failed:%s, errno:%d, fd:%d", strerror(errno),errno, fd);
                return UNC_ERR;
            }
        }
        break;
    }
    return fd;
}

/**
 * ����: Tcp accept
 * ����: @err, sockfd, ip, port
 * ����:
 *      1. ��ip��port���������ָ����ַ
 * ����: �ɹ�����fd, ʧ�ܣ�-x
 **/
int unc_anet_tcp_accept(char *err, int sockfd, char *ip, int *port) 
{
    int fd;
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    if ((fd = unc_anet_generic_accept(err, sockfd, (struct sockaddr*)&sa, &salen)) == UNC_ERR) 
    {
        return UNC_ERR;
    }

    /* Set the ip and port*/
    if (ip) 
    {
        strcpy(ip, inet_ntoa(sa.sin_addr));
    }
    if (port) 
    {
        *port = ntohs(sa.sin_port);
    }
    return fd;
}

/**
 * ����: ��fd��ȡcount���ֽڣ��洢��buf
 * ����: @
 * ����:
 *      1. �������EOF�����߳����򷵻�
 *      2. һֱ��ȡ��ֱ��������count�ֽ�
 * ����: �ɹ���һ����ȡ���ַ�����ʧ�ܣ�-x
 **/
int unc_anet_read(int fd, char *buf, int count) 
{
    int nread, totlen = 0;
    while (totlen < count) 
    {
        nread = read(fd, buf, count - totlen);
        if (nread == 0) 
        {
            return totlen; /* EOF */
        }

        if (nread < 0) 
        {
            return UNC_ERR;
        }
        totlen += nread;
        buf += nread;
    }
    return totlen;
}

/**
 * ����: ��fdд��count���ֽ�
 * ����: @
 * ����:
 *      1. ��������������˳�
 *      2. һֱд��ֱ��д����count���ַ�
 * ����: �ɹ���ʧ�ܣ�
 **/
int unc_anet_write(int fd, char *buf, int count) 
{
    int nwritten, totlen = 0;
    while (totlen < count) 
    {
        nwritten = write(fd, buf, count - totlen);
        if (nwritten == 0) 
        {
            return totlen;
        }
        if (nwritten < 0) 
        {
            return UNC_ERR;
        }
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

/**
 * ����: resolve
 * ����: @err, host, ipbuf
 * ����:
 *      1. �Բ���host����dns���������������ipbuf
 * ����: �ɹ���0, ʧ�ܣ�-x
 **/
int unc_anet_resolve(char *err, char *host, char *ipbuf) 
{
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    if (inet_aton(host, &sa.sin_addr) == 0) 
    {
        struct hostent *he;
        he = gethostbyname(host);
        if (he == NULL) 
        {
            unc_anet_set_error(err, "resolve %s failed: %s", host, hstrerror(h_errno));
            return UNC_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }
    strcpy(ipbuf, inet_ntoa(sa.sin_addr));
    return UNC_OK;
}

/**
 * ����: ����fd��ȡ���Զ���Ϣ�����������ָ����ַ
 * ����: @err, fd, ip, port
 * ����: �ɹ���ʧ�ܣ�
 **/
int unc_anet_peer_tostring(char *err, int fd, char *ip, int *port) 
{
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);

    if (getpeername(fd, (struct sockaddr*)&sa, &salen) == -1) 
    {
        unc_anet_set_error(err, "getpeername failed:%s", strerror(errno));
        return UNC_ERR;
    }

    if (ip) 
    {
        strcpy(ip, inet_ntoa(sa.sin_addr));
    }

    if (port) 
    {
        *port = ntohs(sa.sin_port);
    }
    return UNC_OK;
}

/**
 * ����: ͨ������
 * ����: @err, addr, port, flags
 * ˵��: Ĭ���Ƿ�������
 * ����: �ɹ����µ�fd��ʧ�ܣ�-x
 **/
static int unc_anet_tcp_generic_connect(char *err, char *addr, int port, int flags) 
{
    int sockfd;
    struct sockaddr_in sa;

    if ((sockfd = unc_anet_create_socket(err, AF_INET)) == UNC_ERR) 
    {
        //UNC_ERROR_LOG("Unc_anet_create_socket failed:%s", err);
        return UNC_ERR;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_aton(addr, &sa.sin_addr) == 0) 
    {
        struct hostent *he;
        he = gethostbyname(addr);
        if (he == NULL) 
        {
            unc_anet_set_error(err, "resolve %s failed: %s", addr, hstrerror(h_errno));
            //UNC_ERROR_LOG("Gethostbyname failed:%s", err);
            close(sockfd);
            return UNC_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }

    if (flags & UNC_ANET_CONNECT_NONBLOCK) 
    {
        if (unc_anet_nonblock(err, sockfd) != UNC_OK) 
        {
            return UNC_ERR;
        }
    }

    /* ���Է������ķ�ʽ���������ӵ�ʱ�򣬷��صĽ������� -1,�Ⲣ�������������
     * �����˴���������ķ��ؽ���� EINPROGRESS����ô�ʹ������ӻ��ڽ����С� 
     * �������ͨ��poll����select���ж�socket�Ƿ��д���������д��˵�����������
     **/
    if (connect(sockfd, (struct sockaddr*)&sa, sizeof(sa)) == -1) 
    {
        if (errno == EINPROGRESS && flags & UNC_ANET_CONNECT_NONBLOCK) {
            return sockfd;
        }
        unc_anet_set_error(err, "connect %s failed: %s", addr, strerror(errno));
        //UNC_ERROR_LOG("Connect failed:%s", err);
        close(sockfd);
        return UNC_ERR;
    }
    return sockfd;
}

/**
 * ����: ��������
 **/
int unc_anet_tcp_connect(char *err, char *addr, int port) 
{
    return unc_anet_tcp_generic_connect(err, addr, port, UNC_ANET_CONNECT);
}

/**
 * ����: ����������
 * ˵����
 *       TCP socket ����Ϊ����������� connect ��connect�������������� EINPROCESS���� 
 *       TCP �� 3 �����ּ������С�֮������� select ��� �����Ƿ����ɹ���
 *       ������ connect ��3 ����;��
 *          1. ��3 �����ֵ�ͬʱ��һЩ�����Ĵ���
 *          2. ����ͬʱ����������ӡ�
 *����������3. ������ select �ȴ���ʱ�򣬿��Ը� select �趨һ��ʱ�䣬
 *             �Ӷ��������� connect �ĳ�ʱʱ�䡣
 **/
int unc_anet_tcp_nonblock_connect(char *err, char *addr, int port) 
{
    return unc_anet_tcp_generic_connect(err, addr, port,UNC_ANET_CONNECT_NONBLOCK);
}

#ifdef __UNC_ANET_TEST_MAIN__
int main(void) 
{
    fd_set rfds;
    char error[UNC_ANET_ERR_LEN];
    char msg[] = "hello,world\r\n";
    char ip[INET_ADDRSTRLEN];
    struct timeval timeout;
    int ready;
    int port;
    int fd;
    int sock;

    if (unc_anet_resolve(error, "localhost", ip) == UNC_ERR) 
    {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }
    printf("LOCAL:%s\n", ip);

    if ((fd = unc_anet_tcp_server(error, ip, 8888)) == UNC_ERR) 
    {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }
    
    unc_anet_tcp_nodelay(error, fd);
    /* ����accpet�Ķ���Ҳ�б�Ҫ���÷����� */
    unc_anet_nonblock(error, fd);
    
    /*
    //����NONBLOCK��NODELAY
    unc_anet_nonblock(error, fd);
    unc_anet_tcp_nodelay(error, fd);
    if((sock = unc_anet_tcp_accept(error, fd, ip, &port)) != UNC_ERR)
    {
        printf("accept  =>%s:%d\n", ip, port);
        write(sock, msg, strlen(msg));
        close(sock);  
    }
    else
    {
        fprintf(stderr, "%s\n", error);
    }    
    exit(0);
    */
    
    while (1) 
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        if ((ready = select(fd + 1, &rfds, NULL, NULL, &timeout)) == -1 && errno != EINTR) 
        {
            perror("select");
            exit(1);
        }
        
        if (ready > 0) 
        {
            if (FD_ISSET(fd, &rfds)) 
            {
                if((sock = unc_anet_tcp_accept(error, fd, ip, &port)) != UNC_ERR)
                {
                    printf("accept  =>%s:%d\n", ip, port);
                    
                    if (unc_anet_peer_tostring(error, sock, ip, &port) == UNC_ERR) 
                    {
                        fprintf(stderr, "%s\n", error);
                        continue;
                    }
                    printf("get peer=>%s:%d\n\n", ip, port);
                    unc_anet_read(sock, msg, 5);
                    unc_anet_write(sock, msg, strlen(msg));
                    close(sock);                    
                }

            }
        }
    }
    exit(0);
}
#endif /* __UNC_ANET_TEST_MAIN__ */

