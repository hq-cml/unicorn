/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_ae.c 
 * 
 * Description :  A simple event-driven programming library. It's from Redis.
 *                The default multiplexing layer is select. If the system 
 *                support epoll, you can define the epoll macro.
 *
 *                #define Unc_EPOLL_MODE
 *
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/
#include "unc_core.h"

/**
 * ����: ����ae_loop->api_data�Ľṹ������ʼ��֮
 * ����: @el, ae_loopָ��
 * ע��:
 *      1. epoll��֧���ļ���������������ֻ���������磬�����Ҫ�����ļ�
 *         ����Ҫʹ��select�������ԣ����epoll�����ļ����ᱨ��EPERM
 *         (perror:Operation not permitted)
 * ����: �ɹ���0 ʧ�ܣ�NULL
 **/
#ifdef UNC_EPOLL_MODE

static int unc_ae_api_create(unc_ae_event_loop *el) 
{
    unc_ae_api_state *state = malloc(sizeof(*state));
    if (!state) 
    {
        return UNC_ERR;
    }

    state->epfd = epoll_create(UNC_AE_SETSIZE);
    if (state->epfd == -1) 
    {
        return UNC_ERR;
    }
    el->api_data = state;
    return 0;
}
#elif defined UNC_SELECT_MODE

static int unc_ae_api_create(unc_ae_event_loop *el) 
{
    unc_ae_api_state *state = malloc(sizeof(*state));
    if (!state) 
    {
        return -1;
    }
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    el->api_data = state;
    return UNC_OK;
}
#endif

/**
 * ����: ����ae_loop�ṹ�壬����ʼ��֮
 * ����: �ɹ���ae_lopp�ṹ��ָ��, ʧ�ܣ�NULL
 **/
unc_ae_event_loop *unc_ae_create_event_loop(void) 
{
    unc_ae_event_loop *el;
    int i;

    el = (unc_ae_event_loop*)malloc(sizeof(*el));
    if (!el) 
        return NULL;
    
    el->last_time       = time(NULL);
    el->time_event_head = NULL;
    el->time_event_next_id = 0;
    el->stop = 0;
    el->maxfd = -1;
    el->before_sleep = NULL;
    if (unc_ae_api_create(el) != UNC_OK) 
    {
        free(el);
        return NULL;
    }

    /* Events with mask == UNC_AE_NONE are not set. 
       So let's initialize the vector with it. */
    for (i = 0; i < UNC_AE_SETSIZE; ++i) 
    {
        el->events[i].mask = UNC_AE_NONE;
    }
    return el;
}

/**
 * ����: ����api_data
 * ����: @el
 **/
#ifdef UNC_EPOLL_MODE

static void unc_ae_api_free(unc_ae_event_loop *el) 
{
    unc_ae_api_state *state = el->api_data;
    close(state->epfd);
    free(state);
}
#elif defined UNC_SELECT_MODE

static void unc_ae_api_free(unc_ae_event_loop *el) 
{
    if (el && el->api_data) 
    {
        free(el->api_data);
    }
}
#endif

/**
 * ����: ����ae_loop�ṹ
 * ����: @el
 **/
void unc_ae_free_event_loop(unc_ae_event_loop *el) 
{
    unc_ae_time_event *te, *next;
    /* ����api_data */
    unc_ae_api_free(el);

    /* Delete all time event to avoid memory leak. */
    te = el->time_event_head;
    while (te) 
    {
        next = te->next;
        if (te->finalizer_proc) 
        {
            te->finalizer_proc(el, te->client_data);
        }
        free(te);
        te = next;
    }

    free(el);
}

/**
 * ����: ae_loop��ѵ��ͣ
 **/
void unc_ae_stop(unc_ae_event_loop *el) 
{
    el->stop = 1;
}

/**
 * ����: add event
 * ����: �ɹ���0 ʧ�ܣ�-x
 **/
#ifdef UNC_EPOLL_MODE

