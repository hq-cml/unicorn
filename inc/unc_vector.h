/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_vector.h
 * 
 * Description :  Unc_vector, a vector based on array. Support any type.
 *                在一整片(size*slots)连续的内存中，模拟出数组的行为.
 *                理论上数组的元素可以是任意类型
 *
 *
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/
#ifndef __UNC_VECTOR_H_INCLUDED__
#define __UNC_VECTOR_H_INCLUDED__       

/*
include <assert.h>
*/

typedef struct{
    uint32_t    count;  /*  vector中元素个数，Vector的索引从0开始，即count<=slots */
    void       *data;   /* 数组元素起始地址 */
    size_t      size;   /* the size of a member，每个元素的大小 */
    uint32_t    slots;  /* 已经分配的内存，能容纳member的数量 */
}unc_vector_t;

typedef int (*unc_vector_cmp_t)(const void *, const void *); 
typedef int (*unc_vector_each_t)(void *, void *);

unc_vector_t *unc_vector_create(uint32_t n, size_t size);
uint32_t      unc_vector_idx(unc_vector_t *vec, void *elem);
void  unc_vector_free(unc_vector_t *vec);
void *unc_vector_push(unc_vector_t *vec, void *data);
void *unc_vector_pop(unc_vector_t *vec);
void *unc_vector_get_at(unc_vector_t *vec, uint32_t idx);
void *unc_vector_top(unc_vector_t *vec); 
int   unc_vector_sort(unc_vector_t *vec, unc_vector_cmp_t cmp);
int   unc_vector_each(unc_vector_t *vec, unc_vector_each_t func, void *data);

#endif
