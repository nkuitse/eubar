#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <blake2.h>
#include "eub.h"
#include "arg.h"

char *argv0;

void
usage(void) {
    fputs("usage: eubar [-m1] [-k ID] [-t SEC] [-h HASHLEN] [BASE]\n", stderr);
    exit(1);
}

void
create_from_paths(struct eub *eub, struct eubfile *file) {
    file->path = &eub->pathbuf[0];
    while (eub_read_path(eub, file)) {
        if (eub_stat(eub, file) || eub_write_meta(eub, file) || eub_write_data(eub, file))
            exit(eub->err);
    }
    if (eub->err || eub_write_meta_footer(eub))
        exit(eub->err);
}

void
create_from_meta(struct eub *eub, struct eubfile *file) {
    file->path = &eub->pathbuf[0];
    while (eub_read_meta(eub, file)) {
        if (eub_write_meta(eub, file) || eub_write_data(eub, file))
            exit(eub->err);
    }
    if (eub->err)
        exit(eub->err);
    if (eub_write_meta_footer(eub))
        exit(eub->err);
}

int
main(int argc, char **argv) {
    struct eub eub;
    struct eubfile file;
    int opt_m = 0;
    size_t opt_h = 0;

    eub_init(&eub);
    ARGBEGIN {
        case 'm' : opt_m = 1;
                   break;
        case '1' : eub.onefile = 1;
                   break;
        case 'k' : eub.id = EARGF(usage());
                   break;
        case 't' : eub.begin = strtoull(EARGF(usage()), NULL, 10);
                   break;
        case 'h' : opt_h = atoi(EARGF(usage()));
                   if (opt_h < 1 || opt_h > 64)
                       usage();
                   if (((opt_h << 1) - 1) != ((opt_h - 1) | opt_h))
                       usage();  /* Not a power of 2 */
                   eub.hashlen = opt_h;
                   break;
        default  : usage();
    } ARGEND;
    if (opt_m)
        eub.imeta = stdin;
    else
        eub.ipath = stdin;
    if (argc == 1) {
        if (eub_open(&eub, argv[0], "w"))
            exit(eub.err);
    }
    else if (eub.onefile) {
        eub.odata = eub.ometa = stdout;
    }
    else {
        eub.odata = stdout;
        eub.ometa = stderr;
    }
    if (eub_write_header(&eub))
        exit(eub.err);
    if (opt_m)
        create_from_meta(&eub, &file);
    else
        create_from_paths(&eub, &file);
    return(0);
}