static int unc_ae_api_add_event(unc_ae_event_loop *el, int fd, int mask) 
{
    unc_ae_api_state *state = el->api_data;
    struct epoll_event ee;
    /* If the fd was already monitored for some event, we need a MOD
       operation. Otherwise we need an ADD operation. */
    int op = el->events[fd].mask == UNC_AE_NONE ? 
        EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    ee.events = 0;
    mask |= el->events[fd].mask; /* Merge old events. */
    /* ˮƽ���� */
    if (mask & UNC_AE_READABLE)
    {
        ee.events |= EPOLLIN;
    }

    if (mask & UNC_AE_WRITABLE) 
    {
        ee.events |= EPOLLOUT;
    }
    ee.data.u64 = 0;
    ee.data.fd = fd;
    if (epoll_ctl(state->epfd, op, fd, &ee) == -1) 
    {
        return UNC_ERR;
    }
    return UNC_OK;
}
#elif defined UNC_SELECT_MODE

static int unc_ae_api_add_event(unc_ae_event_loop *el, int fd, int mask) 
{
    unc_ae_api_state *state = el->api_data;

    if (mask & UNC_AE_READABLE) 
    {
        FD_SET(fd, &state->rfds);
    }

    if (mask & UNC_AE_WRITABLE) 
    {
        FD_SET(fd, &state->wfds);
    }
    return UNC_OK;
}
#endif

/**
 * ����: Register a file event. 
 * ����: @el, ae_loop�ṹָ��
 *       @fd, �ļ���������fd
 *       @mask, ����д���
 *       @proc, �ص�����
 *       @client_data, �ص���������
 * ����:
 *      1. fd����Ϊel->events[]������
 * ����: �ɹ���0 ʧ�ܣ�-x
 **/
int unc_ae_create_file_event(unc_ae_event_loop *el, int fd, int mask,
        unc_ae_file_proc *proc, void *client_data) 
{
    if (fd >= UNC_AE_SETSIZE) 
    {
        //UNC_ERROR_LOG("fd:%d, UNC_AE_SETSIZE:%d", fd, UNC_AE_SETSIZE);
        return UNC_ERR;
    } 
    unc_ae_file_event *fe = &el->events[fd];

    if (unc_ae_api_add_event(el, fd, mask) == -1) 
    {
        return UNC_ERR;
    }

    fe->mask |= mask; /* On one fd, multi events can be registered. */
    if (mask & UNC_AE_READABLE) 
    {
        fe->r_file_proc = proc;
        fe->read_client_data = client_data;
    }

    if (mask & UNC_AE_WRITABLE) 
    {
        fe->w_file_proc = proc;
        fe->write_client_data = client_data;
    } 
    /* Ae bug. ��д�¼���ͬһ�������������� */
    //fe->client_data = client_data;
    
    /* Once one file event has been registered, the el->maxfd
       no longer is -1. */
    if (fd > el->maxfd) 
    {
        el->maxfd = fd;
    }
    return UNC_OK;
}

/**
 * ����: delete event
 * ����: �ɹ���0ʧ�ܣ�-x
 **/
#ifdef UNC_EPOLL_MODE

static void unc_ae_api_del_event(unc_ae_event_loop *el, int fd, int delmask) 
{
    unc_ae_api_state *state = el->api_data;
    struct epoll_event ee;
    int mask = el->events[fd].mask & (~delmask);
    ee.events = 0;
    if (mask & UNC_AE_READABLE) 
    {
        ee.events |= EPOLLIN;
    }

    if (mask & UNC_AE_WRITABLE) 
    {
        ee.events |= EPOLLOUT;
    }
    ee.data.u64 = 0;
    ee.data.fd = fd;
    if (mask != UNC_AE_NONE) 
    {
        epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
    } 
    else 
    {
        /* Note, kernel < 2.6.9 requires a non null event pointer even
           for EPOLL_CTL_DEL. */
        epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
    }
} 
#elif defined UNC_SELECT_MODE

