/*
 * Author: Allen Haim <allen@netherrealm.net>, © 2015.
 * Source: github.com/misterfish/fish-lib-util
 * Licence: GPL 2.0
 */

#define _GNU_SOURCE // here, not header, not exported

// Be careful, truncated cmds bad. XX
#define CMD_LENGTH 1000

/* For errors and warnings.
 */
#define COMPLAINT_LENGTH F_COMPLAINT_LENGTH
#define INFO_LENGTH 300

#define SOCKET_LENGTH_DEFAULT 100

#define STATIC_STR_LENGTH 200

// length of (longest) color escape
#define COLOR_LENGTH 5
#define COLOR_LENGTH_RESET 4

#define NUM_STATIC_STRINGS 8

#define unknown_sig(signal) "Signal number " #signal // stringify
#define signame_(n, d) do { \
    if (name) *name = n; \
    if (desc) *desc = d; \
} while (0)

#include <locale.h>

#include <ctype.h>  // isdigit
#include <signal.h>
// errno
#include <errno.h>

#include <sys/timeb.h>

// LONG_MIN
#include <limits.h>

#include <string.h>

#include <stdlib.h>
// varargs
#include <stdarg.h>

#include <stdio.h>
#include <sys/time.h>
#include <math.h>

#include <assert.h>

// dirname. 
#include <libgen.h>

// offsetof
#include <stddef.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <wchar.h>

/* stat */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> // isatty
/* */

#include "fish-util.h"

/* Private headers.
 */

static char **_get_static_str_ptr();
static char *_color(const char *s, int idx);
static void _static_str_init();
static void _color_static(const char *c);
static void _sys_say(const char *cmd);
static void _static_strings_free();

/* Public.
 */

char *_s, *_t, *_u, *_v, *_w, *_x, *_y, *_z;

/* Private.
 */

static char *warn_prefix;
static int warn_prefix_size = 0;

static const char *BULLETS[] = {"ঈ", "꣐", "⩕", "٭", "᳅", "𝄢", "𝄓", "𝄋", "𝁐", "ꢝ"};
static short NUM_BULLETS = sizeof(BULLETS) / sizeof(char*);

static struct stat *mystat;
static bool mystat_initted = false;

static int _static_str_idx = -1;
static bool _static_str_initted = false;
static char **_static_strs[NUM_STATIC_STRINGS];
static bool _static_strings_freed = false;

static char *COL[] = {
    // reset
    "[0m",
    // red
    "[31m",
    // bright red
    "[91m",
    // green
    "[32m",
    // bright green
    "[92m",
    // yellow
    "[33m",
    // bright yellow
    "[93m",
    // blue
    "[34m",
    // bright blue
    "[94m",
    // cyan
    "[36m",
    // bright cyan
    "[96m",
    // magenta
    "[35m",
    // bright magenta
    "[95m",
};

/* static by default.
 *
 * index to COL array.
 */
enum COLORS {
    RED=1,
    BRIGHT_RED,
    GREEN,
    BRIGHT_GREEN,
    YELLOW,
    BRIGHT_YELLOW,
    BLUE,
    BRIGHT_BLUE,
    CYAN,
    BRIGHT_CYAN,
    MAGENTA,
    BRIGHT_MAGENTA,
};

static int _disable_colors = 0;
static bool _die = false;
static bool _verbose = true;

/* Returns random double from [0, r).
 * Caller should check for overflows etc.
 * Don't use this function, ever, at all. Only sometimes.
 */
double get_random(int r) {
    struct timeval tv = {0};
    if (gettimeofday(&tv, NULL)) {
        warn_perr("Can't call gettimeofday");
        return -1;
    }
    srandom(tv.tv_sec * tv.tv_usec);
    // RAND_MAX ~ 31 bits
    long int rand = random(); 
    if (rand == RAND_MAX) 
        rand--;
    double rnd = rand * 1.0 / RAND_MAX * r;
    if (rnd == r) {
        iwarn("Error making random number.");
        return -1;
    }
    return rnd;
}

char *get_bullet() {
    return (char *) BULLETS[(int) get_random(NUM_BULLETS)];
}

static void oom_fatal() {
    fprintf(stderr, "Out of memory! (fish-util.c)");
    _exit(1);
}

/* Public functions.
 */

/* glibc version of dirname segfaults with static strings. 
 * This version does a strdup first.
 * Caller should not free.
 */
char *f_dirname(char *s) {
    if (!s) {
        iwarn("f_dirname(): null arg");
        return NULL;
    }
    char *t = f_strdup(s);
    char *ret = dirname(t);
    free(t);
    return ret;
}

void *f_malloc(size_t s) {
    void *ptr = malloc(s);
    if (!ptr && s) // NULL ok if s is 0
        oom_fatal();
    return ptr;
}

void *f_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr && nmemb && size) // NULL ok if size or nmemb is 0
        oom_fatal();
    return ptr;
}

void *f_realloc(void *ptr, size_t size) {
    void *new = realloc(ptr, size);
    if (!new && size) // NULL can mean size is 0
        oom_fatal();
    return new;
}

char *f_strdup(const char *s) {
    char *ret = strdup(s);
    if (!ret)
        oom_fatal();
    return ret;
}

char *f_strndup(const char *s, size_t n) {
    char *ret = strndup(s, n);
    if (!ret)
        oom_fatal();
    return ret;
}

/* init not necessary, unless you want to start over after having called
 * _cleanup. (And even then it's not (currently) necessary).
 */
void fish_util_init() {
    _static_str_init();
    /* Merge these? XX
     */
    _static_str_initted = true;
    _static_strings_freed = false;
}

void fish_util_cleanup() {
    if (!_static_strings_freed) {
        _static_strings_free();
        _static_strings_freed = true;
    }
    if (mystat_initted) {
        free(mystat);
        mystat_initted = false;
    }
}

/* Caller shouldn't free.
 */
