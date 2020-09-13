#include "fish-util.h"

int main (int argc, char** argv) {

    info ("info.");
    ask ("If it wasn't working, would I ask");
    say ("something to say");

    f_verbose_cmds (true);
    sys ("ls /tmp/tmp/tmp/tmp");

    int i = 17;
    char *sev = "seventeen";

    _ ();
    spr ("%d", i);
    Y (_s);
    BB (sev);

    info ("When I was %s, I wrote it %s.", _t, _u);

    char *regex[2] = {
        "v .* n",
        "v .. n"
    };

    double now_old = f_time_hires_old ();
    double now = f_time_hires ();
    info ("Time hires (ftime        ) is %f", now_old);
    info ("Time hires (clock_gettime) is %f", now);

    return 0;

    /* for (int i = 0; i < 2; i++) {
       char *re = regex[i];
       _ ();
       BB (sev);
       Y (re);
       ask ("Does %s contain %s", _s, _t);
       say ("");
       info (match (sev, re) ? "yes" : "no");
       } */
}
