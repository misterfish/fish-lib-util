#define _GNU_SOURCE // here, not header, not exported

// Be careful, truncated cmds bad. XX
#define CMD_LENGTH 1000

#define ERR_LENGTH 200 // should check biggest string in non-english xx
#define INFO_LENGTH 300
#define WARN_LENGTH 500

#define SOCKET_LENGTH_DEFAULT 100

#define STATIC_STR_LENGTH 200

// length of (longest) color escape
#define COLOR_LENGTH 5
#define COLOR_LENGTH_RESET 4

#define STRNLEN_REVERSE 20

#define NUM_STATIC_STRINGS 8

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

static const char *BULLET = "ঈ";
static const int BULLET_LENGTH = 3; // in utf-8 at least XX

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

/* Public functions.
 */

/* Caller shouldn't free.
 */
FILE *safeopen_f(char *filespec, int flags) {
    FILE *f = NULL;

    char *filename = NULL;
    bool err = false;
    bool die = ! (flags & F_NODIE);
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

    if (test_d(filename)) {
        _();
        Y(filename);
        warnp("File %s is a directory (can't open)", _s);
        err = true;
    }
    else {
        f = fopen(filename, mode_f); // malloc
        if (!f) {
            err = true;
            if (!quiet) {
                _();
                Y(filename);
                CY(mode);
                warnp("Couldn't open %s for %s: %s", _s, _t, perr());
            }
        }
    }

    free(mode_f);

    if (err) {
        if (die) {
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
        saves = malloc(sizeof(char) * STATIC_STR_LENGTH);
        savet = malloc(sizeof(char) * STATIC_STR_LENGTH);
        saveu = malloc(sizeof(char) * STATIC_STR_LENGTH);
        savev = malloc(sizeof(char) * STATIC_STR_LENGTH);
        savew = malloc(sizeof(char) * STATIC_STR_LENGTH);
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

        //free(saves);
        //free(savet);
        //free(saveu);
        //free(savev);
        //free(savew);
    }

}

void _static_strings_restore() { 
    _static_strings_save_restore(1);
}

void _static_strings_save() {
    _static_strings_save_restore(0);
}

char *f_get_warn_prefix(char *file, int line) {
    _static_strings_save();

    if (!warn_prefix_size) {
        warn_prefix_size = 10;
        warn_prefix = str(warn_prefix_size);
    }

    _();
    spr("%d", line);
    Y(_s);

    int len = strlen(file) + 1 + strlen(_t) + 1 + 1;
    if (len > warn_prefix_size) {
        warn_prefix_size = len;
        warn_prefix = (char*) realloc((void*) warn_prefix, len * sizeof(char));
        if (!warn_prefix) 
            die_perr();
    }

    sprintf(warn_prefix, "%s:%s", file, _t);

    _static_strings_restore();

    return warn_prefix;
}


bool test_f(const char *file) {
    struct stat *s = f_stat_f(file, F_QUIET);
    if (!s) return 0;
    int mode = s->st_mode;
    if (!mode) return 0;

    return S_ISREG(mode);
}

bool test_d(const char *file) {
    struct stat *s = f_stat_f(file, F_QUIET);
    if (!s) return 0;
    int mode = s->st_mode;
    if (!mode) return 0;

    return S_ISDIR(mode);
}

/* Take from the heap, probably slower than normal stat.
 */
struct stat *f_stat_f(const char *file, int flags) {
    if (!mystat_initted) {
        mystat = malloc(sizeof(*mystat));
        mystat_initted = true;
    }
    memset(mystat, 0, sizeof(*mystat));

    bool quiet = flags & F_QUIET;

    if (-1 == stat(file, mystat)) {
        if (!quiet) {
            _();
            Y(file);
            warn("Couldn't stat %s: %s", _s, perr());
        }
        return NULL;
    }
    return mystat;
}

struct stat *f_stat(const char *file) {
    return f_stat_f(file, 0);
}

void disable_colors() {
    _disable_colors = 1;
}

/* string with all nulls (can't use strlen).
 * length includes null.
 * caller should free.
 */
char *str(int length) {
    assert(length > 0);
    char *s = malloc(length * sizeof(char));
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
        _static_str_initted = true;
        _static_str_init();
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
        warn("fixme: perr stored string in buf, possible leak.");
        free(st);
        return ret;
    }
}

