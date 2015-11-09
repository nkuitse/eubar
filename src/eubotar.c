#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <blake2.h>
#include <libtar.h>
#include "eub.h"
#include "arg.h"

char *argv0;

void
usage(void) {
    fputs("usage: eubotar [-1] [BASE]\n", stderr);
    exit(1);
}

int
main(int argc, char **argv) {
    struct eub eub;
    struct eubfile file;
    int opt_P = 0;
    TAR *tarp;
    char buf[512];

    eub_init(&eub);
    ARGBEGIN {
        case '1' : eub.onefile = 1;
                   break;
        case 'P' : opt_P = 1;
                   break;
        default  : usage();
    } ARGEND;

    if (argc == 1) {
        if (eub_open(&eub, argv[0], "r"))
            return(eub.err);
    }
    else if (eub.onefile) {
        eub.idata = eub.imeta = stdin;
    }
    else {
        usage();
    }
    if (tar_fdopen(&tarp, 1, "/dev/stdout", NULL, O_WRONLY, 0600, TAR_GNU) != 0)
        return(eub_err(&eub, errno, "Can't start tar output"));
    while (eub_read_meta(&eub, &file)) {
        unsigned long long len, remlen;
        if (eub_meta_to_stat(&eub, &file))
            return(eub.err);
        th_set_path(tarp, file.path);
        th_set_from_stat(tarp, &file.stat);
        /* XXX Hack! */
        if (file.typechar == 'f' && file.size == 0) {
            len = strlen(tarp->th_buf.name);
            if (tarp->th_buf.name[len-1] == '/') {
                /* Argh!! */
                tarp->th_buf.name[len-1] = 0;
            }
        }
        if (file.typechar == 'f') {
            if (eub_read_dataref(&eub, &file))
                return(eub.err);
        }
        else if (file.typechar == 'l') {
            if (eub_read_dataref(&eub, &file))
                return(eub.err);
            if (eub_seek_data(&eub, &file))
                return(eub.err);
            if (eub_read_data(&eub, &file, eub.linkbuf, file.size))
                return(eub_err(&eub, errno, "Can't read link: %s", file.path));
            eub.linkbuf[file.size] = 0;
            th_set_link(tarp, eub.linkbuf);
        }
        th_finish(tarp);
        if (th_write(tarp))
            return(eub_err(&eub, errno, "Can't write tar header for %s", file.path));
        fflush(stdout);
        if (file.typechar == 'f') {
            if (eub_seek_data(&eub, &file))
                return(eub.err);
            for (remlen = file.size; remlen; remlen -= len) {
                len = remlen < sizeof(buf) ? remlen : sizeof(buf);
                if (eub_read_data(&eub, &file, buf, len))
                    return(eub_err(&eub, errno, "Can't read file contents: %s", file.path));
                if (len < sizeof(buf))
                    bzero(buf+len, sizeof(buf)-len);
                if (!tar_block_write(tarp, buf))
                    return(eub_err(&eub, errno, "Can't write to tar: %s", file.path));
                fflush(stdout);
            }
            if (eub.err)
                return(eub.err);
        }
    }
    if (tar_append_eof(tarp))
        return(eub_err(&eub, errno, "Can't finish tar"));
    fflush(stdout);
    return(eub.err);
}