FILE *safeopen_f(char *filespec, int flags) {
    FILE *f = NULL;

    char *filename = NULL;
    bool is_err = false;
    bool i_die = ! (flags & F_NODIE);
    bool quiet = flags & F_QUIET;
    bool utf8 = flags & F_UTF8;
    char *mode;
    char *mode_f = str(3 + 10 + 1); // max mode 3 long, including b, 10 for utf-8

    if (flags & F_WRITE) {
        mode = "writing";
        strcpy(mode_f, "w");
        filename = filespec;
    }
    else if (flags & F_READ_WRITE_NO_TRUNC) {
        mode = "read-write, no create or truncate";
        strcpy(mode_f, "r+");
        filename = filespec;
    }
    else if (*filespec == '+') {
        mode = "read-write, no create or truncate";
        strcpy(mode_f, "r+");
        filename = filespec + 1;
    }
    else if (
            (*filespec == '<' && *(filespec+1) == '+') ||
            (*filespec == '+' && *(filespec+1) == '<')
        ) {
        mode = "read-write, no create or truncate";
        strcpy(mode_f, "r+");
        filename = filespec + 2;
    }
    else if (flags & F_READ_WRITE_TRUNC) {
        mode = "read-write, create or truncate";
        strcpy(mode_f, "w+");
        filename = filespec;
    }
    else if (
            (*filespec == '>' && *(filespec+1) == '+') ||
            (*filespec == '+' && *(filespec+1) == '>') 
        ) {
        mode = "read-write, create or truncate";
        strcpy(mode_f, "w+");
        filename = filespec + 2;
    }
    else if (*filespec == '>' && *(filespec+1) == '>') {
        mode = "appending";
        strcpy(mode_f, "a");
        filename = filespec + 2;
    }
    else if (*filespec == '>') {
        mode = "writing";
        strcpy(mode_f, "w");
        filename = filespec + 1;
    }
    else if (flags & F_APPEND) {
        mode = "appending";
        strcpy(mode_f, "a");
        filename = filespec;
    }
    else if (flags & F_READ) {
        mode = "reading";
        strcpy(mode_f, "r");
        filename = filespec;
    }
    else if (*filespec == '<') {
        mode = "reading";
        strcpy(mode_f, "r");
        filename = filespec + 1;
    }
    else {
        mode = "reading";
        strcpy(mode_f, "r");
        filename = filespec;
    }

    if (utf8) {
        sprintf(mode_f, "%s,ccs=UTF-8", mode_f); // NO space after comma, before is ok
    }

    if (f_test_d(filename)) {
        _();
        Y(filename);
        iwarn("File %s is a directory (can't open)", _s);
        is_err = true;
    }
    else {
        f = fopen(filename, mode_f);
        if (!f) {
            int en = errno;
            is_err = true;
            if (!quiet) {
                _();
                Y(filename);
                CY(mode);
                errno = en;
                iwarn("Couldn't open %s for %s: %s", _s, _t, perr());
            }
        }
    }

    free(mode_f);

    if (is_err) {
        if (i_die) {
            exit(1);
        }
        else {
            return NULL;
        }
    }
    return f;
}

FILE *safeopen(char *filespec) {
    return safeopen_f(filespec, 0);
}

// generic n XX
// make global XX
void _static_strings_save_restore(int which) {
    if (! _static_str_initted) return;

    static char *saves, *savet, *saveu, *savev, *savew;
    static bool saved = false;
    if (which == 0) {
        saves = f_malloc(sizeof(char) * STATIC_STR_LENGTH);
        savet = f_malloc(sizeof(char) * STATIC_STR_LENGTH);
        saveu = f_malloc(sizeof(char) * STATIC_STR_LENGTH);
        savev = f_malloc(sizeof(char) * STATIC_STR_LENGTH);
        savew = f_malloc(sizeof(char) * STATIC_STR_LENGTH);
        memcpy(saves, _s, STATIC_STR_LENGTH);
        memcpy(savet, _t, STATIC_STR_LENGTH);
        memcpy(saveu, _u, STATIC_STR_LENGTH);
        memcpy(savev, _v, STATIC_STR_LENGTH);
        memcpy(savew, _w, STATIC_STR_LENGTH);

        saved = true;
    }
    else { // restore
        if (!saved) return;

        memcpy(_s, saves, STATIC_STR_LENGTH);
        memcpy(_t, savet, STATIC_STR_LENGTH);
        memcpy(_u, saveu, STATIC_STR_LENGTH);
        memcpy(_v, savev, STATIC_STR_LENGTH);
        memcpy(_w, savew, STATIC_STR_LENGTH);

        saved = false;
    }

}

void _static_strings_restore() { 
    _static_strings_save_restore(1);
}

void _static_strings_save() {
    _static_strings_save_restore(0);
}

/* Caller should free.
 */
char *f_get_warn_prefix(char *file, int line) {
    _static_strings_save();

    _();
    spr("%d", line);
    Y(_s);

    int len = strlen(file) + 1 + strlen(_t) + 1 + 1;
    char *warn_prefix = f_malloc(len * sizeof(char));

    sprintf(warn_prefix, "%s:%s", file, _t);

    _static_strings_restore();

    return warn_prefix;
}


bool f_test_f(const char *file) {
    struct stat *s = f_stat_f(file, F_QUIET);
    if (!s) 
        return false;
    int mode = s->st_mode;
    if (!mode) 
        return false;

    return S_ISREG(mode);
}

bool f_test_d(const char *file) {
    struct stat *s = f_stat_f(file, F_QUIET);
    if (!s) 
        return false;
    int mode = s->st_mode;
    if (!mode) 
        return false;

    return S_ISDIR(mode);
}

/* Take from the heap, probably slower than normal stat.
 */
struct stat *f_stat_f(const char *file, int flags) {
    if (!mystat_initted) {
        mystat = f_mallocv(*mystat);
        mystat_initted = true;
    }
    memset(mystat, 0, sizeof(*mystat));

    bool quiet = flags & F_QUIET;

    if (-1 == stat(file, mystat)) {
        int en = errno;
        if (!quiet) {
            _();
            Y(file);
            errno = en;
            warn_perr("Couldn't stat %s: %s", _s);
        }
        return NULL;
    }
    return mystat;
}

