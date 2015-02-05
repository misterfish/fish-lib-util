#define _GNU_SOURCE

#include <string.h>
#include <wchar.h>
#include <stdlib.h>

#ifdef __FISH_UTIL_H
#else
 #define __FISH_UTIL_H

#ifndef DEBUG_LENGTH
 #define DEBUG_LENGTH 200
#endif

/* ##__VA_ARGS__ to swallow the comma if omitted.
 * snprintf: does put \0
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

#define _FISH_WARN_LENGTH 500

/* iwarn, ierr: intended for definitely internal (not user) errors.
 * Probably best if it's restricted to programmer errors.
 * Print file and line num, that's why macro.
 */
#define iwarn_msg(x, ...) do {\
    char *pref = f_get_warn_prefix(__FILE__, __LINE__); \
    int len = strlen(pref); \
    char *warning = str(_FISH_WARN_LENGTH); \
    snprintf(warning, _FISH_WARN_LENGTH, x, ##__VA_ARGS__); \
    int len2 = strnlen(warning, _FISH_WARN_LENGTH);  \
    char *internal = *warning == '\0' ? "Something's wrong." : "Something's wrong: "; \
    int len3 = strlen(internal); \
    char *new = str(len + len2 + len3 + 1 + 1);   \
    sprintf(new, "%s %s%s", pref, internal, warning);  \
    warn(new); \
    free(warning); \
    free(new); \
} while (0);

#define iwarn do {\
    iwarn_msg(""); \
} while (0); 

#define ierr do { \
    ierr_msg(""); \
} while (0);

#define ierr_msg(x, ...) do {\
    char *pref = f_get_warn_prefix(__FILE__, __LINE__); \
    int len = strlen(pref); \
    char *_error = str(_FISH_WARN_LENGTH); \
    snprintf(_error, _FISH_WARN_LENGTH, x, ##__VA_ARGS__); \
    int len2 = strnlen(_error, _FISH_WARN_LENGTH);  \
    char *internal = *_error == '\0' ? "Internal error." : "Internal error: "; \
    int len3 = strlen(internal); \
    char *new = str(len + len2 + len3 + 1 + 1);   \
    sprintf(new, "%s %s%s", pref, internal, _error);  \
    err(new); \
} while (0);

#define warnpref f_get_warn_prefix(__FILE__, __LINE__)

/* 
 * err, warn: For when it's not definitely internal. Could be user error or
 * something like file not found, for example.
 * Those are functions, not macros (cuz don't need file + line).
 */

#define ierr_perr() do { \
    ierr_perr_msg(""); \
} while(0);

#define ierr_perr_msg(x, ...) do { \
    char *warning = str(_FISH_WARN_LENGTH); \
    snprintf(warning, _FISH_WARN_LENGTH, x, ##__VA_ARGS__); \
    _warn_perr_msg("Error: ", warning); \
    exit(1); \
} while(0);

#define _warn_perr_msg(pref, x, ...) do { \
    char *msg = str(_FISH_WARN_LENGTH); \
    char *left_paren; \
    char *right_paren; \
    if (strlen(x) > 0) { \
        snprintf(msg, _FISH_WARN_LENGTH, x, ##__VA_ARGS__); \
        left_paren = " ("; \
        right_paren = ")"; \
    } \
    else { \
        msg = left_paren = right_paren = ""; \
    } \
    warn("%s%s%s%s%s", pref, msg, left_paren, R_(perr()), right_paren ); \
} while(0);

#define iwarn_perr() do { \
    warn_perr_msg(""); \
} while (0);

#define iwarn_perr_msg(x, ...) do { \
    char *warning = str(_FISH_WARN_LENGTH); \
    snprintf(warning, _FISH_WARN_LENGTH, x, ##__VA_ARGS__); \
    _warn_perr_msg("Something's wrong: ", warning); \
} while (0);

#define warn_perr_msg(x, ...) do { \
    if (strlen(x) == 0) \
        warn("%s", perr()); \
    else { \
        char *warning = str(_FISH_WARN_LENGTH); \
        snprintf(warning, _FISH_WARN_LENGTH, x, ##__VA_ARGS__); \
        warn("%s (%s)", warning, perr()); \
    } \
} while(0); 

#define warn_perr do { \
    warn_perr_msg(""); \
} while(0);

/* Synonym for iwarn.
 * piep = 'squeak'
 */
#define piep do { \
    iwarn; \
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

#include <stdbool.h> // not always necessary ??

#include <sys/types.h>

#include <stdio.h>

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

void spr(const char *format, ...);

char *spr_(const char *format, int size, ...);

void error_pref();
void err(const char *format, ...);

//void warn(const char* s);
void warn_pref();
void warn(const char *format, ...);

void say(const char *format, ...);
void ask(const char *format, ...);
void info(const char *format, ...);
int sys(char *ret, const char *cmd);
FILE *sysr(const char *cmd);
FILE *sysw(const char *cmd);
int sysx(const char *cmd);
int sysxf(const char *orig, ...);
int sysclose(FILE *f);
int sysclose_f(FILE *f, const char *cmd);

bool yes_no();
bool yes_no_flags(int, int);

void autoflush();

bool f_sig(int signum, void *func);
void f_benchmark();
long int stoie(const char *s, int *err);
long int stoi(const char *s);
char *str(int length);
//char *strs(int length);

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

void disable_colors();
int int_length(int i);
const char *perr();

void sys_die(bool b);
void verbose_cmds(bool b);

bool f_socket_make_named(const char *filename, int *socket);
bool f_socket_make_client(int socket, int *client_socket);

/* num_read can be NULL.
 */
bool f_socket_read(int client_socket, ssize_t *num_read, char *buf, size_t max_length);

/* num_written can be NULL.
 */
bool f_socket_write(int client_socket, ssize_t *num_written, const char *buf, size_t len);

bool f_socket_close(int client_socket);

bool socket_unix_message(const char *filename, const char *msg);
bool socket_unix_message_f(const char *filename, const char *msg, char *response, int buf_length);

char *_s, *_t, *_u, *_v, *_w, *_x, *_y, *_z;
void _();

double time_hires();

char *f_field(int width, char *string, int max_len);

bool is_int_str(char *s);
bool is_int_strn(char *s, int maxlen);

void fish_util_cleanup();
void chop(char *s);
void chop_w(wchar_t *s);

char *comma(int n);
char *reverse(char *s);

int f_get_static_str_length();

wchar_t *d8(char *s);

bool f_set_utf8();
bool f_set_utf8_f(int flags);

FILE *safeopen(char *filespec);
FILE *safeopen_f(char *filespec, int flags);


struct stat *f_stat_f(const char *file, int flags);
struct stat *f_stat(const char *file);

bool test_f(const char *file);
bool test_d(const char *file);

char *f_get_warn_prefix(char *file, int line);

int f_get_max_color_length();
int f_get_color_reset_length();

/* guard */
#endif
/* guard */


