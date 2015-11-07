#define DATA_MAGIC "#eub01t0f0k0v0\n"
#define META_MAGIC "#eubar 01 meta\n"

#define DATA_MAGIC_LEN  16
#define META_MAGIC_LEN  16

#define IO_BUF_LEN      8192
#define META_BUF_LEN    (PATH_MAX*2)
#define PATH_BUF_LEN    PATH_MAX
#define LINK_BUF_LEN    PATH_MAX

#define BLAKE2B_ALGO    1

struct eub {
    char pathbuf[PATH_MAX];
    char linkbuf[PATH_MAX];
    char iobuf[IO_BUF_LEN];
    char metabuf[META_BUF_LEN];

    FILE *ipath;
    FILE *imeta;
    FILE *odata;
    FILE *ometa;

    char *errpfx;
    int err;
    unsigned long long pos;

    size_t hashlen;
    char b2sum[64];
    blake2b_state b2state;

    unsigned long long begin;
};

struct eubfile {
    char   *path;
    unsigned long long pos;
    unsigned long long size;
    size_t metalen;
    size_t hashlen;
    char   hash[64];
    char   action;
    char   typechar;
    struct stat stat;
};

int eub_init(struct eub *eub);
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