struct stat *f_stat(const char *file) {
    return f_stat_f(file, 0);
}

void f_disable_colors() {
    _disable_colors = 1;
}

/* String with all nulls (can't use strlen).
 * length includes \0.
 * Caller should free.
 */
char *str(int length) {
    assert(length > 0);
    char *s = f_malloc(length * sizeof(char));
    memset(s, '\0', length);
    return s;
}

/* Is null-terminated, even if size too small.
 */
void spr(const char *format, ...) {
    int size = STATIC_STR_LENGTH; // with \0
    char *s = str(size); // \0
    va_list arglist;
    va_start( arglist, format );
    vsnprintf(s, size, format, arglist);
    va_end( arglist );

    int l = strlen(s);
    if (l > STATIC_STR_LENGTH - 1) {
        warn("static string truncated (%s)", s);
        l = STATIC_STR_LENGTH - 1;
    }
    char **ptr = _get_static_str_ptr();
    memcpy(*ptr, s, l + 1);
    free(s);
}

/* Caller should free.
 */
char *spr_(const char *format, int size /*with null*/, ...) {
    char *s = str(size);
    va_list arglist;
    va_start( arglist, size );
    vsnprintf(s, size, format, arglist);
    va_end( arglist );
    return s;
}

void _() {
    if (! _static_str_initted) {
        _static_str_init();
        _static_str_initted = true;
    }

    _static_str_idx = -1;
    for (int i = 0; i < NUM_STATIC_STRINGS; i++) {
        memset(*_static_strs[i], '\0', STATIC_STR_LENGTH);
    }
}

char *R_(const char *s) {
    return _color(s, RED);
}
char *BR_(const char *s) {
    return _color(s, BRIGHT_RED);
}
char *G_(const char *s) {
    return _color(s, GREEN);
}
char *BG_(const char *s) {
    return _color(s, BRIGHT_GREEN);
}
char *Y_(const char *s) {
    return _color(s, YELLOW);
}
char *BY_(const char *s) {
    return _color(s, BRIGHT_YELLOW);
}
char *B_(const char *s) {
    return _color(s, BLUE);
}
char *BB_(const char *s) {
    return _color(s, BRIGHT_BLUE);
}
char *CY_(const char *s) {
    return _color(s, CYAN);
}
char *BCY_(const char *s) {
    return _color(s, BRIGHT_CYAN);
}
char *M_(const char *s) {
    return _color(s, MAGENTA);
}
char *BM_(const char *s) {
    return _color(s, BRIGHT_MAGENTA);
}

void R(const char *s) {
    char *c = _color(s, RED);
    _color_static(c);
    free(c);
}

void BR(const char *s) {
    char *c = _color(s, BRIGHT_RED);
    _color_static(c);
    free(c);
}

void G(const char *s) {
    char *c = _color(s, GREEN);
    _color_static(c);
    free(c);
}

void BG(const char *s) {
    char *c = _color(s, BRIGHT_GREEN);
    _color_static(c);
    free(c);
}

void Y(const char *s) {
    char *c = _color(s, YELLOW);
    _color_static(c);
    free(c);
}

void BY(const char *s) {
    char *c = _color(s, BRIGHT_YELLOW);
    _color_static(c);
    free(c);
}

void B(const char *s) {
    char *c = _color(s, BLUE);
    _color_static(c);
    free(c);
}

void BB(const char *s) {
    char *c = _color(s, BRIGHT_BLUE);
    _color_static(c);
    free(c);
}

void CY(const char *s) {
    char *c = _color(s, CYAN);
    _color_static(c);
    free(c);
}

void BCY(const char *s) {
    char *c = _color(s, BRIGHT_CYAN);
    _color_static(c);
    free(c);
}

void M(const char *s) {
    char *c = _color(s, MAGENTA);
    _color_static(c);
    free(c);
}

void BM(const char *s) {
    char *c = _color(s, BRIGHT_MAGENTA);
    _color_static(c);
    free(c);
}

/* Returns either a static pointer to a string (don't free it), in which
 * case buf is unused, or a pointer to the string stored in buf. In the
 * second case, does it need to be freed? XX
 */

const char *perr() {
    char *st = str(100);
    char *ret = strerror_r(errno, st, 100);
    if (*st == '\0') { 
        // buffer unused
        free(st);
        return ret;
    }
    else {
        iwarn("fixme: perr stored string in buf, possible leak.");
        free(st);
        return ret;
    }
}

char *add_nl(const char *orig) {
    // not including null
    int len = strlen(orig);
    char *new = f_malloc(sizeof(char) * (len + 2));
    memset(new, '\0', len + 2);
    // no null
    strncpy(new, orig, len);
    // adds null
    strcat(new, "\n");
    return new;
}

void say(const char *format, ...) {
    char *new = add_nl(format);
    va_list arglist;
    va_start( arglist, format );
    vprintf( new, arglist );
    va_end( arglist );
    free(new);
}

void ask(const char *format, ...) {
    char *new = str(INFO_LENGTH);
    char *new2 = str(INFO_LENGTH + 1);
    va_list arglist, arglist_copy;
    va_start( arglist, format );
    va_copy(arglist_copy, arglist);
    // get rid of new2, just check rc XX
    vsnprintf( new, INFO_LENGTH, format, arglist );
    vsnprintf( new2, INFO_LENGTH + 1, format, arglist_copy );
    if (strncmp(new, new2, INFO_LENGTH)) { // no + 1 necessary
        warn("Ask string truncated.");
    }
    va_end( arglist );

    char *bullet = get_bullet();
    char *question = str(strlen(new) + COLOR_LENGTH + COLOR_LENGTH_RESET + strlen(bullet) + 1 + 1 + 1 + 1);
    char *c = M_(bullet);
    sprintf(question, "%s %s? ", c, new);
    printf(question);
    free(c);
    free(question);

    free(new);
    free(new2);
}