char *add_nl(const char *orig) {
    // not including null
    int len = strlen(orig);
    char *new = malloc(sizeof(char) * (len + 2));
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
    vsnprintf( new, INFO_LENGTH, format, arglist );
    vsnprintf( new2, INFO_LENGTH + 1, format, arglist_copy );
    if (strncmp(new, new2, INFO_LENGTH)) { // no + 1 necessary
        warn("Ask string truncated.");
    }
    va_end( arglist );

    char *question = str(strlen(new) + COLOR_LENGTH + COLOR_LENGTH_RESET + BULLET_LENGTH + 1 + 1 + 1 + 1);
    char *c = M_(BULLET);
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
    vsnprintf( new, INFO_LENGTH, format, arglist );
    vsnprintf( new2, INFO_LENGTH + 1, format, arglist_copy );
    if (strncmp(new, new2, INFO_LENGTH)) { // no + 1 necessary
        warn("Info string truncated.");
    }
    va_end( arglist );

    char *infos = str(strlen(new) + COLOR_LENGTH + COLOR_LENGTH_RESET + BULLET_LENGTH + 1 + 1 + 1);
    char *c = BB_(BULLET);
    sprintf(infos, "%s %s\n", c, new);
    printf(infos);
    free(c);
    free(infos);

    free(new);
    free(new2);
}

void err(const char *format, ...) {
    char *new = str(ERR_LENGTH);
    char *new2 = str(ERR_LENGTH + 1); // for checking truncations
    va_list arglist, arglist_copy;
    va_start( arglist, format );
    va_copy(arglist_copy, arglist);
    vsprintf(new, format, arglist );
    vsnprintf( new2, ERR_LENGTH + 1, format, arglist_copy );
    if (strncmp(new, new2, ERR_LENGTH)) { // no + 1 necessary
        warn("Error string truncated.");
    }
    va_end( arglist );
    char *errors = str(strlen(new) + COLOR_LENGTH + COLOR_LENGTH_RESET + BULLET_LENGTH + 1 + 1 + 1);
    char *c = R_(BULLET);
    sprintf(errors, "%s %s\n", c, new);
    fprintf(stderr, errors);
    free(errors);
    free(c);
    free(new);
    free(new2);
    exit(1);
}

// used in die() macros.
void __die(const char *file, int line) {
    // leaks, don't care.
    // check for trunc ??
    err(spr_("Unexpected error (%s:%s)", ERR_LENGTH, file, Y_(spr_("%d", 100, line))));
}

// used in die() macros.
void __die_msg(const char *file, int line, const char *msg) {
    // leaks, don't care.
    // check for trunc ??
    err(spr_("Unexpected error (%s:%s) -- %s", ERR_LENGTH, file, Y_(spr_("%d", 100, line)), msg));
}


void warn(const char *format, ...) {
    char *new = str(WARN_LENGTH);
    char *new2 = str(WARN_LENGTH + 1);
    va_list arglist, arglist_copy;
    va_start( arglist, format );
    va_copy(arglist_copy, arglist);
    vsnprintf(new, WARN_LENGTH, format, arglist );
    vsnprintf( new2, WARN_LENGTH + 1, format, arglist_copy );
    if (strncmp(new, new2, WARN_LENGTH)) { // no + 1 necessary
        //warn("Warn string truncated.");
        printf("warn: warn string truncated\n");
    }
    va_end( arglist );

    char *warning = str(strlen(new) + COLOR_LENGTH + COLOR_LENGTH_RESET + BULLET_LENGTH + 1 + 1 + 1);
    char *c = BR_(BULLET);
    sprintf(warning, "%s %s\n", c, new);
    fprintf(stderr, warning);
    free(warning);
    free(c);
    free(new);
    free(new2);
} 

void sys_die(bool b) {
    _die = b;
}

void verbose_cmds(bool b) {
    _verbose = b;
}

/* Non-null FILE means fork or pipe succeeded, but command could still fail
 * (e.g. command not found).
 * Caller should check for NULL, and call sysclose.
 */
FILE *sysr(const char *cmd) {
    if (_verbose) _sys_say(cmd);
    FILE *f = popen(cmd, "r");
    if (f == NULL) {
        _();
        BR(cmd);
        spr("sysr: can't launch cmd (%s) for reading, fork or pipe failed (%s).", _s, perr(errno));
        _die ? err(_t) : warn(_t);
    }
    return f;
}

/* Non-null FILE means fork or pipe succeeded, but command could still fail
 * (e.g. command not found).
 * Caller should call sysclose.
 */
FILE *sysw(const char *cmd) {
    if (_verbose) _sys_say(cmd);
    FILE *f = popen(cmd, "w");
    if (f == NULL) {
        _();
        BR(cmd);
        spr("sysr: can't launch cmd (%s) for reading, fork or pipe failed (%s).", _s, perr(errno));
        _die ? err(_t) : warn(_t);
    }
    return f;
}

// 0 means good.
int sysclose_f(FILE *f, const char *cmd) {
    int i = pclose(f);
    if (i != 0) {
        // Perr doesn't work here -- if the shell fails it should print
        // something to stderr, and set non-zero exit, but it won't (can't)
        // set errno.
        _();
        if (cmd != NULL) {
            BR(cmd);
            spr(" %s", _s);
        }
        else {
            spr("");
            spr("");
        }
        spr("Error with cmd%s.", _t);
        _die ? err(_u) : warn(_u);
    }
    return i;
}