static void unc_ae_api_del_event(unc_ae_event_loop *el, int fd, int mask) 
{
    unc_ae_api_state *state = el->api_data;
    if (mask & UNC_AE_READABLE) 
    {
        FD_CLR(fd, &state->rfds);
    }

    if (mask & UNC_AE_WRITABLE) 
    {
        FD_CLR(fd, &state->wfds);
    }
}
#endif

/**
 * ����: Unregister a file event
 * ����: el, fd, mask 
 **/
void unc_ae_delete_file_event(unc_ae_event_loop *el, int fd, int mask) 
{
    if (fd >= UNC_AE_SETSIZE) 
    {
        return;
    }

    unc_ae_file_event *fe = &el->events[fd];
    if (fe->mask == UNC_AE_NONE) 
    {
        return;
    }
    fe->mask = fe->mask & (~mask);
    if (fd == el->maxfd && fe->mask == UNC_AE_NONE) 
    {
        /* All the events on the fd were deleted, update the max fd. */
        int j;
        for (j = el->maxfd - 1; j >= 0; --j) 
        {
            if (el->events[j].mask != UNC_AE_NONE) 
            {
                break;
            }
        }
        /* If all the file events on all fds deleted, max fd will get
           back to -1. */
        el->maxfd = j;
    }
    unc_ae_api_del_event(el, fd, mask);
}

/**
 * ����: Get the file event by fd
 * ����: �ɹ���UNC_AE_READABLE/UNC_AE_WRITEABLE.ʧ�ܣ�0
 **/
int unc_ae_get_file_events(unc_ae_event_loop *el, int fd) 
{
    if (fd >= UNC_AE_SETSIZE) 
    {
        return 0;
    }

    unc_ae_file_event *fe = &el->events[fd];
    return fe->mask;
}

/**
 * ����: get the now time
 * ����: @seconds, @milliseconds
 * ����:
 *      1. ��ǰʱ�����ͺ�����������ֵ������second, milliseconds
 **/
static void unc_ae_get_now_time(long *seconds, long *milliseconds) 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;              /* �� */
    *milliseconds = tv.tv_usec / 1000; /* ΢��=>���� */
}

/**
 * ����: ��ȡ��ǰʱ��֮���milliseconds������ʱ��
 * ����: @milliseconds, @sec, @ms
 * ����:
 *      1. δ��ʱ�����ͺ�����������ֵ������sec, ms
 **/
static void unc_ae_add_milliseconds_to_now(long long milliseconds, 
        long *sec, long *ms) 
{
    long cur_sec, cur_ms, when_sec, when_ms;
    unc_ae_get_now_time(&cur_sec, &cur_ms);
    when_sec = cur_sec + milliseconds / 1000;
    when_ms = cur_ms + milliseconds % 1000;
    /* cur_ms < 1000, when_ms < 2000, so just one time is enough. */
    if (when_ms >= 1000) 
    {
        ++when_sec;
        when_ms -= 1000;
    }
    *sec = when_sec;
    *ms = when_ms;
}

/**
 * ����: Register a time event. 
 * ����: @ms������ʱ�䣬��ǰʱ���ms����֮��
 *       @proc��ʱ��Ļص�
 *       @client_data���ص������Ĳ���
 *       @finalizer_proc��ʱ���¼��������ص�
 * ����:
 *      1. time_event_next_id��0��ʼ��������һ��ʱ���¼�id=0
 *      2. ʱ���¼���һ������ṹ��ÿ���ڵ���һ���¼�
 *      3. ÿ����һ��ʱ���¼��ڵ㣬���뵽����ͷ��
 * ����: �ɹ���ʱ���¼���id�š�ʧ�ܣ�-x
 **/