void info(const char *format, ...) {
    char *new = str(INFO_LENGTH);
    char *new2 = str(INFO_LENGTH + 1);
    va_list arglist;
    va_list arglist_copy;
    va_start( arglist, format );
    va_copy(arglist_copy, arglist);
    // get rid of new2, just check rc XX
    vsnprintf( new, INFO_LENGTH, format, arglist );
    vsnprintf( new2, INFO_LENGTH + 1, format, arglist_copy );
    if (strncmp(new, new2, INFO_LENGTH)) { // no + 1 necessary
        warn("Info string truncated.");
    }
    va_end( arglist );

    char *bullet = get_bullet();
    char *infos = str(strlen(new) + COLOR_LENGTH + COLOR_LENGTH_RESET + strlen(bullet) + 1 + 1 + 1);
    char *c = BB_(bullet);
    sprintf(infos, "%s %s\n", c, new);
    printf(infos);
    free(c);
    free(infos);

    free(new);
    free(new2);
}

void _err() {
    fish_util_cleanup();
    exit(1);
}

void f_sys_die(bool b) {
    _die = b;
}

void f_verbose_cmds(bool b) {
    _verbose = b;
}

/* Non-null FILE means fork or pipe succeeded, but command could still fail
 * (e.g. command not found).
 * Caller should check for NULL, and if non-null, call sysclose to finish
 * (not free()).
 */
FILE *sysr(const char *cmd) {
    if (_verbose) _sys_say(cmd);
    FILE *f = popen(cmd, "r");
    errno = -1;
    if (!f) {
        /* memory allocation failed (errno doesn't get set).
         */
        if (errno == -1)
            oom_fatal();
        int e = errno;
        _();
        BR(cmd);
        spr("Can't launch cmd (%s) for reading, fork or pipe failed.", _s);
        errno = e;
        if (_die) 
            err_perr(_t);
        else 
            warn_perr(_t);
    }
    return f;
}

/* Non-null FILE means fork or pipe succeeded, but command could still fail
 * (e.g. command not found).
 * Caller should check for NULL, and if non-null, call sysclose to finish
 * (not free()).
 */
FILE *sysw(const char *cmd) {
    if (_verbose) _sys_say(cmd);
    errno = -1;
    FILE *f = popen(cmd, "w");
    if (f == NULL) {
        /* memory allocation failed (errno doesn't get set).
         */
        if (errno == -1)
            oom_fatal();
        int e = errno;
        _();
        BR(cmd);
        spr("Can't launch cmd (%s) for reading, fork or pipe failed.", _s);
        errno = e;
        if (_die) 
            err_perr(_t);
        else 
            warn_perr(_t);
    }
    return f;
}

/* 0 means good, -1 means wait or another sys call failed, > 0 is a non-zero
 * exit from the command.
 * The cmd arg is optional -- with it we can make a nicer error message.
 *
 * This will print an error message if a popen'ed command is closed while
 * there's still data in the buffer. Use flag F_QUIET to silence this.
 */
int sysclose_f(FILE *f, const char *cmd, int flags) {
    bool quiet = flags && F_QUIET;
    int status = pclose(f);
    if (status) {
        int en = errno;
        _();
        if (cmd && *cmd != '\0') {
            BR(cmd);
            spr(" «%s»", _s);
        }
        else {
            spr("");
            spr("");
        }
        /* On linux and most unices, the 7 least significant bits contain
         * the signal, and the 8th tells us if we core dumped, if it was
         * killed by a signal.
         * We will assume we are on one of those systems.
         */
        int signal = status & 0x7F;
        int core_dumped = status & 0x80;
        if (signal) {
            char *name;
            f_signame(signal, &name, NULL);
            CY(name);
            spr(" «got signal %s»", _u);
        }
        else {
            spr("");
            spr("");
        }
        if (core_dumped) {
            M("core dumped");
            spr(" «%s»", _w);
        }
        else {
            spr("");
            spr("");
        }
        
        status >>= 8;
        spr("%d", status);
        Y(_y);
        // XX
        char *msg = spr_("Error closing cmd%s «exit status %s»%s%s.", 300, _t, _z, _v, _x);
        if (status == -1) {
            errno = en;
            if (_die) 
                err_perr(msg);
            else if (!quiet) 
                warn_perr(msg);
        }
        else {
            if (_die) 
                err(msg);
            else if (!quiet) 
                warn(msg);
        }
        free(msg);
    }
    return status;
}

int sysclose(FILE *f) {
    return sysclose_f(f, NULL, 0);
}

/* Run command and read input until EOF.
 * Returns the cmd status, or -1 on failure.
 * To not read the input, use sysr.
 */
int sys(const char *cmd) {
    FILE *f = sysr(cmd);
    /* Leave complaining to sysr.
     */
    if (!f)
        return -1;

    int rc;
    char buf[100];
    errno = -1;
    while (fgets(buf, 100, f)) {
    }
    if (ferror(f)) {
        /* Not clear whether fgets is required to set errno.
         */
        if (errno != -1) 
            warn_perr("Interrupted read from command");
        else 
            warn("Interrupted read from command");
        return -1;
    }

    return sysclose_f(f, cmd, 0);
}

bool f_sig(int signum, void *func) {
    /* Ok that it's thrown away.
     */
    struct sigaction action = {0};
    action.sa_handler = func;
    int rc = sigaction(signum, &action, NULL);
    if (rc) {
        int en = errno;
        _();
        errno = en;
        spr("%s", perr());
        spr("%d", signum);
        R(_t);
        warn("Couldn't attach signal %s: %s", _u, _s);
        return false;
    }
    return true;
}

void f_autoflush() {
    setvbuf(stdout, NULL, _IONBF, 0);
}

void f_benchmark(int flag) {

    static int sec;
    static int usec;

    struct timeval *tv = f_malloc(sizeof *tv); 

    if (flag == 0) {
        gettimeofday(tv, NULL);
        sec = tv->tv_sec;
        usec = tv->tv_usec;
    }
    else {
        gettimeofday(tv, NULL);

        int delta_s = tv->tv_sec - sec;
        int delta_us = tv->tv_usec - usec;
        if (delta_us < 0) {
            delta_s--;
            delta_us += 1e6;
        }
        float delta = delta_s + delta_us / 1e6;
        _();
        spr("%10.3f", delta);
        G(_s);
        warn("Benchmark: %s s", _t);

        sec = -1;
        usec = -1;
    }
}

