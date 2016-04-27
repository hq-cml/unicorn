/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *    Filename :  Unc_vector.c
 * 
 * Description :  Unc_vector, a vector based on vector. Support any type.
 *                ��һ��Ƭ(size*slots)�������ڴ��У�ģ����������Ϊ.
 *                �����������Ԫ�ؿ�������������.
 *
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/
#include "unc_core.h"

/**
 * ����: init vector
 * ����: @vec, @slots, @size
 * ����:
 *      1. ��һ��Ƭ�������ڴ��У�ģ����������Ϊ
 * ����: �ɹ� 0�� ʧ�ܣ�-x
 **/ 
static int unc_vector_init(unc_vector_t *vec, uint32_t slots, size_t size)
{
    if((vec->data = calloc(1, slots * size)) == NULL) 
    {
       //free(vec);//��ɵڶ����ͷ�bug
       return UNC_ERR;
    }

    vec->count = 0;
    vec->size  = size;
    vec->slots = slots;

    return UNC_OK;
}

/**
 * ����: Create a vector, and init it
 * ����: @slots, vectorԪ�ظ���
 *       @size, ÿ��Ԫ�صĴ�С
 * ����:
 *      1. 
 * ����: �ɹ���vector�ṹ��ַ��ʧ�ܣ�NULL
 **/
unc_vector_t *unc_vector_create(uint32_t slots, size_t size)
{
    unc_vector_t  *vec;
    
    if((vec = malloc(sizeof(unc_vector_t))) == NULL) 
    {
        return NULL;
    }

    if(unc_vector_init(vec, slots, size) != UNC_OK) 
    {
        free(vec);
        return NULL;
    }

    return vec;
}

/**
 * ����: destroy the vector
 * ����: @vec
 * ����:
 *      1. ���ͷ�data��vec����ṹ���ͷ�
 **/
static void unc_vector_destroy(unc_vector_t *vec) 
{
    if (vec->data) 
        free(vec->data);
    
    vec->slots = 0;
    vec->count = 0;
    vec->size = 0;
    vec->data = NULL;
}

/**
 * ����: destroy the vector
 * ����: @vec
 * ����:
 *      1. �ͷ�data��Ȼ���ͷ�vec����
 **/
void unc_vector_free(unc_vector_t *vec) 
{
    unc_vector_destroy(vec);
    free(vec);
}

/**
 * ����: ����һ��Ԫ�صĵ�ַ����ô�Ԫ����vector�е�����
 * ����: @vec, @elemԪ�ص�ַ
 * ����: �ɹ���������ʧ�ܣ�-1
 **/
uint32_t unc_vector_idx(unc_vector_t *vec, void *elem)
{
    uint8_t   *p, *q;    //�ڴ水�ֽڱ�ַ
    uint32_t   off, idx;

    if(elem < vec->data)
    {
        return UNC_ERR;
    }

    p = vec->data;
    q = elem;
    off = (uint32_t)(q - p);

    if(off % (uint32_t)vec->size != 0)
    {
        return UNC_ERR;
    }


    idx = off / (uint32_t)vec->size;

    return idx;
}

/**
 * ����: push a element
 * ����: @vec
 * ����: �ɹ� 0 ʧ�� -x
 **/
void *unc_vector_push(unc_vector_t *vec, void *data)
{
    void   *elem, *new;
    size_t  size;

    if (vec->count >= (vec->slots)) 
    {
        /* the vector is full; allocate new vector */
        size = vec->size * vec->slots;
        new  = realloc(vec->data, 2 * size);
        if (new == NULL) 
        {
            return NULL;
        }
        vec->data   = new;
        vec->slots *= 2;
    }

    elem = (uint8_t *)vec->data + vec->size * vec->count;
    memcpy(elem, data, vec->size);
    vec->count++;

    return elem;
}

/**
 * ����: pop a element
 * ����: @vec
 * ����:
 *      1. ������0��ʼ��ע��߽�����
 * ����: �ɹ���element��ַ��ʧ�ܣ�NULL
 **/
void *unc_vector_pop(unc_vector_t *vec)
{
    void *elem;

    if(vec->count <= 0)
    {
        return NULL;
    }

    vec->count--;
    elem = (uint8_t *)vec->data + vec->size * vec->count;

    return elem;
}

/**
 * ����: ���ָ��������Ԫ�صĵ�ַ
 * ����: @vec��@idx
 * ����:
 *      1. ������0��ʼ��ע��߽�����
 * ����: �ɹ���element��ַ��ʧ�ܣ�NULL
 **/
