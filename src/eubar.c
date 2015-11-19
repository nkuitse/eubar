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
    fputs("usage: eubar [-sm] [-o BASE] [-k ID] [-t SEC] [-h HASHLEN] [CMD [ARG...]]\n", stderr);
    exit(1);
}

int
create_from_paths(struct eub *eub, struct eubfile *file) {
    int err = 0;
    file->path = &eub->pathbuf[0];
    while (eub_read_path(eub, file)) {
        if (eub_stat(eub, file) || eub_write_meta(eub, file))
            err = eub->err;
        else if (eub->odata && eub_write_data(eub, file))
            err = eub_write_data(eub, file);
    }
    return(err);
}

int
create_from_meta(struct eub *eub, struct eubfile *file) {
    int err = 0;
    file->path = &eub->pathbuf[0];
    while (eub_read_meta(eub, file)) {
        if (eub_write_meta(eub, file))
            err = eub->err;
        else if (eub->odata && eub_write_data(eub, file))
            err = eub->err;
    }
    return(err);
}

int
main(int argc, char **argv) {
    struct eub eub;
    struct eubfile file;
    int err = 0;
    char *archive = 0;

    eub_init(&eub);
    eub.ipath = stdin;
    eub.odata = stdout;
    eub.ometa = stderr;
    ARGBEGIN {
        case 's' : eub.imeta = stdin;
                   eub.ipath = 0;
                   break;
        case 'm' : eub.odata = 0;
                   eub.ometa = stdout;
                   break;
        case 'o' : if (eub_open(&eub, EARGF(usage()), "w"))
                       exit(eub.err);
                   break;
        case 'k' : eub.id = EARGF(usage());
                   break;
        case 't' : eub.begin = strtoull(EARGF(usage()), NULL, 10);
                   break;
        case 'h' : eub.hashlen = atoi(EARGF(usage()));
                   if (eub.hashlen < 1 || eub.hashlen > 64)
                       usage();
                   if (((eub.hashlen << 1) - 1) != ((eub.hashlen - 1) | eub.hashlen))
                       usage();  /* Not a power of 2 */
                   break;
        default  : usage();
    } ARGEND;

    if (eub_write_header(&eub))
        exit(eub.err);
    if (eub.ipath)
        eub.err = create_from_paths(&eub, &file);
    else
        eub.err = create_from_meta(&eub, &file);
    if (eub.err)
        return(eub.err);
    return(eub_write_meta_footer(&eub));
}

/*
    typical uses:

        find ... | eubar -o ARCH
        find ... | eubar > ARCH.eud 2> ARCHIVE.eum
        find ... | eubdiff PREVARCH | eubar -s
*/