bool f_atod(char *s, double *ret) {
    char *endptr;
    errno = 0;
    double d = strtod(s, &endptr);
    if (errno || (endptr - s != strlen(s)))
        return false;
    if (!ret)
        return false;
    *ret = d;
    return true;
}

bool f_atoi(char *s, int *ret) {
    char *endptr;
    errno = 0;
    int i = strtol(s, &endptr, 10);
    if (errno || (endptr - s != strlen(s)))
        return false;
    if (!ret)
        return false;
    *ret = i;
    return true;
}

int f_int_length(int i) {
    if (i == 0) return 1;
    if (i < 0) return f_int_length(-1 * i);

    return 1 + (int) log10(i);
}

bool f_socket_make_named (const char *filename, int *socknum) {
    struct sockaddr_un name;
    size_t size;
  
    // 0 for protocol
    int sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock < 0) {
        int en = errno;
        _();
        Y(filename);
        errno = en;
        warn("Can't make socket %s: %s", _s, perr());
        return false;
    }

    name.sun_family = AF_UNIX;
    strncpy (name.sun_path, filename, sizeof (name.sun_path));
    name.sun_path[sizeof(name.sun_path) - 1] = '\0';
  
    /* The size of the address is
       the offset of the start of the filename,
       plus its length,
       plus one for the terminating null byte.
       Alternatively you can just do:
       size = SUN_LEN (&name);
   */
    size = (offsetof (struct sockaddr_un, sun_path) + strlen (name.sun_path) + 1);
  
    if (bind (sock, (struct sockaddr *) &name, size) < 0) {
        int en = errno;
        _();
        Y(filename);
        errno = en;
        warn("Can't bind socket %s (%s)", _s, perr());
        return false;
    }
  
    *socknum = sock;
    return true;
}

bool f_socket_make_client(int socket, int *client_socket) {
    /* Apparently ok to throw away.
     */
    struct sockaddr a;
    socklen_t t = sizeof(a);
    int cs = accept(socket, &a, &t);
    if (cs == -1) {
        int en = errno;
        _();
        spr("%d", socket);
        Y(_s);
        errno = en;
        warn("Couldn't make client socket for socknum %s (%s)", _t, perr());
        return false;
    }
    if (client_socket != NULL) 
        *client_socket = cs;
    return true;
}

bool f_socket_read(int client_socket, ssize_t *num_read, char *buf, size_t max_length) {
    ssize_t rc = recv(client_socket, buf, max_length, 0); // blocking
    if (rc == -1) {
        int en = errno;
        _();
        spr("%d", client_socket);
        Y(_s);
        errno = en;
        warn("Couldn't read from socket %s (%s)", _t, perr());
        return false;
    }
    if (num_read != NULL) 
        *num_read = rc;
    return true;
}

bool f_socket_write(int client_socket, ssize_t *num_written, const char *buf, size_t len) {
    ssize_t rc = write(client_socket, buf, len);
    if (rc == -1) {
        int en = errno;
        _();
        spr("%d", client_socket);
        Y(_s);
        errno = en;
        warn("Couldn't write to socket %s (%s)", _t, perr());
        return false;
    }

    if (num_written != NULL) 
        *num_written = rc;

    return true;
}

bool f_socket_close(int client_socket) {
    if (close(client_socket)) {
        int en = errno;
        _();
        spr("%d", client_socket);
        Y(_s);
        errno = en;
        warn("Couldn't close client socket %s (%s)", _t, perr());
        return false;
    }
    return true;
}

/* strlen filename, strlen msg.
 * buf_length includes \0.
 * msg checked against buf_length.
 * response not checked for size -- caller should ssure it is buf_length
 * bytes big.
 */
bool f_socket_unix_message_f(const char *filename, const char *msg, char *response, int buf_length) {
    struct sockaddr_un serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    bool trash_response = response == NULL;

    int msg_len = strlen(msg); // -O- trust caller
    int filename_len = strlen(filename); // -O- trust caller

    // both leak on early return XX
    char *filename_color = CY_(filename);
    char *msg_s = str(msg_len + 1 + 1);

    sprintf(msg_s, "%s\n", msg);

    if (strlen(msg) >= buf_length) {
        _();
        spr("%d", buf_length - 1);
        CY(_s);

        iwarn("msg too long (max is %s)", _t);
        return false;
    }

    // 0 means choose protocol automatically
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sockfd < 0) {
        iwarn("Error opening socket %s: %s", filename_color, perr());
        return false;
    }

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, filename, filename_len);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr) ) < 0) {
        iwarn_perr("Error connecting to socket %s", filename_color);
        return false;
    }

    int rc = write(sockfd, msg_s, msg_len + 1);
    
    if (rc < 0) {
        iwarn_perr("Error writing to socket %s", filename_color);
        return false;
    }

    /* Read 1 char at a time, and stop on newline or EOF.
     * Not efficient, but easy to program.
     * Note that a short read is not an error.
     */
    char c[1];
    int i;
    for (i = 0; i < buf_length; i++) {
        ssize_t numread = read(sockfd, c, 1);
        if (numread < 0) {
            iwarn_perr("Error reading from socket %s", filename_color);
            return false;
        }
        if (*c == '\n') 
            break;
        if (!trash_response) 
            *response++ = *c;
    }
    *response = '\0';

    if (close(sockfd)) {
        iwarn_perr("Error closing socket fd %d", sockfd);
        return false;
    }

    free(filename_color);
    free(msg_s);

    return true;
}

bool f_socket_unix_message(const char *filename, const char *msg) {
    //return f_socket_unix_message_f(filename, msg, NULL, SOCKET_LENGTH_DEFAULT + 1);
    return f_socket_unix_message_f(filename, msg, NULL, SOCKET_LENGTH_DEFAULT);
}

