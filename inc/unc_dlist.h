/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_dlist.h
 * 
 * Description :  Unc_dlist, a generic doubly linked list implementation.
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/
#ifndef __UNC_DLIST_H_INCLUDED__
#define __UNC_DLIST_H_INCLUDED__
/*
#include <stdio.h>
#include <stdlib.h>
*/
/* Directions for iterators */
#define UNC_DLIST_START_HEAD    0
#define UNC_DLIST_START_TAIL    1

typedef struct unc_dlist_node 
{
    struct unc_dlist_node *prev;
    struct unc_dlist_node *next;
    void *value;
} unc_dlist_node_t;

typedef struct unc_dlist 
{
    unsigned int len;                     /* 元素个数 */
    unc_dlist_node_t *head;               /* 第一个节点 */
    unc_dlist_node_t *tail;               /* 最后一个节点 */
    void *(*dup)(void *ptr);            
    void (*free)(void *ptr);              /* free每个节点 */
    int (*match)(void *ptr, void *key);

} unc_dlist_t;

typedef struct unc_dlist_iter 
{
    unc_dlist_node_t *node; /* 该迭代器对应的元素的地址 */
    int direction;
} unc_dlist_iter_t;

/* Functions implemented as macros */
#define unc_dlist_length(l)     ((l)->len)
#define unc_dlist_first(l)      ((l)->head) 
#define unc_dlist_last(l)       ((l)->tail)
#define unc_dlist_prev_node(n)  ((n)->prev)
#define unc_dlist_next_node(n)  ((n)->next)
#define unc_dlist_node_value(n) ((n)->value)

#define unc_dlist_set_dup(l,m)      ((l)->dup = (m))
#define unc_dlist_set_free(l,m)     ((l)->free = (m))
#define unc_dlist_set_match(l,m)    ((l)->match = (m)) 

#define unc_dlist_get_dup(l)    ((l)->dup)
#define unc_dlist_get_free(l)   ((l)->free)
#define unc_dlist_get_match(l)  ((l)->match)

/* Prototypes */
unc_dlist_t *unc_dlist_init(void);
void unc_dlist_destroy(unc_dlist_t *dl);
unc_dlist_t *unc_dlist_add_node_head(unc_dlist_t *dl, void *value);
unc_dlist_t *unc_dlist_add_node_tail(unc_dlist_t *dl, void *value);
unc_dlist_t *unc_dlist_insert_node(unc_dlist_t *dl, unc_dlist_node_t *ponode, 
        void *value, int after);
void unc_dlist_delete_node(unc_dlist_t *dl, unc_dlist_node_t *pnode);
unc_dlist_iter_t *unc_dlist_get_iterator(unc_dlist_t *dl, int direction);
unc_dlist_node_t *unc_dlist_next(unc_dlist_iter_t *iter);
void unc_dlist_destroy_iterator(unc_dlist_iter_t *iter);
unc_dlist_t *unc_dlist_dup(unc_dlist_t *orig);
unc_dlist_node_t *unc_dlist_search_key(unc_dlist_t *dl, void *key);
unc_dlist_node_t *unc_dlist_index(unc_dlist_t *dl, int index);
 
void unc_dlist_rewind(unc_dlist_t *dl, unc_dlist_iter_t *iter);
void unc_dlist_rewind_tail(unc_dlist_t *dl, unc_dlist_iter_t *iter);
#endif /* __UNC_DLIST_H_INCLUDED__ */

