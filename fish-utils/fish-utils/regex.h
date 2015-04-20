#define F_REGEX_EXTENDED            0x01
/* If set, caller of match functions should free all members of the return
 * array.
 */
#define F_REGEX_NO_FREE_MATCHES     0x02

#define F_REGEX_DEFAULT             F_REGEX_EXTENDED

int match(char *target, char *regexp_s);
// caller should free ret
int match_matches(char *target, char *regexp_s, char *ret[]);
int match_matches_flags(char *target, char *regexp_s, char *ret[], int flags);
int match_flags(char *target, char *regexp_s, int flags);
int match_full(char *target, char *regexp_s, char *ret[], int target_len /* without \0 */, int flags);
