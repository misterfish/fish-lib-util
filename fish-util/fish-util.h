#ifdef FISH_UTIL_H
#else
#define FISH_UTIL_H

#define _GNU_SOURCE

#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

#ifndef DEBUG_LENGTH
 #define DEBUG_LENGTH 200
#endif

/* Don't use _() in the macros: can have unexpected consequences for caller.
 */

/* ##__VA_ARGS__ to swallow the preceding comma if omitted.
 */

#ifdef DEBUG 
# define debug(x, ...) do { \
    char *pref = f_get_warn_prefix(__FILE__, __LINE__); \
    char *dbg = str(DEBUG_LENGTH); \
    snprintf (dbg, DEBUG_LENGTH, x, ##__VA_ARGS__); \
    info("%s:debug:%s", pref, dbg); \
    free(dbg); \
} while(0);
#else
# define debug(...) {}
#endif

/* This should disapper (see fish-lib-asound)  XX
 */
#define _FISH_WARN_LENGTH 500

/* 
 * Some nice-to-look-at output and less tedious coding. 
 *
 * It is (probably) GCC-specific -- to keep it portable we could have used
 * __VA_ARGS__ instead of args..., but there is still no good way to do the
 * ##__VA_ARGS__ trick to eat the comma without GCC.
 *
 * #args makes it into a string, commas and all. 
 * Becomes empty string if arg not given (handy).
 * Becomes non-sense string if arg is a list (not handy).
 * Gets extra set of "" if arg is a string (annoying).
 *
 * msg... will only work if it's a single argument, not a list.
 * This is because C99 is strict about leaving the comma if the only
 * argument is variadic (for some reason).
 *
 * Use the _fmt versions for sprintf-like list. (In that case the arg list can't be
 * empty).
 *
 * Pink ('bright red') bullet for warnings, red bullet for errors.
 * iwarn, ierr: intended for programmer (not user) errors.
 *
 * Caller must not call fish_util_cleanup() before calling any of the err
 * functions -- we do it.
 * Also all err functions exit with status of 1.
 *
 * iwarn()/piep:    <file>:<line> Something's wrong (internally).
 * iwarn(msg):      <file>:<line> Internal warning: <msg>
 * ierr()/die:      <file>:<line> Internal error.
 * ierr(msg):       <file>:<line> Internal error: <msg>
 *
 * iwarn_perr():    <file>:<line> Something's wrong (internally) (<system error>).
 * iwarn_perr(msg): <file>:<line> Internal warning: <msg> (<system error>).
 * ierr_perr():     <file>:<line> Internal error (<system error>).
 * ierr_perr(msg):  <file>:<line> Internal error: <msg> (<system error>).
 *
 * err, warn: intended for user errors and system errors.
 *
 * warn():          Something's wrong.
 * warn(msg):       <msg>
 * err():           Error.
 * err(msg):        Error: <msg>
 *
 * warn_perr():     Something's wrong (<system error>).
 * warn_perr(msg):  <msg> (<system error>).
 * err_perr():      Error (<system error>).
 * err_perr(msg):   Error: <msg> (<system error>).
 *
 * piep, pieprf, piepr0, etc.: (pronounced 'peep'). 
 * Intended as easy-to-type way to warn (and possibly return false, return
 * 0, etc.). Pipes through iwarn().
 *
 * die: synonym for ierr()
 *
 * The error functions will often leak their format argument variables. It's
 * too difficult (and overkill) to try to programmatically free them before
 * exit (they could be static).
 *
 */

#define iwarn_fmt(format, args...) do { \
    iwarn(format, ##args); \
} while(0);

#define iwarn(msg...) do { \
    _complain(__FILE__, __LINE__, false, false, #msg); \
} while (0); 

#define ierr_fmt(format, args...) do { \
    ierr(format, ##args); \
} while(0);

#define ierr(msg...) do { \
    _complain(__FILE__, __LINE__, true, false, #msg); \
    _err(); \
} while (0); 

