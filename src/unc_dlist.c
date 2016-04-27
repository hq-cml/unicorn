/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_dlist.c
 * 
 * Description :  Unc_dlist, a generic doubly linked list implementation.
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/
#include "unc_core.h"
 
/**
 * ����: Initialize a new list.
 * ����: @
 * ����:
 *      1. The initalized list should be freed with dlist_destroy(), 
 *         but private value of each node need to be freed by the 
 *         user before to call dlist_destroy if list's 'free' pointer
 *         is NULL.
 * ����: �ɹ���dlist�ṹָ�룬 ʧ�ܣ�NULL
 **/
unc_dlist_t *unc_dlist_init(void) 
{
    unc_dlist_t *dl;

    if ((dl = malloc(sizeof(*dl))) == NULL) 
    {
        return NULL;
    }

    dl->head = NULL;
    dl->tail = NULL;
    dl->len = 0;
    dl->dup = NULL;
    dl->free = NULL;
    dl->match = NULL;
    return dl;
}
/**
 * ����: Destroy the whole list. 
 * ����: @dl��dlist�ṹ��ַ
 * ����:
 *      1. If the list's 'free' pointer is not NULL, 
 *         the value will be freed automately first.
 **/
void unc_dlist_destroy(unc_dlist_t *dl) 
{
    unsigned int len;
    unc_dlist_node_t *curr = NULL;
    unc_dlist_node_t *next = NULL;

    curr = dl->head;
    len = dl->len;
    while (len--) 
    {
        next = curr->next;
        if (dl->free)
            dl->free(curr->value);
        free(curr);
        curr = next;
    }
    free(dl);
}

/**
 * ����: Add a new node to the list's head, containing the specified 
 *       'value' pointer as value. 
 * ����: @dl, dlist�ṹָ��
 *       @value, valueָ��
 * ����:
 *      1. ͷ������һ���ڵ㣨��ͷ��㣩
 *      2. ���ص�ֵ�Ǵ���ʱ���ָ��
 * ����: �ɹ���dlist�ṹָ��,ʧ�ܣ�NULL
 **/
unc_dlist_t *unc_dlist_add_node_head(unc_dlist_t *dl, void *value) 
{
    unc_dlist_node_t *node = NULL;
    if ((node = malloc(sizeof(*node))) == NULL) 
    {
        return NULL;
    }
    
    if(dl->dup)
        node->value = dl->dup(value);
    else
        node->value = value;

    if (dl->len == 0) 
    {
        /* ��һ�� */
        dl->head = node;
        dl->tail = node;
        node->prev = NULL;
        node->next = NULL;
    } 
    else 
    {
        node->prev = NULL;
        node->next = dl->head;
        dl->head->prev = node;
        dl->head = node;
    }
    dl->len++;
    return dl;
}
 
/**
 * ����: Add a new node to the list's tail, containing the specified
 *       'value' pointer as value.
 * ����: @
 * ����:
 *      1. 
 * ����: �ɹ���dlist�ṹָ��,ʧ�ܣ�NULL
 **/
unc_dlist_t *unc_dlist_add_node_tail(unc_dlist_t *dl, void *value) 
{
    unc_dlist_node_t *node;
    if ((node = malloc(sizeof(*node))) == NULL) 
    {
        return NULL;
    }

    if(dl->dup)
        node->value = dl->dup(value);
    else
        node->value = value;

    if (dl->len == 0) 
    {
        /* ��һ�� */
        dl->head = node;
        dl->tail = node;
        node->prev = NULL;
        node->next = NULL;
    } 
    else 
    {
        node->prev = dl->tail;
        node->next = NULL;
        dl->tail->next = node;
        dl->tail = node;
    }
    dl->len++;
    return dl;
}

/**
 * ����: Add a new node to the list's midle, 
 * ����: @dl @mid_node @value @after
 * ����:
 *      1. mid_node�����׼�㣬after==1�����ڻ�׼֮�����
 *         after==0 �ڻ�׼��֮ǰ����
 * ����: �ɹ� �� ʧ�ܣ�
 **/
unc_dlist_t *unc_dlist_insert_node(unc_dlist_t *dl, unc_dlist_node_t *mid_node, 
        void *value, int after) 
{
    unc_dlist_node_t *node;
    if ((node = malloc(sizeof(*node))) == NULL)
        return NULL;

    node->value = value;
    if (after) 
    {
        /*����֮�����*/
        node->prev = mid_node;
        node->next = mid_node->next;
        if (dl->tail == mid_node) 
        {
            dl->tail = node;
        }
    } 
    else 
    {
        /*����֮ǰ����*/
        node->next = mid_node;
        node->prev = mid_node->prev;
        if (dl->head == mid_node) 
        {
            dl->head = node;
        }
    }

    if (node->prev != NULL) 
    {
        node->prev->next = node;
    }

    if (node->next != NULL) 
    {
        node->next->prev = node;
    }
    dl->len++;
    return dl;
}

/**
 * ����: Remove the specified node from the specified list.
 * ����: @dl @node
 * ����:
 *      1. If the list's 'free' pointer is not NULL, 
 *         the value will be freed automately first.
 **/
void unc_dlist_delete_node(unc_dlist_t *dl, unc_dlist_node_t *node) 
{
    if (node->prev) 
    {
        node->prev->next = node->next;
    } 
    else 
    {
        dl->head = node->next;
    }

    if (node->next) 
    {
        node->next->prev = node->prev;
    } 
    else 
    {
        dl->tail = node->prev;
    }

    if (dl->free) 
    {
        dl->free(node->value);
    }

    free(node);
    dl->len--;
}

