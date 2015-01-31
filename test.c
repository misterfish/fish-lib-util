#include <fish-util.h>
#include <fish-utils.h>

int main(int argc, char** argv) {
    fish_utils_init();

    info("Info works");
    ask("If it wasn't working, would I ask");
    say("");

    verbose_cmds(true);
    sysx("ls /tmp/tmp/tmp/tmp");

    int i = 17;
    char *sev = "seventeen";

    _();
    spr("%d", i);
    Y(_s);
    BB(sev);

    info("When I was %s, I wrote it %s.", _t, _u);

    char *regex[2] = {
        "v .* n",
        "v .. n"
    };

    for (int i = 0; i < 2; i++) {
        char *re = regex[i];
        _();
        BB(sev);
        Y(re);
        ask("Does %s contain %s", _s, _t);
        say("");
        info(match(sev, re) ? "yes" : "no");
    }
}
