#include "ff.h"

#include "../common/mjpeg423_types.h"
#include "../common/util.h"
#include "../config.h"
#include "../decoder/hw_idct.h"
#include "../decoder/mjpeg423_decoder.h"
#include "../ece423_vid_ctl/ece423_vid_ctl.h"
#include "../profile.h"

#define FRAME_HEADER_SIZE (4 * sizeof(uint32_t))

static struct profile sd_read_i_time;
static struct profile sd_read_p_time;
static struct profile lossless_y_i_time;
static struct profile lossless_y_p_time;
static struct profile lossless_cb_i_time;
static struct profile lossless_cb_p_time;
static struct profile lossless_cr_i_time;
static struct profile lossless_cr_p_time;
static struct profile sw_idct_time;
static struct profile ycbcr_to_rgb_time;
static struct profile buff_reg_time;

static struct profile sd_read_i_size;
static struct profile sd_read_p_size;

void mjpeg423_decode(video_t *video)
{
    UINT br;
    uint32_t frame_size, frame_type, y_size, cb_size, payload_size;
    uint8_t *y_bitstream, *cb_bitstream, *cr_bitstream;
    rgb_pixel_t *frame;
    uint32_t blocks_size, pixels_size;

    /* Check if all frames were decoded */
    if (video->decoded_frame_index == video->num_frames)
        return;

    /* Try to allocate next slot in video buffer */
    if (!(frame = buff_next()))
        return;

    DEBUG_PRINT("\nDecoding frame\n");
    DEBUG_PRINT_ARG("Frame: %lu\n", video->decoded_frame_index);

    /* Read frame header */
    PROFILE_US_START
    if (f_read(&video->fil, (void *)&frame_size, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read frame_size");
    DEBUG_PRINT_ARG("frame_size: %lu\n", frame_size);
    if (f_read(&video->fil, (void *)&frame_type, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read frame_type");
    DEBUG_PRINT_ARG("frame_type: %lu\n", frame_type);

    /* Read frame payload */
    if (f_read(&video->fil, (void *)&y_size, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read y_size");
    DEBUG_PRINT_ARG("y_size: %lu\n", y_size);
    if (f_read(&video->fil, (void *)&cb_size, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read cb_size");
    DEBUG_PRINT_ARG("cb_size: %lu\n", cb_size);
    payload_size = frame_size - FRAME_HEADER_SIZE;
    if (f_read(&video->fil, (void *)video->bitstreams, payload_size, &br) || br != payload_size)
        error_and_exit("Cannot read bitstreams");
    PROFILE_US_STOP(frame_type ? &sd_read_p_time : &sd_read_i_time);
    PROFILE_UPDATE(frame_type ? &sd_read_p_size : &sd_read_i_size, frame_size);

    y_bitstream = video->bitstreams;
    cb_bitstream = y_bitstream + y_size;
    cr_bitstream = cb_bitstream + cb_size;

    /* Lossless decoding */
    PROFILE_US_START
    lossless_decode(video->num_blocks, y_bitstream, video->dct_blocks.y, Yquant, frame_type);
    PROFILE_US_STOP(frame_type ? &lossless_y_p_time : &lossless_y_i_time);
    PROFILE_US_START
    lossless_decode(video->num_blocks, cb_bitstream, video->dct_blocks.cb, Cquant, frame_type);
    PROFILE_US_STOP(frame_type ? &lossless_cb_p_time : &lossless_cb_i_time);
    PROFILE_US_START
    lossless_decode(video->num_blocks, cr_bitstream, video->dct_blocks.cr, Cquant, frame_type);
    PROFILE_US_STOP(frame_type ? &lossless_cr_p_time : &lossless_cr_i_time);

    /* IDCT */
#if CONFIG_ENABLE_HW_IDCT
    blocks_size = video->num_blocks * sizeof(dct_block_t);
    pixels_size = video->num_blocks * sizeof(color_block_t);
    hw_idct_do(video->dct_blocks.y, blocks_size, video->ycbcr_blocks.y, pixels_size);
    hw_idct_do(video->dct_blocks.cb, blocks_size, video->ycbcr_blocks.cb, pixels_size);
    hw_idct_do(video->dct_blocks.cr, blocks_size, video->ycbcr_blocks.cr, pixels_size);
#else
    PROFILE_US_START
    for (int b = 0; b < video->num_blocks; b++) idct(video->dct_blocks.y[b], video->ycbcr_blocks.y[b]);
    PROFILE_US_STOP(&sw_idct_time);
    for (int b = 0; b < video->num_blocks; b++) idct(video->dct_blocks.cb[b], video->ycbcr_blocks.cb[b]);
    for (int b = 0; b < video->num_blocks; b++) idct(video->dct_blocks.cr[b], video->ycbcr_blocks.cr[b]);
#endif /* CONFIG_ENABLE_HW_IDCT */

    /* YCbCr to RGB conversion */
    PROFILE_US_START
    for (int b = 0; b < video->num_blocks; b++)
        ycbcr_to_rgb(b / video->w_blocks * 8, b % video->w_blocks * 8, video->w_size, video->ycbcr_blocks.y[b],
            video->ycbcr_blocks.cb[b], video->ycbcr_blocks.cr[b], frame);
    PROFILE_US_STOP(&ycbcr_to_rgb_time);

    /* XXX: assumes buff_next() success => buff_reg() success */
    PROFILE_US_START
    if (!buff_reg())
        error_and_exit("Cannot register frame");
    PROFILE_US_STOP(&buff_reg_time);

    video->decoded_frame_index++;
}

void decoder_print_profile(void)
{
	PROFILE_PRINT_RESULTS("SD Read I-Frame", "us", &sd_read_i_time);
    PROFILE_PRINT_RESULTS("SD Read P-Frame", "us", &sd_read_p_time);
    PROFILE_PRINT_RESULTS("Lossless Y I-Frame", "us", &lossless_y_i_time);
    PROFILE_PRINT_RESULTS("Lossless Y P-Frame", "us", &lossless_y_p_time);
    PROFILE_PRINT_RESULTS("Lossless CB I-Frame", "us", &lossless_cb_i_time);
    PROFILE_PRINT_RESULTS("Lossless CB P-Frame", "us", &lossless_cb_p_time);
    PROFILE_PRINT_RESULTS("Lossless CR I-Frame", "us", &lossless_cr_i_time);
    PROFILE_PRINT_RESULTS("Lossless CR P-Frame", "us", &lossless_cr_p_time);
    PROFILE_PRINT_RESULTS("SW IDCT Time", "us", &sw_idct_time);
    PROFILE_PRINT_RESULTS("YCbCr to RGB", "us", &ycbcr_to_rgb_time);
    PROFILE_PRINT_RESULTS("buff_reg()", "us", &buff_reg_time);

    PROFILE_PRINT_RESULTS("SD Read I-Frame", "bytes", &sd_read_i_size);
    PROFILE_PRINT_RESULTS("SD Read P-Frame", "bytes", &sd_read_p_size);
}