/**
 * ����: create a list iterator.
 * ����: @dl
 *       @direction������ͷ->β/β->ͷ
 * ����:
 *      1. 
 * ����: �ɹ���iter�ṹָ�룬ʧ�ܣ�NULL
 **/
unc_dlist_iter_t *unc_dlist_get_iterator(unc_dlist_t *dl, int direction) 
{
    unc_dlist_iter_t *iter;
    if ((iter = malloc(sizeof(*iter))) == NULL)
        return NULL;

    if (direction == UNC_DLIST_START_HEAD) 
        iter->node= dl->head; 
    else
        iter->node = dl->tail;
    
    iter->direction = direction;
    return iter;
}

/**
 * ����: Release the iterator memory.
 **/
void unc_dlist_destroy_iterator(unc_dlist_iter_t *iter) 
{
    if (iter) 
        free(iter);
}

/**
 * ����: Create an iterator in the list iterator structure.
 **/
void unc_dlist_rewind(unc_dlist_t *dl, unc_dlist_iter_t *iter) 
{
    iter->node = dl->head;
    iter->direction = UNC_DLIST_START_HEAD;
}

/**
 * ����: Create an iterator in the list iterator structure.
 **/
void unc_dlist_rewind_tail(unc_dlist_t *dl, unc_dlist_iter_t *iter) 
{
    iter->node = dl->tail;
    iter->direction = UNC_DLIST_START_TAIL;
}

/**
 * ����: ����ָ��ķ����ƶ�ָ��.
 * ����: @iter
 * ����:
 *      1. ����ֵ��ָ���ʼָ���Ԫ��
 *      2. the classical usage patter is:
 *      iter = unc_dlist_get_iterator(dl, <direction>);
 *      while ((node = unc_dlist_next(iter)) != NULL) {
 *          do_something(dlist_node_value(node));
 *      }
 * ����: �ɹ���node�ṹָ�룬ʧ�ܣ�NULL
 **/
unc_dlist_node_t *unc_dlist_next(unc_dlist_iter_t *iter) 
{
    unc_dlist_node_t *curr = iter->node;
    if (curr != NULL) 
    {
        if (iter->direction == UNC_DLIST_START_HEAD)
            iter->node = curr->next;
        else 
            iter->node = curr->prev;/* PREPARE TO DELETE ? */
    }
    return curr;
}

/**
 * ����: Search the list for a node matching a given key.
 * ����: @dl @key
 * ����:
 *      1. ���match�������ڣ�����match�����Ƚϣ�����ֱ�ӱȽ�valueָ���key
 * ����: �ɹ���nodeָ��,ʧ�ܣ�NULL
 **/
unc_dlist_node_t *unc_dlist_search_key(unc_dlist_t *dl, void *key) 
{
    unc_dlist_iter_t *iter = NULL;
    unc_dlist_node_t *node = NULL;
    iter = unc_dlist_get_iterator(dl, UNC_DLIST_START_HEAD);
    while ((node = unc_dlist_next(iter)) != NULL) 
    {
        if (dl->match) 
        {
            if (dl->match(node->value, key)) 
            {
                unc_dlist_destroy_iterator(iter);
                return node;
            }
        } 
        else 
        {
            if (key == node->value) 
            {
                unc_dlist_destroy_iterator(iter);
                return node;
            }
        }
    }
    unc_dlist_destroy_iterator(iter);
    return NULL;
}

/**
 * ����: Duplicate the whole list. 
 * ����: @orig
 * ����:
 *      1. The 'dup' method set with dlist_set_dup() function is used to copy the
 *         node value.Other wise the same pointer value of the original node is 
 *         used as value of the copied node. 
 * ����: �ɹ����µ�dlist�ṹָ�룬ʧ�ܣ�NULL
 **/
unc_dlist_t *unc_dlist_dup(unc_dlist_t *orig) 
{
    unc_dlist_t *copy;
    unc_dlist_iter_t *iter;
    unc_dlist_node_t *node;

    if ((copy = unc_dlist_init()) == NULL) 
    {
        return NULL;
    }

    copy->dup = orig->dup;
    copy->free = orig->free;
    copy->match = orig->match;
    iter = unc_dlist_get_iterator(orig, UNC_DLIST_START_HEAD);
    while ((node = unc_dlist_next(iter)) != NULL) 
    {
        void *value;
        if (copy->dup) 
        {
            value = copy->dup(node->value);
            if (value == NULL) 
            {
                unc_dlist_destroy(copy);
                unc_dlist_destroy_iterator(iter);
                return NULL;
            }
        } 
        else 
        {
            value = node->value;
        }
        if (unc_dlist_add_node_tail(copy, value) == NULL) 
        {
            unc_dlist_destroy(copy);
            unc_dlist_destroy_iterator(iter);
            return NULL;
        }
    } 
    unc_dlist_destroy_iterator(iter);
    return copy;
}

/**
 * ����: ��ͷ��β��������ǰ�ҵ���index��Ԫ�أ�����ָ�� 
 * ����: @
 * ����:
 *      1. index where 0 is the head, 1 is the element next
 *         to head and so on. Negative integers are used in 
 *         order to count from the tail, -1 is the last element,
 *         -2 the penultimante and so on. 
 * ����: �ɹ���nodeָ��,ʧ�ܣ�NULL
 **/
unc_dlist_node_t *unc_dlist_index(unc_dlist_t *dl, int index) 
{
    unc_dlist_node_t *node;

    if (index < 0) 
    {
        /* indexС��0�����β����ǰ�� */
        index = (-index) - 1;
        node = dl->tail;
        while (index-- && node) 
        {
            node = node->prev;
        }
    } 
    else 
    {
        node = dl->head;
        while (index-- && node) 
        {
            node = node->next;
        }
    }
    return node;
}