long long unc_ae_create_time_event(unc_ae_event_loop *el, long long ms, 
        unc_ae_time_proc *proc, void *client_data,
        unc_ae_event_finalizer_proc *finalizer_proc) 
{
    long long id = el->time_event_next_id++;
    unc_ae_time_event *te;
    te = (unc_ae_time_event*)malloc(sizeof(*te));
    if (te == NULL) 
    {
        return UNC_ERR;
    }
    te->id = id;
    unc_ae_add_milliseconds_to_now(ms, &te->when_sec, &te->when_ms);
    te->time_proc = proc;
    te->finalizer_proc = finalizer_proc;
    te->client_data = client_data;
    
    /* Insert time event into the head of the linked list. */
    te->next = el->time_event_head;
    el->time_event_head = te;
    return id;
}

/**
 * ����: Unregister a time event.
 * ����: @el, @id:ʱ���¼�id 
 * ����:
 *      1. el->time_event_next_id����Ҫ����صĲ�������Ϊidֻ��һ����
 * ����: �ɹ���0 ʧ�ܣ�-x
 **/
int unc_ae_delete_time_event(unc_ae_event_loop *el, long long id) 
{
    unc_ae_time_event *te, *prev = NULL;

    te = el->time_event_head;
    while (te) 
    {
        if (te->id == id) 
        {
            /* Delete a node from the time events linked list. */
            if (prev == NULL) 
            {
                el->time_event_head = te->next;
            } 
            else 
            {
                prev->next = te->next;
            }
            if (te->finalizer_proc) 
            {
                /* Ԥ���ڴ�й¶ */
                te->finalizer_proc(el, te->client_data);
            }
            free(te);
            return UNC_OK;
        }
        prev = te;
        te = te->next;
    }
    return UNC_ERR; /* NO event with the specified ID found */
}

/**
 * ����: ����ʱ���¼������ҵ����ȵ�Ӧ�ô�����ʱ���¼���
 * ����: @el
 * ����:
 *      1. This operation is useful to know how many time the select can be
 *         put in sleep without to delay any event. If there are no timers 
 *         NULL is returned.
 *      2. Note that's O(N) since time events are unsorted. Possible optimizations
 *         (not needed by Redis so far, but ...):
 *         1) Insert the event in order, so that the nearest is just the head.
 *            Much better but still insertion or deletion of timer is O(N).
 *         2) Use a skiplist to have this operation as O(1) and insertion as
 *            O(log(N)).
 * ����: �ɹ���ʱ���¼��ڵ�ָ�� ʧ�ܣ�
 **/
static unc_ae_time_event *unc_ae_search_nearest_timer(unc_ae_event_loop *el) 
{
    unc_ae_time_event *te = el->time_event_head;
    unc_ae_time_event *nearest = NULL;

    while (te) 
    {
        if (!nearest || te->when_sec < nearest->when_sec ||
                (te->when_sec == nearest->when_sec &&
                 te->when_ms < nearest->when_ms)) 
        {
            nearest = te;
        }
        te = te->next;
    }
    return nearest;
}

/**
 * ����: Process time events.
 * ����: @el
 * ����:
 *      1. time_proc�������ܺ������������������Ӱ������ʱ����д���
 *      2. time_proc�����ķ���ֵ������ڵ�ǰʱ��㣬��һ�δ����Լ��ĺ�����
 *      3. time_proc�����п��ܻ�ע���µ�ʱ���¼���Ϊ�˷�ֹunc_process_time_events
 *         ����ѭ��ִ����ȥ����¼maxid�����ֲ�������ע���ʱ���¼�
 *      4. time_proc�������0��˵����Ҫ������ʱ���¼�����������Ҫɾ���ýڵ㣬
 *         ��������ṹ�ͷ����˱仯������ÿ�δ����궼��ͷ�ٴα�������
 *      5. �°�redis������һ��last_time���ֶΣ������Ƿ�ֹϵͳʱ��������⣬���
 *         �����ʼʱϵͳʱ����ǰ�����ֱ����أ���ǿ������ʱ���¼������̴���һ��
 * ����: �ɹ������ִ���ʱ����� ʧ�ܣ�
 **/
