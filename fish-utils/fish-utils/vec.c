/*
 * Author: Allen Haim <allen@netherrealm.net>, © 2015.
 * Source: github.com/misterfish/fish-lib-util
 * Licence: GPL 2.0
 */

#define _GNU_SOURCE

#include "../fish-utils.h"

#define mlc(a) malloc(sizeof(a));

#define VEC_CAP 10

vec *vec_new() {
    vec *v = mlc(vec)
    if (!v) {
        _();
        R(perr());
        warn("Couldn't make new vector: %s", _s);
        return NULL;
    }
    v->n = 0;
    v->cap = VEC_CAP;
    v->_data = calloc(VEC_CAP, sizeof(void*));
    return v;
}

bool vec_add(vec *v, void *ptr) {
    if (v == NULL)
        pieprf;

    int i = ++(v->n);
    if (i > v->cap) {
        int newcap = v->cap * 2;

        _();
        spr("%d", newcap);
        G(_s);
        debug("vec %p: reallocating: size -> %s", v, _t);

        void **new = realloc(v->_data, newcap * sizeof(void*));

        if (!new) {
            _();
            R(perr());
            warn("Couldn't resize vector: %s", _s);
            return false; // keeping v->data as it was
        }

        memset(new + v->cap, 0, v->cap * sizeof(void*));

        v->_data = new;
        v->cap = newcap;
    }
    v->_data[v->n - 1] = ptr;
    return true;
}

int vec_size(vec *v) {
    if (v == NULL)
        pieprneg1;
    return v->n;
}

void *vec_get(vec *v, int n) {
    if (! v) {
        warn("Null pointer passed to vec_get");
        return NULL;
    }
    if (n < 0)
        pieprnull;
    if (n > v->n - 1)
        pieprnull;
    return v->_data[n];
}

void *vec_last(vec *v) {
    if (v == NULL)
        pieprnull;;
    return v->_data[v->n - 1];
}

bool vec_destroy_deep(vec *v) {
    return vec_destroy_f(v, VEC_DESTROY_DEEP);
}

bool vec_destroy_f(vec *v, int flags) {
    if (v == NULL)
        pieprf;
    if (flags && VEC_DESTROY_DEEP) {
        debug("deep destroy.", 1);
        if (!vec_clear_f(v, VEC_CLEAR_DEEP))
            pieprf;
    }
    free(v->_data);
    free(v);
    return true;
}

bool vec_destroy(vec *v) {
    return vec_destroy_f(v, 0);
}

bool vec_clear_f(vec *v, int flags) {
    for (int i = 0; i < v->n; i++) {
        void *ptr = v->_data[i];
        if (!ptr)
            continue;
        if (flags && VEC_CLEAR_DEEP) {
            debug("freeing %p", ptr);
            free(ptr);
        }
        v->_data[i] = NULL;
    }
    /* Vector will stay at its current size.
     * Might be nice to shrink it again to init size.
     * XX
     */
    v->n = 0;
    return true;
}

bool vec_clear(vec *v) {
    return vec_clear_f(v, 0);
}