double f_time_hires() {
    struct timeb t;
    memset(&t, 0, sizeof(t));
    ftime(&t);

    time_t time = t.time;
    unsigned short millitm = t.millitm;

    double c = time + millitm / 1000.0;
    return c;
}

bool f_yes_no_flags(int deflt, int flags) {
    bool infinite = ! (flags & F_NOINFINITE); // def true

    bool ret = false;
    bool loop = true;
    while (loop) {
        char *yn = 
            deflt == F_DEFAULT_YES ?  "(Y/n)" : 
            deflt == F_DEFAULT_NO ? "(y/N)" :
            "(y/n)";
        printf("%s ", yn);

        char *l = NULL;
        size_t n = 0;

        int rc = getline(&l, &n, stdin);
        f_chop(l);

        if (rc == -1 || !strcmp(l, "\0")) { // no input
            if (deflt == F_DEFAULT_YES) {
                ret = true;
                loop = false;
            }
            else if (deflt == F_DEFAULT_NO) {
                ret = false;
                loop = false;
            }
        }
        else {
            if (
                !strcmp(l, "no\0") ||
                !strcmp(l, "n\0")
            ) {
                ret = false;
                loop = false;
            }
            else if (
                !strcmp(l, "yes\0") ||
                !strcmp(l, "y\0")
            ) {
                ret = true;
                loop = false;
            }
        }

        free(l);
        if (!infinite) loop = false;
    }
    return ret;
}

bool f_yes_no() {
    return f_yes_no_flags(0, 0);
}

/* Max string size helps catch non-null terminated strings.
 * But if it's too big, it could cause a segfault, I believe; if they're
 * lucky, the function will stay alive but return null.
 */

char *f_field(int width, char *string, int max_len) {
    // Max len gives some protection, not total.
    int len = strnlen(string, max_len);  // without \0

    if (len == max_len) {
        warn("field_with_len: max_len reached -- string not null terminated.");
        return NULL;
    }
    int num_spaces = width - len;
    if (num_spaces < 0) {
        warn("Field length (%d) bigger than desired width (%d)", len, width);
        num_spaces = 0;
    }
    char *new = str(len+num_spaces + 1); // comes with \0
    memset(new, ' ', len+num_spaces);
    memcpy(new, string, len);
    return new;
}

/* Not space-tolerant.
 * maxlen doesn't include \0, like strlen.
 */
bool f_is_int_strn(char *s, int maxlen) {
    char *t = s;
    int len = 1;
    if (*t != '+' && *t != '-' && *t != '0' && !isdigit(*t)) 
        return false;

    if (maxlen <= 0) {
        warn("f_is_int_strn: maxlen must be > 0 (got %d)", maxlen);
        return false;
    }

    while (++t, ++len, *t) {
        if (len > maxlen) break;
        if (!isdigit(*t)) return false;
    }
    return true;
}

/* Not space-tolerant.
 * f_is_int_strn is safer.
 */
bool f_is_int_str(char *s) {
    char *t = s;
    if (*t != '+' && *t != '-' && *t != '0' && !isdigit(*t)) 
        return false;
    while (++t, *t) {
        if (!isdigit(*t)) return false;
        //t++;
    }
    return true;
}

/* Caller has to assure \0-ok.
 */
void f_chop(char *s) {
    int i = strlen(s);
    if (i) s[i-1] = '\0';
    else piep;
}

/* Caller has to assure \0-ok.
 */
void f_chop_w(wchar_t *s) {
    int i = wcslen(s);
    if (i) s[i-1] = L'\0';
    else piep;
}

/* Caller should free.
 * len is the length without the \0 (in other words, the return value of
 * str(n)len).
 */
char *f_reverse_str(char *orig, size_t len) {
    if (len <= 0) {
        warn("f_reverse_str: len should be positive");
        return NULL;
    }
    char *ret = f_malloc((len + 1) * sizeof(char));
    ret[len] = '\0';
    char *p = orig + len;
    while (--p >= orig) {
        *ret++ = *p;

    }
    return ret - len;
}

/* Caller should free.
 */
char *f_comma(int n) {
    int n_sz = f_int_length(n);
    char *n_as_str = str(n_sz + 1);
    /* *2 is more than enough for commas and \0.
     */
    size_t ret_r_sz = n_sz * 2 * sizeof(char); 
    char *ret_r = str(ret_r_sz);
    if (snprintf(n_as_str, n_sz + 1, "%d", n) >= n_sz + 1)
        pieprnull;
    int i;
    int j = 0;
    int k = -1;
    char *str_r = f_reverse_str(n_as_str, n_sz);
    for (i = 0; i < n_sz; i++) {
        char c = str_r[i];
        ret_r[++k] = c;
        
        if (++j == 3 && i != n_sz - 1) {
            j = 0;
            k++;
            ret_r[k] = ',';
        }
    }
    int bytes_written = k + 1; // not counting \0
    free(n_as_str);
    char *ret = f_reverse_str(ret_r, bytes_written);
    free(ret_r);
    free(str_r);
    return ret;
}

int f_get_static_str_length() {
    return STATIC_STR_LENGTH;
}

bool f_set_utf8_f(int flags) {
    int no_warn = flags & F_NOWARN;
    int verbose = flags & F_VERBOSE;
    char *l = setlocale(LC_ALL, ""); // get from env
    if (! l) {  
        if (!no_warn) 
            warn("Couldn't get lang or charset from env variables.");
        return false;
    }
    else {
        char *lang, *charset;
        if (strlen(l) < 5) {
            if (!no_warn) 
                warn("Couldn't get lang or charset from env variables.");
            return false;
        }

        lang = str(6);
        strncpy(lang, l, 5);

        if (strlen(l) < 11) {
            if (!no_warn) 
                warn("No charset in locale (%s), trying to set", l);
        }
        else if (strncmp(l + 6, "UTF-8", 5)) {
            if (!no_warn) 
                warn("Charset in locale (%s) is not UTF-8, trying to set", l + 6);
        }
        else {
            // ok
            if (verbose) 
                info("Set locale to %s", l);
            return true;
        }

        char *locale = str(12);

        sprintf(locale, "%s.UTF-8", lang);
        free(lang);

        char *m = setlocale(LC_ALL, locale);
        bool ok;
        if (! m) {
            if (!no_warn) 
                warn("Couldn't set locale to %s", locale);
            ok = false;
        }
        else {
            ok = true;
            if (verbose)
                info("Set locale to %s", locale);
        }
        free(locale);
        return ok;
        
    }
    return true;
}