#define die do { \
    ierr(); \
} while (0); 

#define iwarn_perr_fmt(format, args...) do { \
    iwarn_perr(format, ##args); \
} while(0);

#define iwarn_perr(msg...) do { \
    _complain(__FILE__, __LINE__, false, true, #msg); \
} while (0); 

#define ierr_perr_fmt(format, args...) do { \
    ierr_perr(format, ##args); \
} while(0);

#define ierr_perr(msg...) do { \
    _complain(__FILE__, __LINE__, true, true, #msg); \
    _err(); \
} while (0); 

#define warn_fmt(format, args...) do { \
    warn(format, ##args); \
} while(0);

#define warn(msg...) do { \
    _complain("", 0, false, false, #msg); \
} while (0); 

#define err_fmt(format, args...) do { \
    err(format, ##args); \
} while(0);

#define err(msg...) do { \
    _complain("", 0, true, false, #msg); \
    _err(); \
} while (0); 

#define warn_perr_fmt(format, args...) do { \
    warn_perr(format, ##args); \
} while(0);

#define warn_perr(msg...) do { \
    _complain("", 0, false, true, #msg); \
} while (0); 

#define err_perr_fmt(format, args...) do { \
    err_perr(format, ##args); \
} while(0);

#define err_perr(msg...) do { \
    _complain("", 0, true, true, #msg); \
    _err(); \
} while (0); 

/*
    char *pref = f_get_warn_prefix(__FILE__, __LINE__); \
    char *warning = str(_FISH_WARN_LENGTH); \
    snprintf(warning, _FISH_WARN_LENGTH, x, ##__VA_ARGS__); \
    int len2 = strnlen(warning, _FISH_WARN_LENGTH);  \
    char *new = str(len + len2 + 1 + 1);   \
    */

//char *internal = _empty(_error) ? "Internal error." : "Internal error: "; 
/* This might leak the x string, but that's hard to avoid.
 */

/*
#define ierr_msg(x, ...) do {\
    char *pref = f_get_warn_prefix(__FILE__, __LINE__); \
    int len = strlen(pref); \
    char *_error = str(_FISH_WARN_LENGTH); \
    if (!_empty(x)) \
        snprintf(_error, _FISH_WARN_LENGTH, x, ##__VA_ARGS__); \
    int len2 = strnlen(_error, _FISH_WARN_LENGTH);  \
    char *new = str(len + len2 + 1 + 1);   \
    sprintf(new, "%s %s", pref, _error);  \
    free(pref); \
    free(_error); \
    err(new); \
} while (0);
*/
/* Note that do {} makes it impossible for this to be used in ternary
 * conditional.
 */


/* e.g. <file>:<line>, with colors.

#define warnpref f_get_warn_prefix(__FILE__, __LINE__)

 * Only checks if first char is null.
 * No strlen.
 */

#define _isemptystr(x) (!strncmp(x, "", 1))

#define piep do { \
    iwarn(); \
} while (0); 

#define pieprf do { \
    piep; \
    return false; \
} while (0) ;

#define pieprt do { \
    piep; \
    return true; \
} while (0) ;

#define piepr1 do { \
    piep; \
    return 1; \
} while (0) ;

#define piepr do { \
    piep; \
    return; \
} while (0) ;

#define piepbr do { \
    piep; \
    break; \
} while (0) ;

#define piepc do { \
    piep; \
    continue; \
} while (0) ;

#define pieprnull do { \
    piep; \
    return NULL; \
} while (0) ;

#define pieprneg1 do { \
    piep; \
    return -1; \
} while (0) ;

#define piepr0 do { \
    piep; \
    return 0; \
} while (0) ;

/* int (32 bits).
 * Not all combinations make sense and some are redundant. 
 * Depends on function.
 */

