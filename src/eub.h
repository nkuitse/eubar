#define EUB_MAGIC_META "#eubar meta 1.0\n"
#define EUB_MAGIC_LEN 16

#define SEG_SHIFT (26)
#define SEG_LEN (1<<SEG_SHIFT)
#define SEG_MASK ((1<<SEG_SHIFT)-1)

#define PATH_BUF_LEN (PATH_MAX+1)
#define LINK_BUF_LEN (PATH_MAX+1)
#define META_BUF_LEN (PATH_MAX*2+1)
#define COPY_BUF_LEN 8192

#define EUB_EXT_DATA ".eud"
#define EUB_EXT_META ".eum"
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

void eub_init(struct eub *eub);
int eub_open(struct eub *eub, char *path, char *mode);
size_t eub_read_path(struct eub *eub, struct eubfile *file);
int eub_read_meta(struct eub *eub, struct eubfile *file);
int eub_read_data_ref(struct eub *eub, struct eubfile *file);
int eub_seek_data(struct eub *eub, struct eubfile *file);
int eub_read_data(struct eub *eub, struct eubfile *file, char *buf, size_t len);
int eub_write_meta(struct eub *eub, struct eubfile *file);
int eub_stat(struct eub *eub, struct eubfile *file);
size_t eub_meta(struct eub *eub, struct eubfile *file);
int eub_write_meta_header(struct eub *eub);
int eub_write_data_header(struct eub *eub);
int eub_write_data(struct eub *eub, struct eubfile *file);
int eub_err(struct eub *eub, int err, char *fmt, ...);
void eub_die(struct eub *eub, char *fmt, ...);
