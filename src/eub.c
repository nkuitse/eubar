#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdarg.h>
#include <error.h>
#include <time.h>
#include <blake2.h>
#include "eub.h"

unsigned long long eub_util_strkeyval(char **pp, char key, int base);
char eub_util_mode2typechar(mode_t mode);

void
eub_init(struct eub *eub) {
    bzero(eub, sizeof *eub);
}

int
eub_open(struct eub *eub, char *path, char *mode) {
    char *pmeta, *pdata;
    FILE *xdata, *xmeta;
    size_t pathlen;
    
    pathlen = strlen(path);
    if (pathlen + EUB_EXT_LEN >= PATH_MAX)
        exit(eub_err(eub, ENAMETOOLONG, "Archive path too long"));
    memcpy(eub->pathbuf, path, pathlen);
    /* Open data file */
    eub->pathbuf[pathlen] = 0;
    pdata = strcat(eub->pathbuf, EUB_EXT_DATA);
    errno = 0;
    if (!(xdata = fopen(pdata, mode)))
        exit(eub_err(eub, errno, "Can't open data file: %s, pdata"));
    /* Open metadata file */
    eub->pathbuf[pathlen] = 0;
    pmeta = strcat(eub->pathbuf, EUB_EXT_META);
    errno = 0;
    if (!(xmeta = fopen(pmeta, mode)))
        exit(eub_err(eub, errno, "Can't open metadata file: %s", pmeta));
    if (strchr(mode, 'w')) {
        eub->odata = xdata;
        eub->ometa = xmeta;
    }
    else {
        eub->idata = xdata;
        eub->imeta = xmeta;
    }
    return(0);
}

size_t
eub_read_path(struct eub *eub, struct eubfile *file) {
    size_t len;
    if (!fgets(eub->pathbuf, PATH_BUF_LEN, eub->ipath))
        return(0);
    len = strlen(eub->pathbuf);
    if (eub->pathbuf[len-1] == '\n')
        eub->pathbuf[--len] = 0;
    file->path = eub->pathbuf;
    file->metalen = 0;
    return(len);
}

int
eub_read_meta(struct eub *eub, struct eubfile *file) {
    size_t len;
    char *p;
    eub->err = errno = 0;
    while (fgets(eub->metabuf, META_BUF_LEN, eub->imeta)) {
        if (eub->metabuf[0] == '#' || eub->metabuf[0] == '$')
            continue;
        len = strlen(eub->metabuf);
        if (eub->metabuf[len-1] == '\n')
            eub->metabuf[--len] = 0;
        if (p = strchr(eub->metabuf, '*'))
            file->size = strtoull(++p, &p, 10);
        file->metalen = len;
        file->action = eub->metabuf[0];
        file->typechar = eub->metabuf[1];
        file->path = strchr(eub->metabuf, '/');
        /* XXX Parse metadata here? */
        return(len);
    }
    if (ferror(eub->imeta))
        eub_err(eub, errno, "Can't read metadata");
    return(0);
}

int
eub_read_dataref(struct eub *eub, struct eubfile *file) {
    size_t len;
    unsigned long long size = 0;
    char *p;
    eub->err = errno = 0;
    if (!fgets(eub->metabuf, META_BUF_LEN, eub->imeta))
        return(eub_err(eub, errno, "File data ref expected, found EOF: %s", file->path));
    len = strlen(eub->metabuf);
    if (eub->metabuf[len-1] == '\n')
        eub->metabuf[--len] = 0;
    if (p = strchr(eub->metabuf, '*')) {
        size = strtoull(++p, &p, 10);
        if (!p)
            return(eub_err(eub, -1, "Malformed file size: %s", file->path));
        if (size != file->size)
            return(eub_err(eub, -1, "File size mismatch: %s", file->path));
    }
    if (size == 0) {
        file->pos = 0;
    }
    else if (eub->metabuf[0] == '@') {
        file->pos = strtoull(eub->metabuf+1, &p, 10);
        if (!p)
            return(eub_err(eub, -1, "Unparseable file data pos: %s", file->path));
    }
    else {
        return(eub_err(eub, errno, "Missing pos in file data ref: %s", file->path));
    }
    return(0);
}

int
eub_seek_data(struct eub *eub, struct eubfile *file) {
    if (fseeko(eub->idata, file->pos, SEEK_SET))
        return(eub_err(eub, errno, "Can't seek in data"));
    return(0);
}

int
eub_read_data(struct eub *eub, struct eubfile *file, char *buf, size_t len) {
    size_t read;
    if ((read = fread(buf, len, 1, eub->idata)) != 0)
        return(0);
    else if (ferror(eub->idata))
        return(eub_err(eub, errno, "Can't read data for %s", file->path));
}

