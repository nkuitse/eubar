#define EUB_MAGIC_DATA "#eubar data 1.0\n"
#define EUB_MAGIC_META "#eubar meta 1.0\n"
#define EUB_MAGIC_BOTH "#eubar arch 1.0\n"
#define EUB_MAGIC_LEN 16

#define PATH_BUF_LEN (PATH_MAX+1)
#define LINK_BUF_LEN (PATH_MAX+1)
#define META_BUF_LEN (PATH_MAX*2+1)
#define COPY_BUF_LEN 8192

#define EUB_EXT_DATA ".eud"
#define EUB_EXT_META ".eum"
#define EUB_EXT_BOTH ".eub"
#define EUB_EXT_LEN 4

struct eub {
    char pathbuf[PATH_BUF_LEN];
    char linkbuf[LINK_BUF_LEN];
    char metabuf[META_BUF_LEN];
    char copybuf[COPY_BUF_LEN];

    FILE *ipath;
    FILE *idata;
    FILE *imeta;
    FILE *odata;
    FILE *ometa;

    int onefile;
    char *errpfx;
    int err;
    unsigned long long curpos;

    size_t hashlen;
    char b2sum[64];
    blake2b_state b2state;

    /* Metadata for the archive itself */
    char *id;
    unsigned long long begin;
    unsigned long long end;
};

struct eubfile {
    char   action;
    char   typechar;
    size_t metalen;
    unsigned long long pos;
    unsigned long long size;
    struct stat stat;
    size_t hashlen;
    char   hash[64];
    char   *path;
};

int eub_init(struct eub *eub);
int eub_open(struct eub *eub, char *path, char *mode);
size_t eub_read_path(struct eub *eub, struct eubfile *file);
size_t eub_read_meta(struct eub *eub, struct eubfile *file);
int eub_write_meta(struct eub *eub, struct eubfile *file);
int eub_stat(struct eub *eub, struct eubfile *file);
size_t eub_meta(struct eub *eub, struct eubfile *file);
int eub_write_meta_header(struct eub *eub);
int eub_write_data_header(struct eub *eub);
int eub_write_data(struct eub *eub, struct eubfile *file);
int eub_err(struct eub *eub, int err, char *fmt, ...);
void eub_die(struct eub *eub, char *fmt, ...);
char eub_util_mode2typechar(mode_t mode);