int sysclose(FILE *f) {
    return sysclose_f(f, NULL);
}

// 0 means good
int sysxf(const char *orig, ...) {
    char *cmd = str(CMD_LENGTH);
    va_list arglist;
    va_start( arglist, orig );
    vsnprintf( cmd, CMD_LENGTH, orig, arglist );
    va_end( arglist );

    int c = sysx(cmd);
    free(cmd);
    return c;
}

// 0 means good
int sysx(const char *cmd) {
    FILE *f = sysr(cmd);

    int le = CMD_LENGTH;
    char *buf = str(le);
    while (fgets(buf, le, f) != NULL) {
    }
    free(buf);

    if (f != NULL) {
        int ret = sysclose_f(f, cmd);
        return ret;
    }
    else return 1;
}

/* Not implemented. XX
 */
int sys(char *ret, const char *cmd) {
    /*
    char *buf = str(CMD_LENGTH);
    while (fgets(buf, CMD_LENGTH, f) != NULL) {
        printf(buf);
    }
    */
    return -1;
}

bool f_sig(int signum, void *func) {
    // Ok that it's thrown away apparently.
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = func;
    int rc = sigaction(signum, &action, NULL);
    if (rc) {
        _();
        spr("%s", perr());
        spr("%d", signum);
        R(_t);
        warn("Couldn't attach signal %s: %s", _u, _s);
        return false;
    }
    return true;
}

void autoflush() {
    setvbuf(stdout, NULL, _IONBF, 0);
}

void f_benchmark(int flag) {

    static int sec;
    static int usec;

    struct timeval *tv = malloc(sizeof *tv); 

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
        //spr("%10.3f", 15, delta);
        spr("%10.3f", delta);
        G(_s);
        warn("Benchmark: %s s", _t);

        sec = -1;
        usec = -1;
    }
}

// errno doesn't always work.
long int stoie(const char *s, int *err) {
    char *endptr;
    long int i = strtol(s, &endptr, 10);
    // nothing converted
    if (endptr == s) {
        *err = 1;
        return LONG_MIN;
    }
    else {
        return i;
    }
}

long int stoi(const char *s) {
    int err = 0;
    int i = stoie(s, &err);
    if (err) {
        // XX
        warn("Can't convert string to int: %s", R_(s));
        return LONG_MIN;
    }
    return i;
}

int int_length(int i) {
    if (i == 0) return 1;
    if (i < 0) return int_length(-1 * i);

    return 1 + (int) log10(i);
}

bool f_socket_make_named (const char *filename, int *socknum) {
    struct sockaddr_un name;
    size_t size;
  
    // 0 for protocol
    int sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock < 0) {
        _();
        Y(filename);
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
        _();
        Y(filename);
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
        _();
        spr("%d", socket);
        Y(_s);
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
        _();
        spr("%d", client_socket);
        Y(_s);
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
        _();
        spr("%d", client_socket);
        Y(_s);
        warn("Couldn't write to socket %s (%s)", _t, perr());
        return false;
    }

    if (num_written != NULL) 
        *num_written = rc;

    /* Not necessary it seems.

    if (!fsync(client_socket)) {
        _();
        spr("%d", client_socket);
        Y(_s);
        warn("Couldn't flush socket %s after write (%s)", _t, perr());
        return false;
    }
    */

    return true;
}

bool f_socket_close(int client_socket) {
    if (close(client_socket)) {
        _();
        spr("%d", client_socket);
        Y(_s);
        warn("Couldn't close client socket %s (%s)", _t, perr());
        return false;
    }
    return true;
}

/* strlen filename, strlen msg.
 * buf_length applies to both msg and response.
 */
//bool socket_unix_message_f(const char *filename, const char *msg, char **response, int buf_length, int flags) {
bool socket_unix_message_f(const char *filename, const char *msg, char *response, int buf_length) {
    struct sockaddr_un serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    bool trash_response = response == NULL;

    int msg_len = strlen(msg);
    int filename_len = strlen(filename);

    char *filename_color = CY_(filename);

    char *msg_s = str(msg_len + 1 + 1);
    sprintf(msg_s, "%s\n", msg);

    if (strlen(msg) >= buf_length) {
        _();
        spr("%d", buf_length - 1);
        CY(_s);

        warnp("msg too long (max is %s)", _t);
        return false;
    }

    // 0 means choose protocol automatically
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sockfd < 0) {
        warnp("error opening socket %s: %s", filename_color, perr());
        return false;
    }

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, filename, filename_len);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr) ) < 0) {
        warnp("error connecting to socket %s: %s", filename_color, perr());
        return false;
    }

    int rc = write(sockfd, msg_s, msg_len + 1);
    
    if (rc < 0) {
        warnp("error writing to socket %s: %s", filename_color, perr());
        return false;
    }

    char c[2] = "a";
    int i;
    for (i = 0; i < buf_length; i++) {
        int rc = read(sockfd, c, 1);
        if (rc < 0) {
            warnp("error reading from socket %s: %s", filename_color, perr());
            return false;
        }
        if (*c == '\n') {
            break;
        }
        if (!trash_response) strncat(response, c, 1);
    }
    close(sockfd);

    free(filename_color);

    return true;
}