int
eub_stat(struct eub *eub, struct eubfile *file) {
    eub->err = errno = 0;
    if (lstat(file->path, &file->stat) < 0)
        return(eub_err(eub, errno, "Can't stat: %s", file->path));
    file->typechar = eub_util_mode2typechar(file->stat.st_mode);
    file->action = '+';
    file->size = file->stat.st_size;
    return(0);
}

size_t
eub_meta(struct eub *eub, struct eubfile *file) {
    char *mp;
    long dev, ino, uid, gid, rdev, mtime, ctime;
    unsigned perm;
    struct stat *stat;
    
    if (file->metalen)
        return(file->metalen);
    stat = &file->stat;
    perm = stat->st_mode;
    /* perm = stat->st_mode & ~S_IFMT; */
    dev = stat->st_dev;
    ino = stat->st_ino;
    uid = stat->st_uid;
    gid = stat->st_gid;
    rdev = stat->st_rdev;
    mtime = stat->st_mtime;
    ctime = stat->st_ctime;
    mp = eub->metabuf;
    *mp++ = file->action;
    *mp++ = file->typechar;
    mp += sprintf(mp, " d%ld i%ld r%ld p%o u%ld g%ld m%ld c%ld", dev, ino, rdev, perm, uid, gid, mtime, ctime);
    *mp++ = 0;
    return(mp - eub->metabuf - 1);
}

int
eub_meta_to_stat(struct eub *eub, struct eubfile *file) {
    struct stat *stat = &file->stat;
    char *p, action, typechar;

    p = eub->metabuf;
    bzero(stat, sizeof(struct stat));
    if (!(action = *p++))
        return(eub_err(eub, -1, "Unparseable metadata: %s", eub->metabuf));
    if (!(typechar = *p++))
        return(eub_err(eub, -1, "Unparseable metadata: %s", eub->metabuf));
    stat->st_dev   = eub_util_strkeyval(&p, 'd', 10);
    stat->st_ino   = eub_util_strkeyval(&p, 'i', 10);
    stat->st_rdev  = eub_util_strkeyval(&p, 'r', 10);
    stat->st_mode  = eub_util_strkeyval(&p, 'p',  8);
    stat->st_uid   = eub_util_strkeyval(&p, 'u', 10);
    stat->st_gid   = eub_util_strkeyval(&p, 'g', 10);
    stat->st_mtime = eub_util_strkeyval(&p, 'm', 10);
    stat->st_ctime = eub_util_strkeyval(&p, 'c', 10);
    if (S_ISREG(stat->st_mode) || S_ISLNK(stat->st_mode))
        stat->st_size = eub_util_strkeyval(&p, '*', 10);
    return(0);
}

int
eub_write_meta(struct eub *eub, struct eubfile *file) {
    size_t len;

    errno = eub->err = 0;
    if (file->metalen == 0) {
        size_t pathlen, hashlen;
        char *mp, *path, *hash, t;
        unsigned long long pos;
        struct stat *stat;

        stat = &file->stat;
        len = eub_meta(eub, file);
        mp = eub->metabuf + len;
        path = file->path;
        pathlen = strlen(path);
        pos = file->pos;
        hash = &file->hash[0];
        hashlen = file->hashlen;
        t = file->typechar;
        if (t == 'f' || t == 'l')
            mp += sprintf(mp, " *%lld", file->size);
        *mp++ = ' ';
        memcpy(mp, path, pathlen);
        mp += pathlen;
        *mp++ = 0;
        file->metalen = mp - eub->metabuf;
    }
    len = fprintf(eub->ometa, "%s\n", eub->metabuf);
    if (!len)
        return(eub_err(eub, errno, "Can't write metadata: %s", file->path));
    return(0);
}

int
eub_write_header(struct eub *eub) {
    if (eub->ometa)
        if (eub_write_meta_header(eub))
            return(eub->err);
    if (eub->odata)
        if (eub_write_data_header(eub))
            return(eub->err);
    return(0);
}

int
eub_write_meta_header(struct eub *eub) {
    eub->err = errno = 0;
    if (!fwrite(EUB_MAGIC_META, EUB_MAGIC_LEN, 1, eub->ometa))
        return(eub_err(eub, errno, "Can't write meta header"));
    if (eub->id && !fprintf(eub->ometa, "$id %s\n", eub->id))
        return(eub_err(eub, errno, "Can't write meta header"));
    if (!eub->begin)
        eub->begin = time(NULL);
    if (!fprintf(eub->ometa, "$begin %lld\n", eub->begin))
        return(eub_err(eub, errno, "Can't write meta header"));
    if (eub->hashlen)
        fprintf(stderr, "$hash BLAKE2b/%d\n", eub->hashlen * 8);
    return(0);
}

