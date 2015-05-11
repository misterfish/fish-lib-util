/*
 * Author: Allen Haim <allen@netherrealm.net>, © 2015.
 * Source: github.com/misterfish/fish-lib-util
 * Licence: GPL 2.0
 */

/*
 * gcc fish-util.o test.c -lm -o test 
 */
#include "fish-util.h"

int main(int argc, char **argv) {
    sysx("ls /tmp/adbsadsf");
    //ierr_perr();
    //ierr_msg("hello");
    //iwarn();
    piep;
    //iwarn_msg("hello");
    //iwarn;
    //iwarn_perr_msg("hello");
    warn_perr_msg("hello %d", 42);
}

