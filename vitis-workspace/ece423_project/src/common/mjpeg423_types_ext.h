#ifndef _MJPEG423_TYPES_EXT_H
#define _MJPEG423_TYPES_EXT_H

#include <stdint.h>

#include "ff.h"

typedef struct {
    FIL              fil;

    /* Header */

    uint32_t         num_frames;
    uint32_t         w_size;
    uint32_t         h_size;
    uint32_t         num_iframes;

    /* Trailer */

    iframe_trailer_t *iframes;

    /* Player */

    uint32_t         displayed_frame_index;
    uint32_t         last_iframe;
    uint32_t         next_iframe;

    /* Decoder */

    uint32_t decoded_frame_index;
    /* XXX: may be different for Y and CbCr components */
    uint32_t         w_blocks;
    uint32_t         h_blocks;
    uint32_t         num_blocks;
    /* One frame of 8x8 blocks of YCbCr pixels */
    struct {
        color_block_t *y;
        color_block_t *cb;
        color_block_t *cr;
    } ycbcr_blocks;
    /* One frame of 8x8 blocks of AC and DC coefficients */
    struct {
        dct_block_t *y;
        dct_block_t *cb;
        dct_block_t *cr;
    } dct_blocks;
    /* Holds y, then cb, then cr bitstreams */
    uint8_t          *bitstreams;
} video_t;

#endif /* _MJPEG423_TYPES_EXT_H */