#define F_DIE              0x01
#define F_NODIE            0x02
#define F_QUIET            0x04
#define F_NOQUIET          0x08
#define F_KILLERR          0x10
#define F_NOKILLERR        0x20
#define F_UTF8             0x40
#define F_NOUTF8           0x80
#define F_WARN            0x100
#define F_NOWARN          0x200
#define F_VERBOSE         0x400
#define F_NOVERBOSE       0x800
#define F_DEFAULT_NONE   0x1000
#define F_DEFAULT_NO     0x2000
#define F_DEFAULT_YES    0x4000
#define F_INFINITE       0x8000
#define F_NOINFINITE    0x10000
#define F_WRITE         0x20000
#define F_READ          0x40000
#define F_APPEND        0x80000
#define F_READ_WRITE_NO_TRUNC   0x100000
#define F_READ_WRITE_TRUNC      0x200000

/* Static strings. 
 * These names should not be used for any other variables.
 */
char *_s, *_t, *_u, *_v, *_w, *_x, *_y, *_z;

void fish_util_cleanup();

/* Only necessary to restart after a cleanup.
 */
void fish_util_init();

/* Functions without f_ prefix.
 */

void _();
void spr(const char *format, ...);
char *spr_(const char *format, int size, ...);

void say(const char *format, ...);
void ask(const char *format, ...);
void info(const char *format, ...);
FILE *sysr(const char *cmd);
FILE *sysw(const char *cmd);
/* Run command and read input until EOF.
 */
int sys(const char *cmd);
int sysclose(FILE *f);
int sysclose_f(FILE *f, const char *cmd, int flags);

FILE *safeopen(char *filespec);
FILE *safeopen_f(char *filespec, int flags);

const char *perr();

wchar_t *d8(char *s);

char *str(int length);

char *R_(const char *s);
char *BR_(const char *s);
char *G_(const char *s);
char *BG_(const char *s);
char *Y_(const char *s);
char *BY_(const char *s);
char *B_(const char *s);
char *BB_(const char *s);
char *CY_(const char *s);
char *BCY_(const char *s);
char *M_(const char *s);
char *BM_(const char *s);

void R(const char *s);
void BR(const char *s);
void G(const char *s);
void BG(const char *s);
void Y(const char *s);
void BY(const char *s);
void B(const char *s);
void BB(const char *s);
void CY(const char *s);
void BCY(const char *s);
void M(const char *s);
void BM(const char *s);

/* Called by error macros in .h. 
 * Means header is not enough to use error macros (.o needs to be linked as well).
 */
void _err();

/* Functions with f_
 */
void f_disable_colors();
void f_sys_die(bool b);
void f_verbose_cmds(bool b);

bool f_yes_no();
bool f_yes_no_flags(int, int);
void f_autoflush();
bool f_sig(int signum, void *func);
void f_benchmark();
int f_int_length(int i);

bool f_socket_make_named(const char *filename, int *socket);
bool f_socket_make_client(int socket, int *client_socket);

/* num_read can be NULL.
 */
bool f_socket_read(int client_socket, ssize_t *num_read, char *buf, size_t max_length);

/* num_written can be NULL.
 */
bool f_socket_write(int client_socket, ssize_t *num_written, const char *buf, size_t len);

bool f_socket_close(int client_socket);

bool f_socket_unix_message(const char *filename, const char *msg);
bool f_socket_unix_message_f(const char *filename, const char *msg, char *response, int buf_length);

double f_time_hires();

char *f_field(int width, char *string, int max_len);

bool f_is_int_str(char *s);
bool f_is_int_strn(char *s, int maxlen);

void f_chop(char *s);
void f_chop_w(wchar_t *s);

//char *f_comma(int n);

int f_get_static_str_length();

bool f_set_utf8();
bool f_set_utf8_f(int flags);

struct stat *f_stat_f(const char *file, int flags);
struct stat *f_stat(const char *file);

bool f_test_f(const char *file);
bool f_test_d(const char *file);

char *f_get_warn_prefix(char *file, int line);

int f_get_max_color_length();
int f_get_color_reset_length();

void _complain(char *file, unsigned int line, bool iserr, bool perr, char *format, ...);

/* guard */
#endif
/* guard */



