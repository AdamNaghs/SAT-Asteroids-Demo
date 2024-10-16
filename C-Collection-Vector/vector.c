#include "vector.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void vec_deref_free(const void *data)
{
    free(*(void **)data);
}

static size_t default_growth_rate(Vec *v)
{
    return v->capacity * 2;
}

int vec_char_cmp(const void *data0, const void *data1)
{
    /* compare the first byte */
    if (!(data0 && data1))
        return 0;
    char d0 = *(char *)data0, d1 = *(char *)data1;
    if (d0 == d1)
        return 0;
    if (d0 > d1)
        return 1;
    return -1;
}

int vec_int_cmp(const void *data0, const void *data1)
{
    /* compare the first byte */
    if (!(data0 && data1))
        return 0;
    int d0 = *(int *)data0, d1 = *(int *)data1;
    if (d0 == d1)
        return 0;
    if (d0 > d1)
        return 1;
    return -1;
}

int vec_uint_cmp(const void *data0, const void *data1)
{
    /* compare the first byte */
    if (!(data0 && data1))
        return 0;
    unsigned int d0 = *(unsigned int *)data0, d1 = *(unsigned int *)data1;
    if (d0 == d1)
        return 0;
    if (d0 > d1)
        return 1;
    return -1;
}

int vec_ll_cmp(const void *data0, const void *data1)
{
    /* compare the first byte */
    if (!(data0 && data1))
        return 0;
    long long d0 = *(long long *)data0, d1 = *(long long *)data1;
    if (d0 == d1)
        return 0;
    if (d0 > d1)
        return 1;
    return -1;
}

int vec_ull_cmp(const void *data0, const void *data1)
{
    /* compare the first byte */
    if (!(data0 && data1))
        return 0;
    unsigned long long d0 = *(unsigned long long *)data0, d1 = *(unsigned long long *)data1;
    if (d0 == d1)
        return 0;
    if (d0 > d1)
        return 1;
    return -1;
}

Vec *vec_new(size_t capacity, size_t elem_size, void_cmp_func cmp, vec_growth_rate_func grow, void (*free_entry)(const void *))
{
    VEC_ASSERT(elem_size != 0);
    Vec *p = malloc(sizeof(Vec));
    VEC_ASSERT(p);
    *p = (Vec){.elem_size = elem_size,
               .capacity = 0,
               .len = 0,
               .data = NULL,
               .cmp = cmp ? cmp : NULL,
               .grow = grow ? grow : default_growth_rate,
               .free_entry = free_entry ? free_entry : NULL,};
    vec_resize(p, capacity);
    return p;
}

void vec_free(Vec *v)
{
    VALIDATE_VECTOR(v);
    vec_clear(v);
    free(v->data);
    free(v);
}

void *vec_at(Vec *v, size_t index)
{
    return (void *)(&v->data[index * v->elem_size]);
}

void *vec_at_s(Vec *v, size_t index)
{
    VALIDATE_VECTOR(v);
    if (index >= v->len)
    {
        return NULL;
    }
    return vec_at(v, index);
}

void vec_resize(Vec *v, size_t new_cap)
{
    VALIDATE_VECTOR(v);
    if (new_cap == v->capacity)
        return;
    if (!new_cap)
        new_cap++;
    byte *new_data = (byte *)realloc(v->data, new_cap * v->elem_size * sizeof(byte));
    VEC_ASSERT(new_data != NULL && "vec_resize: Failed to resize vec array.");

    /*  Initialize the newly allocated memory */
    size_t old_cap_start = v->capacity * v->elem_size * sizeof(byte);
    if (old_cap_start < new_cap)
    {
        size_t region_len = new_cap - v->capacity;
        memset(new_data + old_cap_start, 0, region_len * v->elem_size * sizeof(byte));
    }
    v->capacity = new_cap;
    v->data = new_data;
}

void vec_push_back(Vec *v, void *data)
{
    VALIDATE_VECTOR(v);
    if (!data)
        return;
    if (v->len >= v->capacity)
        vec_resize(v, v->grow(v));
    memcpy(vec_at(v, v->len), data, v->elem_size * sizeof(byte));
    v->len++;
}

void vec_sort(Vec *v)
{
    VALIDATE_VECTOR(v);
    if (!v->cmp)
    {
        perror("vec_sort: Compare function is undefined.");
        return;
    }
    qsort(v->data, v->len, v->elem_size, v->cmp);
}

void vec_insert(Vec *v, size_t index, void *data)
{
    VALIDATE_VECTOR(v);
    if (index > v->len)
        return;

    if (v->len >= v->capacity)
        vec_resize(v, v->grow(v));

    memmove(vec_at(v, index + 1), vec_at(v, index), (v->len - index) * v->elem_size * sizeof(byte));
    memcpy(vec_at(v, index), data, v->elem_size);

    v->len++;
}

void vec_clear(Vec *v)
{
    VALIDATE_VECTOR(v);
    if (v->free_entry)
    {
        void *var;
        V_FOR_EACH_ANSI(v, var)
        {
            v->free_entry(var);
        }
    }
    memset(v->data, 0, v->capacity * v->elem_size * sizeof(byte));
    v->len = 0;
}

