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
 *                ��һ��Ƭ(size*slots)�������ڴ��У�ģ����������Ϊ.
 *                �����������Ԫ�ؿ�������������
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
    uint32_t    count;  /*  vector��Ԫ�ظ�����Vector��������0��ʼ����count<=slots */
    void       *data;   /* ����Ԫ����ʼ��ַ */
    size_t      size;   /* the size of a member��ÿ��Ԫ�صĴ�С */
    uint32_t    slots;  /* �Ѿ�������ڴ棬������member������ */
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
