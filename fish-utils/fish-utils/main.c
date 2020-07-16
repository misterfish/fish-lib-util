/*
 * Author: Allen Haim <allen@netherrealm.net>, © 2015.
 * Source: github.com/misterfish/fish-lib-util
 * Licence: GPL 2.0
 */

#define _GNU_SOURCE

#include "../fish-utils.h"

static vec *_fish_utils_heap = NULL;

void fish_utils_init() {
    _fish_utils_heap = vec_new();
}

void f_track_heap(void *ptr) {
    if (! vec_add(_fish_utils_heap, ptr))
        piep;
}

void fish_utils_cleanup() {
    if (! _fish_utils_heap)
        piepr;
    if (!vec_destroy_f(_fish_utils_heap, VEC_DESTROY_DEEP))
        piepr;
}