void *unc_vector_get_at(unc_vector_t *vec, uint32_t idx)
{
    void  *elem;
    if(vec->count == 0 || idx >= vec->count)
    {
        return NULL;
    }


    elem = (uint8_t *)vec->data + (vec->size * idx);
    return elem;
}

/**
 * ����: ���ջ��Ԫ�أ����ǲ�pop
 * ����: @vec
 * ����: �ɹ���element��ַ��ʧ�ܣ�NULL
 **/
void *unc_vector_top(unc_vector_t *vec)
{
    return unc_vector_get_at(vec, vec->count - 1);
}

/**
 * ����: ���ո����ȽϺ������������Ԫ�ؽ�����������
 * ����: @vec, @cmp
 * ����:
 *       void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));
 *
 * DESCRIPTION
 *       The qsort() function sorts an array with nmemb elements of size size.  The base argument points to the start of the array.
 *       The contents of the array are sorted in ascending order according to a comparison function pointed to by compar, which is called with two arguments that point to the objects being compared.
 *       The  comparison function must return an integer less than, equal to, or greater than zero if the first argument is considered to be respectively less than, equal to, or greater than the second.  If two mem-
 *       bers compare as equal, their order in the sorted array is undefined.
 * ����: �ɹ���0��ʧ�ܣ�-1
 **/
int unc_vector_sort(unc_vector_t *vec, unc_vector_cmp_t cmp)
{
    if(vec->count == 0)
    {
        return UNC_ERR;
    }
    qsort(vec->data, vec->count, vec->size, cmp);
    return UNC_OK;
}

/**
 * ����: �������ÿ��Ԫ�أ�ִ���ض�����
 * ����: @vec, @func, 
 *       @data������func���������
 * ����:
 *      1. ���ĳ��Ԫ��ִ��ʧ�ܣ���ֱ�ӷ��ش���
 * ����: �ɹ���0��ʧ�ܣ�NULL
 **/
int unc_vector_each(unc_vector_t *vec, unc_vector_each_t func, void *data)
{
    uint32_t i;
    int  rc;
    void *elem;

    if(vec->count == 0 || func==NULL)
    {
        return UNC_ERR;
    }
    
    for (i = 0; i < vec->count; i++) 
    {
        elem = unc_vector_get_at(vec, i);

        rc = func(elem, data);
        if (rc != UNC_OK) 
        {
            return rc;
        }
    }

    return UNC_OK;
}

#ifdef __UNC_VECTOR_TEST_MAIN__

//#define COUNT       100   #����resize
#define COUNT 10

typedef struct _test{
    int a;
    int b;
}test_t;

int my_cmp(const void* val1, const void* val2)
{
     test_t *p1, *p2;
     p1 = (test_t*)val1;
     p2 = (test_t*)val2;

     return (p1->b - p2->b); 
}

int my_print(void* elem, void* data)
{
    test_t        *p1;
    int            idx;
    unc_vector_t  *vec;
    
    p1  = (test_t*)elem; 
    vec = (unc_vector_t*)data;
    idx = unc_vector_idx(vec, elem);
    
    printf("idx=%d, value.a=%d, value.b=%d\n", idx, p1->a, p1->b);
    return UNC_OK;
}


int main(int argc, char **argv) 
{
    int i = 0;
    test_t t;
    
    unc_vector_t *vec = unc_vector_create(32, sizeof(test_t));
    for (i = 0; i < COUNT; ++i) 
    {
        t.a = i;
        t.b = 100-i;
        unc_vector_push(vec, &t);
    }

    //�����ظ�ֵ
    t.a = 3;t.b = 97;
    unc_vector_push(vec, &t);
    unc_vector_push(vec, &t);
    /* 
     // DUMP
     for (i = 0; i < vec->count; ++i) 
     {
        value = (test_t *)unc_vector_get_at(vec, i);
        printf("idx=%d, value.a=%d, value.b=%d\n",i, value->a, value->b);
     }
     */
    unc_vector_each(vec, my_print, vec);
    printf("slots:%d\n", vec->slots);
    printf("count:%d\n\n\n", vec->count);

    //����
    unc_vector_sort(vec, my_cmp);
    unc_vector_each(vec, my_print, vec);
    printf("slots:%d\n", vec->slots);
    printf("count:%d\n\n\n", vec->count);

    
    unc_vector_free(vec);

    return 0;
}
#endif /* _MSD_VECTOR_TEST_MAIN__ */