/*  void vec_clear(Vec *v) */
/*  { */
/*      VALIDATE_VECTOR(v); */
/*      memset(v->data, 0, v->capacity * v->elem_size * sizeof(byte)); */
/*      v->len = 0; */
/*  } */

void vec_clamp(Vec *v)
{
    VALIDATE_VECTOR(v);
    byte *tmp;
    if (!v->len)
        tmp = (byte *)realloc(v->data, v->elem_size * sizeof(byte));
    else
        tmp = (byte *)realloc(v->data, v->len * v->elem_size * sizeof(byte));
    VEC_ASSERT(tmp);
    v->data = tmp;
    v->capacity = v->len;
}

void *vec_pop_back(Vec *v)
{
    VALIDATE_VECTOR(v);
    if (v->len < 1)
        return NULL;
    return vec_at(v, --v->len);
}

void *vec_find(Vec *v, void *_find)
{
    VALIDATE_VECTOR(v);
    if (!v->cmp)
    {
        perror("vec_find: Compare function is undefined.");
        return NULL;
    }
    size_t i;
    for (i = 0; i < v->len; i++)
    {
        if (v->cmp(vec_at(v, i), _find) == 0)
        {
            return vec_at(v, i);
        }
    }
    return NULL;
}

size_t vec_find_idx(Vec* v, void* _find)
{
    VALIDATE_VECTOR(v);
    if (!v->cmp)
    {
        perror("vec_find_idx: Compare function is undefined.");
        return INVALID_FE_IDX;
    }
    size_t i;
    for (i = 0; i < v->len; i++)
    {
        if (v->cmp(vec_at(v, i), _find) == 0)
        {
            return i;
        }
    }
    return INVALID_FE_IDX;
}

void vec_remove(Vec *v, size_t index)
{
    VALIDATE_VECTOR(v);
    if (index >= v->len)
        return;
    if (v->fe_idx != INVALID_FE_IDX)
    {
        v->fe_idx--;
    }
    if (index < v->len - 1)
    {
        memmove(vec_at(v, index), vec_at(v, index + 1), (v->len - index - 1) * v->elem_size * sizeof(byte));
    }

    v->len--;
}

void vec_remove_fast(Vec *v, size_t index)
{
    VALIDATE_VECTOR(v);
    if (index >= v->len)
        return;
    if (v->fe_idx != INVALID_FE_IDX) /* allows V_FOR_EACH to remove entries */ 
    {
        v->fe_idx--;
    }
    memcpy(vec_at(v, index), vec_at(v, v->len - 1), v->elem_size * sizeof(byte));
    v->len--;
}

/* Returns heap allocated deep copy */
Vec *vec_copy(Vec *v)
{
    VALIDATE_VECTOR(v);
    Vec *ret = (Vec *)malloc(sizeof(Vec));
    VEC_ASSERT(ret);
    ret->grow = v->grow;
    ret->cmp = v->cmp;
    ret->elem_size = v->elem_size;
    ret->len = 0;
    ret->data = NULL;
    ret->capacity = 0;
    vec_resize(ret, v->capacity);
    if (v->capacity == 0)
    {
        return ret;
    }
    VEC_ASSERT(ret->data && ret->capacity == v->capacity);
    memcpy(ret->data, v->data, v->capacity);
    return ret;
}

void vec_reverse(Vec *v)
{
    VALIDATE_VECTOR(v);
    size_t i;
    for (i = 0; i < v->len / 2; i++)
    {
        vec_swap(v, i, v->len - i - 1);
    }
}

void vec_swap(Vec *v, size_t idx0, size_t idx1)
{
    VALIDATE_VECTOR(v);
    if (idx0 >= v->len)
        return;
    if (idx1 >= v->len)
        return;

    size_t write_size = v->elem_size;
    byte *idx0_entry = vec_at(v, idx0);
    byte *idx1_entry = vec_at(v, idx1);
    while (write_size)
    {
        size_t idx = write_size - 1;
        byte tmp = idx0_entry[idx];
        idx0_entry[idx] = idx1_entry[idx];
        idx1_entry[idx] = tmp;
        write_size--;
    }
}

void *vec_pop_front(Vec *v)
{
    VALIDATE_VECTOR(v);
    if (v->len < 1)
        return NULL;
    void *ret = vec_at(v, 0);
    vec_remove(v, 0);
    return ret;
}

size_t vec_size(Vec *v)
{
    return v->len;
}

int vec_append(Vec *dest, Vec *source)
{
    if (dest->elem_size != source->elem_size)
        return 1;
    size_t i;
    for (i = 0; i < source->capacity; i++)
    {
        void *data = vec_at_s(source, i);
        if (!data)
            return 2;
        vec_push_back(dest, data);
    }
    return 0;
}

void *vec_arr_copy(Vec *v, size_t *ret_elem_count)
{
    if (!(v))
        return NULL;
    void *copy = malloc(v->elem_size * v->len);
    if (ret_elem_count)
        *ret_elem_count = v->len;
    return memcpy(copy, v->data, v->len * v->elem_size);
}