bool f_set_utf8() {
    return f_set_utf8_f(0);
}

// caller should free
wchar_t *d8(char *s) {
    int len = strlen(s);
    char *_line = str(len + 1);
    strcpy(_line, s);
    const char *line = _line; // doesn't need free
    wchar_t *line8 = f_malloc(sizeof(wchar_t) * (len+1)); // overshoot
    memset(line8, L'\0', sizeof(wchar_t) * (len+1)); // valgrind complains without this, maybe ok though without it
    mbstate_t ps = {0};
    size_t num = mbsrtowcs(line8, &line, len /* at most this many wides are written, excl L\0 */, &ps); // consumes line but s untouched
    if (num == 0) {
        warn("d8: 0 wide chars written to dest string");
    }
    else if (num == -1) {
        warn("d8: unable to decode string: %s", perr());
    }
    else {
        debug("d8: converted %d chars", num);
    }
    free(_line);
    return line8;
}

int f_get_max_color_length() {
    return COLOR_LENGTH;
}

int f_get_color_reset_length() {
    return COLOR_LENGTH_RESET;
}

/* Don't free.
 */
void f_signame(int signal, char **name, char **desc) {
    switch(signal) {
        case SIGHUP:
            signame_("HUP", "Hangup detected on controlling terminal or death of controlling process");
            break;
        case SIGINT:
            signame_("INT", "Interrupt from keyboard");
            break;
        case SIGQUIT:
            signame_("QUIT", "Quit from keyboard");
            break;
        case SIGILL:
            signame_("ILL", "Illegal Instruction");
            break;
        case SIGABRT:
            signame_("ABRT", "Abort signal from abort(3)");
            break;
        case SIGFPE:
            signame_("FPE", "Floating point exception");
            break;
        case SIGKILL:
            signame_("KILL", "Kill signal");
            break;
        case SIGSEGV:
            /* signame_("SEGV", "Invalid memory reference");
             * better known as:
             */
            signame_("SEGV", "Segmentation fault");
            break;
        case SIGPIPE:
            signame_("PIPE", "Broken pipe: write to pipe with no readers");
            break; 
        case SIGALRM:
            signame_("ALRM", "Timer signal from alarm(2)");
            break;
        case SIGTERM:
            signame_("TERM", "Termination signal");
            break;
        case SIGUSR1:
            signame_("USR1", "User-defined signal 1");
            break;
        case SIGUSR2:
            signame_("USR2", "User-defined signal 2");
            break; 
        case SIGCHLD:
            signame_("CHLD", "Child stopped or terminated");
            break;
        case SIGCONT:
            signame_("CONT", "Continue if stopped");
            break;
        case SIGSTOP:
            signame_("STOP", "Stop process");
            break;
        case SIGTSTP:
            signame_("TSTP", "Stop typed at terminal");
            break; 
        case SIGTTIN:
            signame_("TTIN", "Terminal input for background process");
            break;
        case SIGTTOU:
            signame_("TTOU", "Terminal output for background process");
            break;
        case SIGBUS:
            signame_("BUS", "Bus error (bad memory access)");
            break; 
            /* synonym for SIGIO
        case SIGPOLL:
            signame_("POLL", "Pollable event (Sys V).");
            break; 
            */
        case SIGPROF:
            signame_("PROF", "Profiling timer expired");
            break;
        case SIGSYS:
            signame_("SYS", "Bad argument to routine");
            break;
        case SIGTRAP:
            signame_("TRAP", "Trace/breakpoint trap");
            break;
        case SIGURG:
            signame_("URG", "Urgent condition on socket");
            break;
        case SIGVTALRM:
            signame_("VTALRM", "Virtual alarm clock");
            break;
        case SIGXCPU:
            signame_("XCPU", "CPU time limit exceeded");
            break;
        case SIGXFSZ:
            signame_("XFSZ", "File size limit exceeded");
            break;
            /* synonym for SIGABRT
        case SIGIOT:
            signame_("IOT", "IOT trap");
            break; 
            */
/* these ifdefs won't work if they're not macros but normal ints. 
 * works on linux in any case.
 */
#ifdef SIGEMT
        case SIGEMT:
            signame_("EMT", "");
            break; 
#endif
#ifdef SIGSTKFLT
        case SIGSTKFLT:
            signame_("STKFLT", "Stack fault on coprocessor (unused)");
            break;
#endif
        case SIGIO:
            signame_("IO", "I/O now possible");
            break;
            /* synonym for SIGCHLD 
        case SIGCLD:
            signame_("CLD", "Child stopped or terminated");
            break; 
            */
        case SIGPWR:
            signame_("PWR", "Power failure");
            break;
            /* synonym for SIGPWR
        case SIGINFO:
            signame_("INFO", "Power failure");
            break;
            */
#ifdef SIGLOST
        case SIGLOST:
            signame_("LOST", "File lock lost");
            break;
#endif
        case SIGWINCH:
            signame_("WINCH", "Window resize signal");
            break;
            /* synonym for SIGSYS
        case SIGUNUSED:
            signame_("UNUSED", "Bad argument to routine");
            break;
            */
        default:
            if (signal >= SIGRTMIN && signal <= SIGRTMAX) 
                signame_("RTMIN+n", "Real-time signal");
            else
                signame_(unknown_sig(signal), "Unknown signal");
    }
}

/* Private functions.
 */

/* not static, because needs to be called from header.
 * and not a macro to not pollute the caller's namespace.
 */