bool socket_unix_message(const char *filename, const char *msg) {
    return socket_unix_message_f(filename, msg, NULL, SOCKET_LENGTH_DEFAULT + 1);
}

double time_hires() {
    struct timeb t;
    memset(&t, 0, sizeof(t));
    ftime(&t);

    time_t time = t.time;
    unsigned short millitm = t.millitm;

    double c = time + millitm / 1000.0;
    return c;
}

bool yes_no_flags(int deflt, int flags) {
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
        chop(l);

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

bool yes_no() {
    return yes_no_flags(0, 0);
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
bool is_int_strn(char *s, int maxlen) {
    char *t = s;
    int len = 1;
    if (*t != '+' && *t != '-' && *t != '0' && !isdigit(*t)) 
        return false;

    if (maxlen <= 0) {
        warn("is_int_strn: maxlen must be > 0 (got %d)", maxlen);
        return false;
    }

    while (++t, ++len, *t) {
        //printf("len is %d, char is %c\n", len, *t);
        if (len > maxlen) break;
        if (!isdigit(*t)) return false;
        //t++;
    }
    return true;
    /*
    errno = 0;
    int a = strtol(s, NULL, 10); // man atoi
    return errno ? false : true;
    */
}

/* Not space-tolerant.
 * is_int_strn is safer.
 */
bool is_int_str(char *s) {
    char *t = s;
    if (*t != '+' && *t != '-' && *t != '0' && !isdigit(*t)) 
        return false;
    while (++t, *t) {
        if (!isdigit(*t)) return false;
        //t++;
    }
    return true;
    /*
    errno = 0;
    int a = strtol(s, NULL, 10); // man atoi
    return errno ? false : true;
    */
}

void fish_util_cleanup() {
    if (!_static_strings_freed) {
        _static_strings_free();
        _static_strings_freed = true;
    }

    if (warn_prefix_size) {
        free(warn_prefix);
        warn_prefix_size = 0;
    }
    if (mystat_initted) {
        free(mystat);
        mystat_initted = false;
    }
}

/* Caller has to assure \0-ok.
 */
void chop(char *s) {
    int i = strlen(s);
    if (i) s[i-1] = '\0';
    else piep;
}

/* Caller has to assure \0-ok.
 */
void chop_w(wchar_t *s) {
    int i = wcslen(s);
    if (i) s[i-1] = L'\0';
    else piep;
}

char *comma(int n) {
    int size = int_length(n);
    char *as_str = str(size + 1);
    int size2 = size * 2 * sizeof(char); // *2 is more than enough for commas
    char *ret_r = str(size2 + 1);
    memset(ret_r, '\0', size2);
    int actual_size = snprintf(as_str, size + 1, "%d", n);
    int i;
    int j = 0;
    int k = -1;
    char *str_r = reverse(as_str);
    for (i = 0; i < actual_size; i++) {
        char c = *(str_r + i);
        k++;
        *(ret_r + k) = c;
        
        if (++j == 3 && i != actual_size - 1) {
            j = 0;
            k++;
            *(ret_r + k) = ',';
        }
    }
    free(as_str);
    free(str_r);
    char *ret = reverse(ret_r);
    free(ret_r);
    return ret;
}

char *reverse(char *s) {
    int len = strnlen(s, STRNLEN_REVERSE);  // without \0. can still segfault.
    if (len == STRNLEN_REVERSE) {
        warn("reverse(): input too big");
        return NULL;
    }
    char *r = str(len+1);
    int i;
    for (i = 0; i < len; i++) {
        *(r+i) = *(s+len-i-1);
    }
    *(r+i) = '\0';
    return r;
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
    wchar_t *line8 = malloc(sizeof(wchar_t) * (len+1)); // overshoot
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

/* Private functions.
 */

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
        *_static_strs[i] = malloc(sizeof(char) * STATIC_STR_LENGTH);
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
    char *new = str(strlen(cmd) + BULLET_LENGTH + COLOR_LENGTH + COLOR_LENGTH_RESET + 2 + 1);
    char *c = G_(BULLET);
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
}
