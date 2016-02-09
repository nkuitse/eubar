#include <stdlib.h>
#include <unistd.h>
#include "rabin.h"

#define _READ_SIZE (32768)

static void pregenerate_polys(rabin_splitter_t *rsp) {
    // Calculates result = prime ^ windowsize
    // After that multiplies all 256 bytes with result
    unsigned long long result = 1;
    unsigned int i;
    for (i=0; i<rsp->window_size; i++)
        result *= rsp->prime;

    for (i=0; i<=256; i++)
        rsp->polynomial_map[i] = i * result;

    return;
}

static void create_rabin_ring(rabin_splitter_t *rsp) {
    // initializes all the values to 0 and sets things up for cycling of the
    // hash window
    unsigned int i = 0;
    for (i = 0; i < (rsp->window_size-1); i++) {
        (rsp->cycle+i)->next = (rsp->cycle+i+1);
        (rsp->cycle+i)->value = 0;
    }
    // close ring
    (rsp->cycle+rsp->window_size-1)->next  = rsp->cycle;
    (rsp->cycle+rsp->window_size-1)->value = 0;
}

void rabin_splitter_reset(rabin_splitter_t *rsp) {
    // reset the ring
    create_rabin_ring(rsp);
    rsp->curr         = rsp->cycle;

    // set blocksize and fingerprint to 0
    rsp->blk_sz       = 0;
    rsp->fingerprint  = 0;

}

rabin_splitter_t* rabin_splitter_init(unsigned long min_block_size,
                                                unsigned long max_block_size) {

    // basic sanity check.
    if (min_block_size > max_block_size) {
        return 0;
    }

    rabin_splitter_t * rsp = malloc(sizeof(rabin_splitter_t));
    if (!rsp) {
        return 0;
    }

    rsp->prime       = RAB_POLYNOMIAL_CONST;
    rsp->min_blk_sz  = min_block_size;
    rsp->avg_blk_sz  = RAB_AVG_BLOCKSIZE;
    rsp->max_blk_sz  = max_block_size;
    rsp->window_size = RAB_WINDOW_SIZE;

    // pre-generate the rabin polynomials
    pregenerate_polys(rsp);

    // set up the ring buffer
    create_rabin_ring(rsp);

    // set up the buffers
    rsp->buffer       = malloc(rsp->max_blk_sz);
    if (!rsp->buffer) {
        rabin_splitter_free(rsp);
        return 0;
    }

    // reset to finish setup
    rabin_splitter_reset(rsp);

    return rsp;
}

void rabin_splitter_free(rabin_splitter_t * splitter) {
    if (splitter) {
        if (splitter->buffer) {
            free(splitter->buffer);
            splitter->buffer = NULL;
        }
        free(splitter);
        splitter = NULL;
    }
}

int rabin_split_file(rabin_splitter_t * rsp,
                     int fileno,
                     rsp_chunk_callback cb,
                     void *userdata) {
    int ch;
    int i = 0, nread = 0;
    unsigned char input_buffer[_READ_SIZE];

    // if there's no file or splitter, give up
    // likewise, if there's no callback there isn't much point ...
    // TODO: check file handle is valid
    if ((!rsp)||(!cb))
        return 1;

    // read the file in slabs of the minimum block size into the input buffer
    while ((nread = read(fileno, input_buffer, _READ_SIZE))) {

        // loop over the input buffer
        for (i = 0; i < nread; i++) {
            // read off the buffer
            ch = input_buffer[i];

            // copy to the output buffer
            rsp->buffer[rsp->blk_sz] = ch;

            // rabin stuff
            rsp->fingerprint *= rsp->prime;
            rsp->fingerprint += ch;
            rsp->fingerprint -= rsp->polynomial_map[rsp->curr->value];

            rsp->curr->value = ch;
            rsp->curr = rsp->curr->next;

            rsp->blk_sz++;
            // if we have a block we can consider outputting ...
            if ((rsp->blk_sz > rsp->min_blk_sz)) {

                // ... check for it having a valid fingerprint or being too large 
                if (((rsp->fingerprint & rsp->avg_blk_sz) == 1) || (rsp->blk_sz >= rsp->max_blk_sz)) {
                    // call the callback
                    (*cb)(rsp->blk_sz, rsp->buffer, userdata);
                    // reset the block size
                    rsp->blk_sz = 0;
                }
            }
        }
    }
    // if we have any spare bytes
    if (rsp->blk_sz != 0) // send them to the callback, too!
        (*cb)(rsp->blk_sz, rsp->buffer, userdata);
    return 0;
}
