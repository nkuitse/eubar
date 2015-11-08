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

#define MUST(x,e) if ((x)!=0) { exit(e); }

char *argv0;

void
usage(void) {
    fputs("usage: eubar [-p] [-h HASHLEN] [BASE]\n", stderr);
    exit(1);
}

void
create_from_paths(struct eub *eub, struct eubfile *file) {
    file->path = &eub->pathbuf[0];
    while (eub_read_path(eub, file)) {
        MUST(eub_stat(eub, file), eub->err);
        MUST(eub_write_meta(eub, file), eub->err);
        MUST(eub_write_data(eub, file), eub->err);
    }
    MUST(eub_write_meta_footer(eub), eub->err);
}

void
create_from_meta(struct eub *eub, struct eubfile *file) {
    file->path = &eub->pathbuf[0];
    while (eub_read_meta(eub, file)) {
        file->metalen = strlen(eub->metabuf);
        MUST(eub->err, eub->err);
        MUST(eub_write_meta(eub, file), eub->err);
        MUST(eub_write_data(eub, file), eub->err);
    }
    MUST(eub_write_meta_footer(eub), eub->err);
}

int
main(int argc, char **argv) {
    struct eub eub;
    struct eubfile file;
    int opt_m = 0;
    size_t opt_h = 0;

    if (eub_init(&eub) != 0)
        eub_die(&eub, "init failed\n");
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
    eub.ipath = stdin;
    if (argc == 1) {
        MUST(eub_open(&eub, argv[0], "w"), eub.err);
    }
    else if (eub.onefile) {
        eub.odata = eub.ometa = stdout;
    }
    else {
        eub.odata = stdout;
        eub.ometa = stderr;
    }
    if (!eub.onefile)
        MUST(eub_write_meta_header(&eub), eub.err);
    MUST(eub_write_data_header(&eub), eub.err);
    if (opt_m)
        create_from_meta(&eub, &file);
    else
        create_from_paths(&eub, &file);
    return(0);
}

