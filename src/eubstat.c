#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <blake2.h>
#include "eub.h"
#include "arg.h"

char *argv0;

void
usage(void) {
    fputs("usage: eubstat [BASE]\n", stderr);
    exit(1);
}

int
main(int argc, char **argv) {
    struct eub eub;
    struct eubfile file;
    size_t opt_h = 0;

    if (eub_init(&eub) != 0)
        eub_die(&eub, "init failed\n");
    ARGBEGIN {
        default  : usage();
    } ARGEND;
    eub.ipath = stdin;
    eub.ometa = stdout;
    if (eub_write_meta_header(&eub) != 0)
        exit(eub.err);
    while (eub_read_path(&eub, &file)) {
        if (eub_stat(&eub, &file) == 0)
            (void) eub_write_meta(&eub, &file);
    }
    if (eub_write_meta_footer(&eub) != 0)
        exit(eub.err);
    return(0);
}