int
eub_write_data_header(struct eub *eub) {
    char *magic;
    
    magic = EUB_MAGIC_DATA;
    eub->err = errno = 0;
    if (!fwrite(magic, EUB_MAGIC_LEN, 1, eub->odata))
        return(eub_err(eub, errno, "Can't write data header"));
    eub->curpos += EUB_MAGIC_LEN;
    return(0);
}

int
eub_write_data(struct eub *eub, struct eubfile *file) {
    unsigned long long pos, size;
    char *hash, *path, *mp, *copybuf, *linkbuf, t;
    size_t hashlen, pathlen;
    ssize_t m, n;
    blake2b_state b2state;

    pos = file->pos = eub->curpos;
    size = n = file->size;
    t = file->typechar;
    path = file->path;
    hash = &file->hash[0];
    hashlen = eub->hashlen;
    pathlen = strlen(path);
    copybuf = &eub->copybuf[0];
    linkbuf = &eub->linkbuf[0];
    eub->err = errno = 0;
    mp = eub->metabuf;
    if (t == 'f') {
        FILE *fp;
        if(!(fp = fopen(path, "r")))
            return(eub_err(eub, errno, "Can't open %s", path));
        if (hashlen && blake2b_init(&b2state, hashlen) < 0)
            exit(eub_err(eub, errno, "Can't initialize hash"));
        for(; (m = (n < COPY_BUF_LEN) ? n : COPY_BUF_LEN); n -= m) {
            if (!fread(copybuf, m, 1, fp)) {
                (void) fclose(fp);
                return(eub_err(eub, errno, "Can't read from %s", path));
            }
            if (hashlen && blake2b_update(&b2state, copybuf, m) < 0)
                exit(eub_err(eub, errno, "Can't initialize hash"));
            if (!fwrite(copybuf, m, 1, eub->odata))
                exit(eub_err(eub, errno, "Can't write file %s", path));
        }
        (void) fclose(fp);
        mp += sprintf(mp, "@%lld *%lld", pos, size);
        if (hashlen) {
            if (blake2b_final(&b2state, hash, hashlen) < 0)
                exit(eub_err(eub, errno, "Can't finalize hash"));
            *mp++ = ' ';
            *mp++ = '#';
            while (hashlen--)
                mp += sprintf(mp, "%02hhx", *hash++);
        }
    }
    else if (t == 'l') {
        if ((size = readlink(path, linkbuf, LINK_BUF_LEN)) < 1)
            return(eub_err(eub, errno, "Can't read link: %s", path));
        file->size = size;
        mp += sprintf(mp, "@%lld *%lld", pos, size);
        if (!fwrite(linkbuf, size, 1, eub->odata))
            return(eub_err(eub, errno, "Can't write link: %s", path));
    }
    else {
        return(0);
    }

    *mp++ = ' ';
    memcpy(mp, path, pathlen);
    mp += pathlen;
    *mp++ = '\n';
    file->metalen = mp - eub->metabuf;
    eub->curpos += size;
    if (!fwrite(eub->metabuf, file->metalen, 1, eub->ometa))
        return(eub_err(eub, errno, "Can't write meta: %s", path));
    return(0);
}

int
eub_write_meta_footer(struct eub *eub) {
    if (!fprintf(eub->ometa, "$end %lld\n", (unsigned long long) time(NULL))
            || !fprintf(eub->ometa, "$size %lld\n", eub->curpos)
            || !fwrite("\n", 1, 1, eub->ometa))
        return(eub_err(eub, errno, "Can't write meta footer"));
    return(0);
}

int
eub_err(struct eub *eub, int err, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    (void) fwrite("\n", 1, 1, stderr);
    return(eub->err = err);
}

void
eub_die(struct eub *eub, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(2);
}

char
eub_util_mode2typechar(mode_t mode) {
    switch (mode & S_IFMT) {
    case S_IFREG:  return('f');
    case S_IFDIR:  return('d');
    case S_IFLNK:  return('l');
    case S_IFBLK:  return('b');
    case S_IFCHR:  return('c');
    case S_IFIFO:  return('p');
    case S_IFSOCK: return('s');
    default:       return('?');
    }
}

unsigned long long
eub_util_strkeyval(char **pp, char key, int base) {
    char *p = *pp, c;
    unsigned long long val = 0;
    if (!*pp)
        return(0);
    if ((c = *p++) == ' ') {
        if ((c = *p++) == key)
            val = strtoull(p, pp, base);
        else
            *pp = NULL;
    }
    else {
        *pp = NULL;
    }
    return(val);
}

