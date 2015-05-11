/*
 * Author: Allen Haim <allen@netherrealm.net>, © 2015.
 * Source: github.com/misterfish/fish-lib-util
 * Licence: GPL 2.0
 * Version: 0.4
 */

#ifndef __FISH_UTILS_H

#define __FISH_UTILS_H 1

#define mlc(a) malloc(sizeof(a));

#define MAX_BUF 1000

#include <string.h>
#include <stdlib.h>

/* DEBUG is globally exported. Niet handig. XX
 */

/*
// be polite
#ifdef DEBUG
 #define SAVE_DEBUG
#else
 #define SAVE_UNDEBUG
#endif
*/

/* Define/undef here:
#define DEBUG
 */


#include <fish-util.h>

/*
#ifdef SAVE_DEBUG
 #define DEBUG
#endif

#ifdef SAVE_UNDEBUG
 #undef DEBUG
#endif
*/

#include "fish-utils/vec.h"
#include "fish-utils/regex.h"

void f_track_heap(void *ptr);
void fish_utils_init();
void fish_utils_cleanup();

#endif