static int unc_process_time_events(unc_ae_event_loop *el) 
{
    int processed = 0;
    unc_ae_time_event *te;
    long long maxid;
    time_t now = time(NULL);

    /* If the system clock is moved to the future, and then set back to the
     * right value, time events may be delayed in a random way. Often this
     * means that scheduled operations will not be performed soon enough.
     *
     * Here we try to detect system clock skews, and force all the time
     * events to be processed ASAP when this happens: the idea is that
     * processing events earlier is less dangerous than delaying them
     * indefinitely, and practice suggests it is. */
    if (now < el->last_time) {
        te = el->time_event_head;
        while(te) {
            te->when_sec = 0;
            te = te->next;
        }
    }
    el->last_time = now;
    
    te = el->time_event_head;
    maxid = el->time_event_next_id - 1;

    /* ѭ������ʱ���¼����� */
    while (te) 
    {
        long now_sec, now_ms;
        long long id;
        /* Don't process the time event registered during this process. */
        if (te->id > maxid) 
        { 
            te = te->next;
            continue;
        }
        unc_ae_get_now_time(&now_sec, &now_ms);
        /* timeout */ 
        if (now_sec > te->when_sec ||
            (now_sec == te->when_sec && now_ms >= te->when_ms)) 
        {
            /* ���㳬ʱ�������򴥷���ʱ�ص� */
            int ret;
            id = te->id;
            ret = te->time_proc(el, id, te->client_data);
            processed++;
            /* After an event is processed our time event list may no longer be the same, 
             * so we restart from head. Still we make sure to don't process events 
             * registered by event handlers itself in order to don't loop forever.
             * To do so we saved the max ID we want to handle.
             *
             * FUTURE OPTIMIZATIONS:
             * Note that this is NOT great algorithmically. Redis uses a single time event
             * so it's not a problem but the right way to do this is to add the new elements
             * on head, and to flag deleted elements in a special way for later deletion 
             * (putting references to the nodes to delete into another linked list). 
             */
            if (ret > 0) 
            {
                /* û�����꣬time_porcӦ�÷����´δ���ʱ�� */
                unc_ae_add_milliseconds_to_now(ret, &te->when_sec, &te->when_ms);
            } 
            else 
            {
                /* �������ˣ���ֱ��ɾ���ڵ� */
                unc_ae_delete_time_event(el, id);
            }

            te = el->time_event_head;
        } 
        else 
        {
            te = te->next;
        }
    }
    return processed;
}

/**
 * ����: poll����
 * ����: @el��@tvp��poll�ĳ�ʱʱ��ָ��
 * ����:
 *      1. 
 * ����: �ɹ���������file�¼������� ʧ�ܣ�
 **/
#ifdef UNC_EPOLL_MODE

static int unc_ae_api_poll(unc_ae_event_loop *el, struct timeval *tvp) 
{
    unc_ae_api_state *state = el->api_data;
    int retval, numevents = 0;
    retval = epoll_wait(state->epfd, state->events, UNC_AE_SETSIZE,
        tvp ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : -1);
    if (retval > 0) 
    {
        int j;
        numevents = retval;
        for (j = 0; j < numevents; ++j) 
        {
            int mask = 0;
            struct epoll_event *e = state->events + j;
            if (e->events & EPOLLIN) 
            {
                mask |= UNC_AE_READABLE;
            }

            if (e->events & EPOLLOUT) 
            {
                mask |= UNC_AE_WRITABLE;
            }
            el->fired[j].fd = e->data.fd;
            el->fired[j].mask = mask;
        }
    }
    return numevents;
}
#elif defined UNC_SELECT_MODE

