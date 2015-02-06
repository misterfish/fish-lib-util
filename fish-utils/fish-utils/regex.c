#define _GNU_SOURCE

#include <pcre.h> // local

#include "../fish-utils.h"

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

int match_full(char *target, char *regexp_s, char *ret[], int target_len /* without \0 */, int flags) {

    int idx = -1;
    int rc;

    int pass_flags;
    if (flags && F_REGEX_EXTENDED)
        pass_flags |= PCRE_EXTENDED;

    int erroffset;
    const char *error; 
    pcre *re = pcre_compile(
            regexp_s,
            pass_flags,
            &error, // static, don't free
            &erroffset,
            NULL // char tables
            );

    int num_groups;
    if (pcre_fullinfo(re, 
            NULL, // no study
            PCRE_INFO_CAPTURECOUNT,
            &num_groups
            )) { 
        piep;
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
            if (ret) ret[++idx] = match; // caller should free

            f_track_heap(match);
            //vec_add(_fish_utils_heap, match);
        }
        a += 2;
        b += 2;
    }

    pcre_free(re);

    return true;
}