void _complain(char *file, unsigned int line, bool iserr, bool do_perr, char *format, ...) {

    int en = errno;

    char *the_str = str(COMPLAINT_LENGTH); 

    /* On overflow, snprintf is null-terminated, but rc is >= n.
     */
    va_list arglist;
    va_start(arglist, format);
    int rc;
    rc = vsnprintf(the_str, COMPLAINT_LENGTH, format, arglist);
    if (rc >= COMPLAINT_LENGTH) {
        // careful, recursive
        warn("(message string truncated).");
    }
    va_end(arglist);

    /* malloc
     */
    char *fl = NULL;
    char *p = NULL; // perr
    char *bull = get_bullet();
    char *bullet = iserr ? R_(bull) : BR_(bull);

    char *t1 = "";
    char *t2 = "";
    char *t3 = "";
    char *t4 = "";
    char *t5 = "";
    char *t6 = "";
    char *t7 = "";

    /* see comments in fish-util.h for all the cases.
     */
    if (file && line) {
        fl = f_get_warn_prefix(file, line);
        t1 = fl;
        t2 = " ";
        if (iserr) {
            if (do_perr) {
                errno = en;
                p = BR_((char*) perr()); // ditch const
                // ierr_perr(msg)
                if (*the_str) {
                    t3 = "Internal error: ";
                    t4 = the_str;
                    t5 = " (";
                    t6 = p;
                    t7 = ").";
                }
                // ierr_perr()
                else {
                    t3 = "Internal error (";
                    t4 = p;
                    t5 = ").";
                }
            }
            else {
                // ierr(msg)
                if (*the_str) {
                    t3 = "Internal error: ";
                    t4 = the_str;
                }
                // ierr() 
                else {
                    t3 = "Internal error.";
                }
            }
        }
        else {
            if (do_perr) {
                errno = en;
                p = BR_((char*) perr()); // ditch const
                // iwarn_perr(msg)
                if (*the_str) {
                    t3 = "Internal warning: ";
                    t4 = the_str;
                    t5 = " (";
                    t6 = p;
                    t7 = ").";
                }
                // iwarn_perr()
                else {
                    t3 = "Something's wrong (internally) (";
                    t2 = p;
                    t3 = ").";
                }
            }
            else {
                // iwarn(msg)
                if (*the_str) {
                    t3 = "Internal warning: ";
                    t4 = the_str;
                }
                // iwarn()
                else {
                    t3 = "Something's wrong (internally).";
                }
            }
        }
    }
    /* user/system warnings and errors
     */
    else {
        if (iserr) {
            if (do_perr) {
                errno = en;
                p = BR_((char*) perr()); // ditch const
                // err_perr(msg)
                if (*the_str) {
                    t1 = "Error: ";
                    t2 = the_str;
                    t3 = " (";
                    t4 = p;
                    t5 = ").";
                }
                // err_perr()
                else {
                    t1 = "Error (";
                    t2 = p;
                    t3 = ").";
                }
            }
            else {
                // err(msg)
                if (*the_str) {
                    t1 = "Error: ";
                    t2 = the_str;
                }
                // err()
                else {
                    t1 = "Error.";
                }
            }
        }
        else {
            if (do_perr) {
                errno = en;
                p = BR_((char*) perr()); // ditch const
                // warn_perr(msg)
                if (*the_str) {
                    t1 = the_str;
                    t2 = " (";
                    t3 = p;
                    t4 = ").";
                }
                // warn_perr()
                else {
                    t1 = "Something's wrong (";
                    t2 = p;
                    t3 = ").";
                }
            }
            else {
                // warn(msg)
                if (*the_str) {
                    t1 = the_str;
                }
                // warn()
                else {
                    t1 = "Something's wrong.";
                }
            }
        }
    }

    fprintf(stderr, "%s %s%s%s%s%s%s%s\n", bullet, t1, t2, t3, t4, t5, t6, t7);

    if (fl) free(fl);
    if (p) free(p);
    free(the_str);
    free(bullet);
}

static char *_color(const char *s, int idx) {
    char *t = str(strlen(s) + 1 + 4 + 5);
    bool disable = _disable_colors | ! isatty(fileno(stdout));
    char *a = disable ? "" : COL[idx];
    char *b = disable ? "" : COL[0];
    sprintf(t, "%s%s%s", a, s, b );
    return t;
}

static void _static_str_init() {
    _static_strs[0] = &_s;
    _static_strs[1] = &_t;
    _static_strs[2] = &_u;
    _static_strs[3] = &_v;
    _static_strs[4] = &_w;
    _static_strs[5] = &_x;
    _static_strs[6] = &_y;
    _static_strs[7] = &_z;

    for (int i = 0; i < NUM_STATIC_STRINGS; i++) {
        *_static_strs[i] = f_malloc(sizeof(char) * STATIC_STR_LENGTH);
    }

}

static void _color_static(const char *c) {
    int l = strlen(c);
    if (l > STATIC_STR_LENGTH - 1) {
        warn("static string truncated (%s)", c);
        l = STATIC_STR_LENGTH - 1;
    }
    char **ptr = _get_static_str_ptr();
    // include null
    memcpy(*ptr, c, l + 1);
}

static char **_get_static_str_ptr() {
    _static_str_idx = (_static_str_idx+1) % NUM_STATIC_STRINGS;
    return _static_strs[_static_str_idx];
}

static void _sys_say(const char *cmd) {
    char *bullet = get_bullet();
    char *new = str(strlen(cmd) + strlen(bullet) + COLOR_LENGTH + COLOR_LENGTH_RESET + 2 + 1);
    char *c = G_(bullet);
    sprintf(new, "%s %s", c, cmd);
    say (new);
    free(new);
    free(c);
}

static void _static_strings_free() {
    if (!_static_str_initted) return;
    for (int i = 0; i < NUM_STATIC_STRINGS; i++) {
        char **c = _static_strs[i];
        char *cc = *c;
        free(cc);
    }
    _static_str_initted = false;
}