static int unc_ae_api_poll(unc_ae_event_loop *el, struct timeval *tvp) 
{
    unc_ae_api_state *state = el->api_data;
    int ret, j, numevents = 0;
    /* ���� */
    memcpy(&state->_rfds, &state->rfds, sizeof(fd_set));
    memcpy(&state->_wfds, &state->wfds, sizeof(fd_set));

    ret = select(el->maxfd + 1, &state->_rfds, &state->_wfds, NULL, tvp);
    if (ret > 0) 
    {
        /* ����ڳ�ʱʱ��tvp����file�¼���������Ӧ��д��fired������ */
        for (j = 0; j <= el->maxfd; ++j) 
        {
            int mask = 0;
            unc_ae_file_event *fe = &el->events[j];
            if (fe->mask == UNC_AE_NONE) 
            {
                continue;
            }
            if (fe->mask & UNC_AE_READABLE && FD_ISSET(j, &state->_rfds)) 
            {
                mask |= UNC_AE_READABLE;
            }
            if (fe->mask & UNC_AE_WRITABLE && FD_ISSET(j, &state->_wfds)) 
            {
                mask |= UNC_AE_WRITABLE;
            }

            /* ��������file�¼���fired���飬ͨ��mask���жϴ����˶�����д */
            el->fired[numevents].fd = j;
            el->fired[numevents].mask = mask;
            numevents++;
        }
    }
    return numevents;
}

#endif


/**
 * ����: Process every pending event
 * ����: @el, @flag
 * ����:
 *      1. Process every pending time event, then every pending file event
 *         (that may be registered by time event callbacks just processed).
 *         Without special flags the function sleeps until some file event
 *         fires, or when the next time event occurs (if any).
 *
 *          If flags is 0, the function does nothing and returns.
 *          if flags has AE_ALL_EVENTS set, all the kind of events are processed.
 *          if flags has AE_FILE_EVENTS set, file events are processed.
 *          if flags has AE_TIME_EVENTS set, time events are processed.
 *          if flags has AE_DONT_WAIT set the function returns ASAP until all
 *          the events that's possible to process without to wait are processed.
 *      2. ���û��ʱ���¼�������Ĭ��AE_DONT_WAITδ���ã���poll����Զ������ֱ��file
 *         �¼�����������Ӧ�þ�����������һ��ʱ���¼�
 * ע�⣺
 *      1. �����������͵��¼�����������Ӧ�ñ�������������������������ʱ��
 *         �����ϵ��¼���������!!!!!!!��������������
 * ����: �ɹ���the number of events processed. ʧ�ܣ�
 **/
