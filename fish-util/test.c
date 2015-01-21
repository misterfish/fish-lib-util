#include <fish-util.h>

int main(int argc, char** argv) {
    info("works");

    int i = 17;
    char *sev = "seventeen";

    _();
    spr("%d", i);
    Y(_s);
    BB(sev);

    info ("Digits %s, write it like %s", _t, _u);

    /* Future maybe nicer:
    f_("spr", "%d", i);
    f_("Y");
    f_("BB", sev);

    info ("Digits %s, write it like %s", f_(), f_());
    */
}
