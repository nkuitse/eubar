#ifndef _RABIN_H
#define _RABIN_H

// taken from pcompress implementation
// https://github.com/moinakg/pcompress
#define RAB_POLYNOMIAL_CONST 153191
#define RAB_WINDOW_SIZE      48
#define RAB_AVG_BLOCKSIZE    65535


struct rabin_node_t {
    unsigned long long value;
    struct rabin_node_t *next;
};

typedef struct {
    unsigned long long prime;
    unsigned long      window_size;
    unsigned long      min_blk_sz;
    unsigned long      avg_blk_sz;
    unsigned long      max_blk_sz;
    unsigned long long blk_sz;
    unsigned long long fingerprint;
    unsigned long long polynomial_map[256];
    struct rabin_node_t cycle[RAB_WINDOW_SIZE];
    struct rabin_node_t* curr;
    unsigned char      *buffer;

} rabin_splitter_t;

// define chunk handling callback type
typedef void (*rsp_chunk_callback)(unsigned long long blk_sz,
                                   unsigned char *chunk,
                                   void *userdata);

// initialize a new rabin splitter, ready for use.
rabin_splitter_t* rabin_splitter_init(unsigned long min_block_size,
                                                unsigned long max_block_size);

// reset a splitter to allow it to be used on a new file
void rabin_splitter_reset(rabin_splitter_t *splitter);

// free all memory associated with a splitter
void rabin_splitter_free(rabin_splitter_t * splitter);

// fingerprint a given file
int rabin_split_file(rabin_splitter_t * splitter,
                     int fileno,
                     rsp_chunk_callback cb,
                     void *userdata);



#endif /* RABIN_H */

