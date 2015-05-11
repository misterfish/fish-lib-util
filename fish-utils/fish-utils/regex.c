/*
 * Author: Allen Haim <allen@netherrealm.net>, © 2015.
 * Source: github.com/misterfish/fish-lib-util
 * Licence: GPL 2.0
 */

#define _GNU_SOURCE

#include <pcre.h> // local

#include "../fish-utils.h"

/* In all functions where ret is filled with matches, the caller should free
 * each element, including ret[0] (the whole match).
 */
int match(char *target, char *regexp_s) {
    return match_full(target, regexp_s, NULL, 0, F_REGEX_DEFAULT);
}

// ret[0] is whole match.
int match_matches(char *target, char *regexp_s, char *ret[]) {
    return match_full(target, regexp_s, ret, 0, F_REGEX_DEFAULT);
}

int match_matches_flags(char *target, char *regexp_s, char *ret[], int flags) {
    return match_full(target, regexp_s, ret, 0, flags);
}

int match_flags(char *target, char *regexp_s, int flags) {
    return match_full(target, regexp_s, NULL, 0, flags);
}

// -> bool XX
int match_full(char *target, char *regexp_s, char *ret[], int target_len /* without \0 */, int flags) {

    int idx = -1;
    int rc;

    int pass_flags = 0;
    if (flags & F_REGEX_EXTENDED)
        pass_flags |= PCRE_EXTENDED;

    bool auto_gc = (flags & F_REGEX_NO_FREE_MATCHES) ? false : true;

    int erroffset;
    const char *error; 
    pcre *re = pcre_compile(
            regexp_s,
            pass_flags,
            &error, // static, don't free
            &erroffset,
            NULL // char tables
            );

    if (!re) {
        _();
        BR(regexp_s);
        iwarn("Error compiling regex %s (%s)", _s, error);
        return false;
    }

    int num_groups;
    if ((rc = pcre_fullinfo(re, 
            NULL, // no study
            PCRE_INFO_CAPTURECOUNT,
            &num_groups
            ))) { 
        _();
        BR(regexp_s);
        /* man pcreapi */
        char *msg;
        switch(rc) {
            case PCRE_ERROR_NULL:
                msg = "'code' or 'where' was null";
                break;
            case PCRE_ERROR_BADMAGIC:
                msg = "magic number not found"; // ?
                break;
                /*
                 * older pcre doesn't have this
            case PCRE_ERROR_BADENDIANNESS:
                msg = "the pattern was compiled with different endian-ness";
                break;
                */
            case PCRE_ERROR_BADOPTION:
                msg = "bad option given (value of 'what' was invalid)";
                break;
                /*
                 * older pcre doesn't have this
            case PCRE_ERROR_UNSET:
                msg = "the requested field is not set";
                break;
                */
            default:
                msg = "unknown error";
        }
        iwarn("Error analysing pattern %s (%s)", _s, msg);
        pcre_free(re);
        return false;
    }

    // first two thirds are the pairs, last third is reserved; see man.
    int ovector_size = (num_groups+1) * 3;
    int ovector[ovector_size];

    if (!target_len) {
        //target_len = 1 + strnlen(target, MAX_BUF);
        target_len = strnlen(target, MAX_BUF);
    }

    rc = pcre_exec(
        re,             /* result of pcre_compile() */
        NULL,           /* we didn't study the pattern */
        target,  /* the subject string */
        target_len,             /* the length of the subject string, not counting \0 */
        0,              /* start at offset 0 in the subject */
        0,              /* default options */
        ovector,        /* vector of integers for substring information */
        ovector_size            /* number of elements (NOT size in bytes) */
    );

    if (rc < 0) { 
        if (rc == PCRE_ERROR_NOMATCH) {
            // ok, return false
        }
        else {
            _();
            spr("%d", rc);
            R(_s);
            warn("Error matching regex: %s (pcre.h)", _t);
        }
        pcre_free(re);
        return false;
    }

    int num_matches = rc;

    // ovector[0] and ovector[1] are the endpoints of the whole match.
    // right endpt is not inclusive.

    int a = 0;
    int b = 1;
    for (int match_num = 0; match_num < num_matches; match_num++) {
        if (ovector[a] >= 0) {
            int l = ovector[a];
            int r = ovector[b];
            char *match = str(r - l + 1);
            memcpy(match, target + l, r - l); 
            if (ret) 
                ret[++idx] = match; 

            /* match'es will be freed when fish_utils_cleanup() is
             * called. convenient but footprint will get big if program does
             * lots of matches.
             */
            if (auto_gc)
                f_track_heap(match);
        }
        a += 2;
        b += 2;
    }

    pcre_free(re);

    return true;
}

