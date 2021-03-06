/*
 * Author: Allen Haim <allen@netherrealm.net>, © 2015.
 * Source: github.com/misterfish/fish-lib-util
 * Licence: GPL 2.0
 */

#define VEC_DESTROY_DEEP    0x01
#define VEC_CLEAR_DEEP      0x02

typedef struct vec {
    int n;
    int cap;
    void **_data;
} vec;

vec *vec_new();
int vec_size(vec *v);
bool vec_add(vec *v, void *ptr);
void *vec_get(vec *v, int n);
void *vec_last(vec *v);

/* vec_destroy should be void, while _deep and _f should be bool.
 * XX
 */
bool vec_destroy(vec *v);
bool vec_destroy_deep(vec *v);
bool vec_destroy_f(vec *v, int flags);
bool vec_clear_f(vec *v, int flags);
bool vec_clear(vec *v);