int unc_ae_process_events(unc_ae_event_loop *el, int flags) 
{
    int processed = 0, numevents;

    /* Nothing to do ? return ASAP */
    if (!(flags & UNC_AE_TIME_EVENTS) && !(flags & UNC_AE_FILE_EVENTS)) 
    {
        return 0;
    }

    /* Note that we want call select() even if there are no file
       events to process as long as we want to process time events,
       in order to sleep until the next time event is ready to fire. */
    if (el->maxfd != -1 ||
        ((flags & UNC_AE_TIME_EVENTS) && !(flags & UNC_AE_DONT_WAIT))) 
    {
        /* ��file�¼���ע�ᣬ����(Ҫ�ȴ�ʱ��ʱ���Ҳ��������˳���) */
        int j;
        unc_ae_time_event *shortest = NULL;
        struct timeval tv, *tvp;
        
        if (flags & UNC_AE_TIME_EVENTS && !(flags & UNC_AE_DONT_WAIT)) 
        {
            shortest = unc_ae_search_nearest_timer(el);
        } 
        /* 
         *  ����ԭ��:���ҳ�һ������ͻᷢ�������飬Ȼ�������˿̵�ʱ����룬�ڴ˾����ڿ�����select����
         *           �ȹ������ʱ����룬�����Ȼû��file�¼���select�Զ��⿪������ȥ����ʱ���¼���
         *           �������tv�������ʱ�����tvp��ָ��tv��ָ�롣
         */
        if (shortest) 
        {
            long now_sec, now_ms;

            /* Calculate the time missing for the nearest timer 
               to fire. store it in the tv */
            unc_ae_get_now_time(&now_sec, &now_ms);
            tvp = &tv;
            tvp->tv_sec = shortest->when_sec - now_sec;
            if (shortest->when_ms < now_ms) 
            {
                tvp->tv_usec = ((shortest->when_ms + 1000) - now_ms) * 1000;
                --tvp->tv_sec;
            } 
            else 
            {
                tvp->tv_usec = (shortest->when_ms - now_ms) * 1000;
            }

            if (tvp->tv_sec < 0) 
            {
                tvp->tv_sec = 0;
            }

            if (tvp->tv_usec < 0) 
            {
                tvp->tv_usec = 0;
            }
        } 
        else 
        {
            /* ���û��ʱ���¼������Ƽ�!! */ 
            /* Ĭ�������UNC_AE_DONT_WAITδ���ã�poll��һֱ����ֱ��file�¼�������
             * �������������˳�����tv����Ϊ0��select�������˳� */
            /* If we have to check for events but need to return
               ASAP because of AE_DONT_WAIT we need to set the 
               timeout to zero. */
            if (flags & UNC_AE_DONT_WAIT) 
            {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            } 
            else 
            {
                /* Otherwise we can block. */
                tvp = NULL; /* wait forever�������޳�ʱ */
            }
        }

        /* block������select/epool����fileʱ�䣬tvpָʾ��ʱ�䣬��ʾ������������ʱ�� */
        numevents = unc_ae_api_poll(el, tvp);
        
        /* ���������е�file�¼�(�ں���unc_ae_api_poll�У�fired[]�ǲ��ϸ��ǵ�) */
        for (j = 0; j < numevents; ++j) 
        {
            unc_ae_file_event *fe = &el->events[el->fired[j].fd];
            
            int mask = el->fired[j].mask;
            int fd   = el->fired[j].fd;
            int rfired = 0;/* �Ƿ񴥷��˶��¼� */
            
            /* Note the fe->mask & mask & ... code: maybe an already
               processed event removed an element that fired and we
               still didn't processed, so we check if the events is 
               still valid. */
            /* 
             * ����:events�����е�Ԫ�أ����ܱ�һ���Ѿ������˵��¼��޸��ˣ���ʱ
             * fired�¼���û�����أ�����Ҫ����֤һ��events�����ӦԪ��״̬
             */   
            if (fe->mask & mask & UNC_AE_READABLE) 
            {
                rfired = 1;
                fe->r_file_proc(el, fd, fe->read_client_data, mask);
            } 

            if (fe->mask & mask & UNC_AE_WRITABLE) 
            {
                /* ȷ���������д��ͬһ��������ʱ�򣬲����ظ�ִ�� */
                if (!rfired || fe->w_file_proc != fe->r_file_proc) 
                {
                    fe->w_file_proc(el, fd, fe->write_client_data, mask);
                }
            }

            ++processed;
        }
    }
    /* Check time events */
    /* �������е�ʱ���¼� */
    if (flags & UNC_AE_TIME_EVENTS) 
    {
        processed += unc_process_time_events(el);
    }

    /* Return the number of processed file/time events */
    return processed; 
}
/**
 * ����: Wait for milliseconds until the given file descriptior becomes
 *       writable/readable/exception
 * ����: @
 * ����:
 *      1. ������ĳ���ļ���fd��select���ȴ�milliseconds����
 * ����: �ɹ��� ʧ�ܣ�
 **/
int unc_ae_wait(int fd, int mask, long long milliseconds) 
{
    struct timeval tv;
    fd_set rfds, wfds, efds;
    int retmask = 0, retval;

    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    if (mask & UNC_AE_READABLE) 
    {
        FD_SET(fd, &rfds);
    }
    
    if (mask & UNC_AE_WRITABLE) 
    {
        FD_SET(fd, &wfds);
    }
    
    if ((retval = select(fd + 1, &rfds, &wfds, &efds, &tv)) > 0) 
    {
        if (FD_ISSET(fd, &rfds)) 
        {
            retmask |= UNC_AE_READABLE;
        }
        if (FD_ISSET(fd, &wfds)) 
        {
            retmask |= UNC_AE_WRITABLE;
        }
        return retmask;
    } 
    else 
    {
        return retval;
    }
}

