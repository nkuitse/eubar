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

unsigned int
hash_path(libtar_listptr_t key, unsigned int mod)
{
    /* djb hash function */
    register unsigned h;
    register int i;
    register char *p;
    if (!key)
        return(0);
    p = (char *) key->data;
    for (h = 5381, i = 0; *p; i++)
        h = (h << 5) + h + *p++;
    return(h % mod);
}

int
compare_paths(void *data1, void *data2) {
    libtar_listptr_t lp1 = (libtar_listptr_t) data1;
    libtar_listptr_t lp2 = (libtar_listptr_t) data2;
    char *p1 = (char *) lp1->data;
    char *p2 = (char *) lp2->data;
    return (strcmp(p1, p2) == 0);
}

int
main(int argc, char **argv) {
    struct eub eub;
    struct eubfile file;
    int opt_P = 0;
    TAR *tarp;
    char buf[512];
    libtar_hash_t *lthash = NULL;
    
    eub_init(&eub);
    ARGBEGIN {
        case 'P' : opt_P = 1;
                   break;
        default  : usage();
    } ARGEND;

    if (argc == 0)
        usage();
    if (eub_open(&eub, argv[0], "r"))
        return(eub.err);
    if (argc > 1) {
        lthash = libtar_hash_new(3, (libtar_hashfunc_t) hash_path);
        while (--argc)
            libtar_hash_add(lthash, ++argv);
    }

    if (tar_fdopen(&tarp, 1, "/dev/stdout", NULL, O_WRONLY, 0600, TAR_GNU) != 0)
        return(eub_err(&eub, errno, "Can't start tar output"));
    while (eub_read_meta(&eub, &file)) {
        unsigned long long len, remlen;
        struct libtar_node lp = { file.path, 0, 0 };
        libtar_hashptr_t hp;
        hp.bucket = -1;
        hp.node = NULL;
        /*
        if (lthash && !libtar_hash_search(lthash, &hp, &lp, (libtar_matchfunc_t) compare_paths))
        */
        if (lthash && !libtar_hash_getkey(lthash, &hp, &lp, (libtar_matchfunc_t) compare_paths))
            continue;
        if (eub_meta_to_stat(&eub, &file))
            return(eub.err);
        th_set_from_stat(tarp, &file.stat);
        th_set_path(tarp, file.path);
        if (file.typechar == 'f') {
            /* XXX Hack!
            len = strlen(tarp->th_buf.name);
            if (tarp->th_buf.name[len-1] == '/')
                tarp->th_buf.name[len-1] = 0;
            */
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

