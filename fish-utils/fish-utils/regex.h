const int MATCH_EXTENDED;

int match(char *target, char *regexp_s);
// caller should free ret
int match_matches(char *target, char *regexp_s, char *ret[]);
int match_matches_flags(char *target, char *regexp_s, char *ret[], int flags);
int match_flags(char *target, char *regexp_s, int flags);
int match_full(char *target, char *regexp_s, char *ret[], int target_len /* without \0 */, int flags);
