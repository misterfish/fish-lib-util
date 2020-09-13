#include <unistd.h>

#include "fish-util.h"

int main(int argc, char **argv) {
    _ ();
    unlink ("./non-existent");
    ierr_perr ("error deleting ./non-existent");
    piep;
    iwarn ("hello");
    iwarn_perr ("hello");
    warn_perr ("hello %d", 42);
    return 0;
}

