// here, not header, so not exported
#define _GNU_SOURCE

#include "../fish-utils.h"

static vec *_fish_utils_heap;

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
// XX !
vec_destroy_flags(_fish_utils_heap, VEC_DESTROY_DEEP);
}