/**
 * ����: main loop of the event-driven framework 
 * ����: @el
 * ����:
 *      1. ���before_sleep�ǿգ����ȵ���֮��Ȼ���������loop
 **/
void unc_ae_main_loop(unc_ae_event_loop *el) 
{
    el->stop = 0;
    while (!el->stop) 
    {
        if (el->before_sleep != NULL) 
        {
            el->before_sleep(el);
        }
        unc_ae_process_events(el, UNC_AE_ALL_EVENTS);
    }
}
 
/**
 * ����: set before_sleep_proc
 * ����: @el, @before_sleep
 */
void unc_ae_set_before_sleep_proc(unc_ae_event_loop *el, 
        unc_ae_before_sleep_proc *before_sleep) 
{
    el->before_sleep = before_sleep;
}

#ifdef __UNC_AE_TEST_MAIN__
int print_timeout(unc_ae_event_loop *el, long long id, void *client_data) 
{
    printf("Hello, AE!\n");
    return 5000; /* return AE_NOMORE */
}

struct items {
    char        *item_name;
    int         freq_sec;
    long long   event_id;
} acq_items [] = {
    {"acq1", 10 * 1000, 0},
    {"acq2", 30 * 1000, 0},
    {"acq3", 60 * 1000, 0},
    {"acq4", 10 * 1000, 0},
    {NULL, 0, 0}
};

int output_result(unc_ae_event_loop *el, long long id, void *client_data) 
{
    int i = 0;
    for ( ; ; ++i) 
    {
        if (!acq_items[i].item_name) 
        {
            break;
        }
        if (id == acq_items[i].event_id) 
        {
            printf("%s:%d\n", acq_items[i].item_name, (int)time(NULL));
            fflush(stdout);
            return acq_items[i].freq_sec;
        }
    }
    return 0;
}

/*���ʱ���¼�*/
void add_all(unc_ae_event_loop *el) 
{
    int i = 0;
    for ( ; ; ++i) 
    {
        if (!acq_items[i].item_name) 
        {
            break;
        }
        acq_items[i].event_id = unc_ae_create_time_event(el, 
            acq_items[i].freq_sec,
            output_result, NULL, NULL);
    }
}

/*ɾ��ʱ���¼�*/
void delete_all(unc_ae_event_loop *el) 
{
    int i = 0;
    for ( ; ; ++i) {
        if (!acq_items[i].item_name) {
            break;
        } 

        unc_ae_delete_time_event(el, acq_items[i].event_id);
    }
}

/* file�¼� 
 * �¼��Ĵ���Ӧ������������������
 */
void file_process(struct unc_ae_event_loop *el, int fd, void *client_data, int mask)
{
    char buf[1000] = {0};
    if(mask & UNC_AE_WRITABLE)
    {
        write( fd, client_data, strlen(client_data));
        /* ģ����������� */
        //sleep(1);
    }
    
    if (mask & UNC_AE_READABLE)
    {
        read(fd, buf, 1000);
        //printf("read:%s\n", buf);
        //sleep(2);/* ģ����������� */
    }
}


int main(int argc, char *argv[]) 
{
    //long long id;
    int mask, fd;

    fd = open("test_ae.file", O_RDWR );
    mask = UNC_AE_WRITABLE | UNC_AE_READABLE;
    unc_ae_event_loop *el = unc_ae_create_event_loop();
    add_all(el);
    /* �����epoll����֧�� */
    unc_ae_create_file_event(el, fd, mask, file_process, "hello\n");
    unc_ae_main_loop(el);
    delete_all(el);
    exit(0);
}
#endif /* __UNC_AE_TEST_MAIN__ */

