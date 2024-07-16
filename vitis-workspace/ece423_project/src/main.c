#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "xtime_l.h"

#include "common/mjpeg423_types.h"
#include "common/mjpeg423_types_ext.h"
#include "common/util.h"
#include "config.h"
#include "decoder/hw_idct.h"
#include "decoder/mjpeg423_decoder.h"
#include "decoder/mjpeg423_decoder_ext.h"
#include "ece423_vid_ctl/ece423_vid_ctl.h"
#include "ece423_vid_ctl/ece423_vid_ctl_ext.h"
#include "timer_gpio/timer_gpio.h"
#include "timer_gpio/timer_gpio_ext.h"

#define FPS               (24)
#define FPS_TO_COUNT(fps) (325000000 / fps)
#define MS_TO_COUNT(ms)   (325000 * ms)

#define VIDEO_HEADER_SIZE        (5 * sizeof(uint32_t))
#define VIDEO_TRAILER_ENTRY_SIZE (2 * sizeof(uint32_t))

struct node {
    struct node *next;
    void        *data;
};

struct event {
    struct event *next;
    int          fsm_input;
};

typedef void (*action_t)(void);
struct transition {
    int      next_state;
    action_t action;
};

enum {
    FSM_INPUT_LOAD_VIDEO,    /* BTN0 */
    FSM_INPUT_PAUSE_PLAY,    /* BTN1 */
    FSM_INPUT_SKIP_FORWARD,  /* BTN2 */
    FSM_INPUT_SKIP_BACKWARD, /* BTN3 */
    FSM_INPUT_NEAR_END,      /* For skip_forward() */
    FSM_INPUT_LAST_FRAME,    /* For try_display_frame() */

    /* Keep last */
    FSM_NUM_INPUTS
};

enum {
    FSM_STATE_PAUSED,
    FSM_STATE_PLAYING,
    FSM_STATE_END,

    /* Keep last */
    FSM_NUM_STATES
};

static FATFS fatfs;
struct node *filename_list;

static struct {
    struct event *head;
    struct event **tail;
} event_queue;

static void pause_load_video(void);
static void play_video(void);
static void pause_video(void);
static void skip_forward(void);
static void skip_backward(void);
static int current_state;
static struct transition fsm[FSM_NUM_INPUTS][FSM_NUM_STATES] = {
    { { FSM_STATE_PAUSED,  pause_load_video }, { FSM_STATE_PAUSED,  pause_load_video },
      { FSM_STATE_PAUSED, pause_load_video } },
    { { FSM_STATE_PLAYING, play_video       }, { FSM_STATE_PAUSED,  pause_video      },
      { FSM_STATE_END,    NULL             } },
    { { FSM_STATE_PAUSED,  skip_forward     }, { FSM_STATE_PLAYING, skip_forward     },
      { FSM_STATE_END,    NULL             } },
    { { FSM_STATE_PAUSED,  skip_backward    }, { FSM_STATE_PLAYING, skip_backward    },
      { FSM_STATE_PAUSED, skip_backward    } },
    { { FSM_STATE_PAUSED,  NULL             }, { FSM_STATE_PAUSED,  pause_video      },
      { FSM_STATE_END,    NULL             } },
    { { FSM_STATE_PAUSED,  NULL             }, { FSM_STATE_END,     pause_video      },
      { FSM_STATE_END,    NULL             } }
};

static video_t current_video;

static void init_filename_list(void)
{
    DIR dir;
    FILINFO fno;
    char *ext;
    struct node tmp;

    filename_list = &tmp;

    if (f_opendir(&dir, CONFIG_VIDEO_DIRECTORY))
        error_and_exit("Cannot open video directory");

    DEBUG_PRINT("\nVideos found:\n");

    while (1) {
        if (f_readdir(&dir, &fno))
            error_and_exit("Cannot read video directory");
        if (!fno.fname[0])
            break;
        ext = strrchr(fno.fname, '.');
        if (!(fno.fattrib & AM_DIR) && strcmp(fno.fname, CONFIG_VIDEO_IGNORE) && ext && \
        !strcmp(ext, CONFIG_VIDEO_EXTENSION)) {
            DEBUG_PRINT_ARG("%s\n", fno.fname);
            if ((filename_list->next = malloc(sizeof(struct node))) == NULL)
                error_and_exit("Cannot allocate filename node");
            filename_list = filename_list->next;
            if ((filename_list->data = malloc(strlen(CONFIG_VIDEO_DIRECTORY) + strlen(fno.fname) + 1)) == NULL)
                error_and_exit("Cannot allocate filename");
            strcpy(filename_list->data, CONFIG_VIDEO_DIRECTORY);
            strcat(filename_list->data, fno.fname);
        }
    }

    if (filename_list == &tmp)
        error_and_exit("Filename list is empty");

    filename_list->next = tmp.next;
    filename_list = filename_list->next;

    if (f_closedir(&dir))
        error_and_exit("Cannot close video directory");
}

static void transition_fsm(int input)
{
    struct transition transition = fsm[input][current_state];
    DEBUG_PRINT_ARG("\nTransitioning from state %d ", current_state);
    DEBUG_PRINT_ARG("to state %d\n", transition.next_state);
    /*
     * action() may call transition_fsm(), so current_state must be updated
     * before it is called
     */
    current_state = transition.next_state;
    if (transition.action){
        transition.action();
    }
}

static void try_display_frame(void)
{
    struct event *event;

    DEBUG_PRINT("\nDisplaying frame\n");

    /* Try to display next frame */
    if (!vdma_out()) {
        DEBUG_PRINT("No frame to display\n");
        return;
    }

    DEBUG_PRINT_ARG("Frame index: %lu\n", current_video.displayed_frame_index);

    /* Track last and next I-frame */
    if (current_video.next_iframe < current_video.num_iframes && \
    current_video.displayed_frame_index == current_video.iframes[current_video.next_iframe].frame_index) {
        current_video.last_iframe++;
        current_video.next_iframe++;
    }

    /* Check if last frame was displayed */
    if (++current_video.displayed_frame_index == current_video.num_frames) {
        if ((event = malloc(sizeof(struct event))) == NULL)
            error_and_exit("Cannot allocate last-frame event");
        event->next = NULL;
        event->fsm_input = FSM_INPUT_LAST_FRAME;
        *event_queue.tail = event;
        event_queue.tail = &event->next;
    }
}

static inline void display_first_frame(void)
{
    buff_clr();
    mjpeg423_decode(&current_video);
    try_display_frame();
}

static void play_video(void)
{
    DEBUG_PRINT("\nPlaying video\n");

#if CONFIG_ENABLE_TIMER
    timer_start(FPS_TO_COUNT(FPS));
#endif
}

static void pause_video(void)
{
    DEBUG_PRINT("\nPausing video\n");

#if CONFIG_ENABLE_TIMER
    timer_stop();
    /*
     * FIXME: Timer interrupt may be pending in GIC Distributor. Fix by writing
     *        1 to bit corresponding to timer IRQ in ICDICPR register.
     */
#endif
}

static void load_video(void)
{
    char *fname;
    UINT br;
    uint32_t payload_size, trailer_size;
    uint32_t num_pixels, dct_blocks_size;

    DEBUG_PRINT("\nLoading video\n");

    /* Cleanup previous video */
    f_close(&current_video.fil);
    if (current_video.iframes) free(current_video.iframes);
    if (current_video.ycbcr_blocks.y) free(current_video.ycbcr_blocks.y);
    if (current_video.ycbcr_blocks.cb) free(current_video.ycbcr_blocks.cb);
    if (current_video.ycbcr_blocks.cr) free(current_video.ycbcr_blocks.cr);
    if (current_video.dct_blocks.y) free(current_video.dct_blocks.y);
    if (current_video.dct_blocks.cb) free(current_video.dct_blocks.cb);
    if (current_video.dct_blocks.cr) free(current_video.dct_blocks.cr);
    if (current_video.bitstreams) free(current_video.bitstreams);

    /* Open next video file */
    fname = (char *)filename_list->data;
    filename_list = filename_list->next;
    DEBUG_PRINT_ARG("Filename: %s\n", fname);
    if (f_open(&current_video.fil, fname, FA_READ))
        error_and_exit("Cannot open file");

    /* Read video header */
    if (f_read(&current_video.fil, (void *)&current_video.num_frames, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read num_frames");
    DEBUG_PRINT_ARG("num_frames: %lu\n", current_video.num_frames);
    if (f_read(&current_video.fil, (void *)&current_video.w_size, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read w_size");
    DEBUG_PRINT_ARG("w_size: %lu\n", current_video.w_size);
    if (f_read(&current_video.fil, (void *)&current_video.h_size, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read h_size");
    DEBUG_PRINT_ARG("h_size: %lu\n", current_video.h_size);
    if (f_read(&current_video.fil, (void *)&current_video.num_iframes, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read num_iframes");
    DEBUG_PRINT_ARG("num_iframes: %lu\n", current_video.num_iframes);

    /* Read video trailer */
    if (f_read(&current_video.fil, (void *)&payload_size, sizeof(uint32_t), &br) || br != sizeof(uint32_t))
        error_and_exit("Cannot read payload size");
    DEBUG_PRINT_ARG("payload_size: %lu\n", payload_size);
    if (f_lseek(&current_video.fil, VIDEO_HEADER_SIZE + payload_size))
        error_and_exit("Cannot seek to trailer");
    trailer_size = current_video.num_iframes * VIDEO_TRAILER_ENTRY_SIZE;
    if ((current_video.iframes = malloc(trailer_size)) == NULL)
        error_and_exit("Cannot allocate iframe table");
    if (f_read(&current_video.fil, (void *)current_video.iframes, trailer_size, &br) || br != trailer_size)
        error_and_exit("Cannot read trailer");
    if (f_lseek(&current_video.fil, VIDEO_HEADER_SIZE))
        error_and_exit("Cannot seek back to header");

    /* Initialize player state */
    current_video.displayed_frame_index = 0;
    current_video.last_iframe = 0;
    current_video.next_iframe = 1;

    /* Initialize decoder state */
    current_video.decoded_frame_index = 0;
    /* XXX: assumes w_size and h_size are both multiples of 8 */
    current_video.w_blocks = current_video.w_size / 8;
    current_video.h_blocks = current_video.h_size / 8;
    current_video.num_blocks = current_video.w_blocks * current_video.h_blocks;
    num_pixels = current_video.w_size * current_video.h_size;
    if ((current_video.ycbcr_blocks.y = malloc(num_pixels)) == NULL)
        error_and_exit("Cannot allocate Y blocks");
    if ((current_video.ycbcr_blocks.cb = malloc(num_pixels)) == NULL)
        error_and_exit("Cannot allocate Cb blocks");
    if ((current_video.ycbcr_blocks.cr = malloc(num_pixels)) == NULL)
        error_and_exit("Cannot allocate Cr blocks");
    dct_blocks_size = num_pixels * sizeof(DCTELEM);
    if ((current_video.dct_blocks.y = malloc(dct_blocks_size)) == NULL)
        error_and_exit("Cannot allocate Y DCT blocks");
    if ((current_video.dct_blocks.cb = malloc(dct_blocks_size)) == NULL)
        error_and_exit("Cannot allocate CB DCT blocks");
    if ((current_video.dct_blocks.cr = malloc(dct_blocks_size)) == NULL)
        error_and_exit("Cannot allocate CR DCT blocks");
    if ((current_video.bitstreams = malloc(3 * num_pixels * sizeof(DCTELEM))) == NULL)
        error_and_exit("Cannot allocate bitstreams");

    display_first_frame();
}

static void pause_load_video(void)
{
    pause_video();
    load_video();
}

static void skip_forward(void)
{
    uint32_t i, last_i;
    uint32_t target_index;

    DEBUG_PRINT("\nSkipping forward\n");

    if (current_video.num_frames < CONFIG_PLAYER_SKIP_LEN || \
    current_video.displayed_frame_index >= current_video.num_frames - CONFIG_PLAYER_SKIP_LEN) {
        transition_fsm(FSM_INPUT_NEAR_END);
        return;
    }

    target_index = current_video.displayed_frame_index + CONFIG_PLAYER_SKIP_LEN;
    last_i = current_video.num_iframes - 1;
    for (i = current_video.last_iframe; i < last_i && \
    current_video.iframes[i].frame_index < target_index; i++);

    if (f_lseek(&current_video.fil, current_video.iframes[i].frame_position))
        error_and_exit("Cannot seek to iframe offset");
    DEBUG_PRINT_ARG("Skipped from %lu", current_video.displayed_frame_index);
    DEBUG_PRINT_ARG(" to %lu\n", current_video.iframes[i].frame_index);

    current_video.displayed_frame_index = current_video.iframes[i].frame_index;
    current_video.last_iframe = i;
    current_video.next_iframe = i + 1;
    current_video.decoded_frame_index = current_video.displayed_frame_index;

    display_first_frame();
}

static void skip_backward(void)
{
    uint32_t i, first_i;
    uint32_t target_index;

    DEBUG_PRINT("\nSkipping backward\n");

    if (current_video.displayed_frame_index < CONFIG_PLAYER_SKIP_LEN) {
        i = 0;
    } else {
        first_i = 0;
        target_index = current_video.displayed_frame_index - CONFIG_PLAYER_SKIP_LEN;
        for (i = current_video.last_iframe; i > first_i && \
        current_video.iframes[i].frame_index > target_index; i--);
    }

    if (f_lseek(&current_video.fil, current_video.iframes[i].frame_position))
        error_and_exit("Cannot seek to iframe offset");
    DEBUG_PRINT_ARG("Skipped from %lu", current_video.displayed_frame_index);
    DEBUG_PRINT_ARG(" to %lu\n", current_video.iframes[i].frame_index);

    current_video.displayed_frame_index = current_video.iframes[i].frame_index;
    current_video.last_iframe = i;
    current_video.next_iframe = i + 1;
    current_video.decoded_frame_index = current_video.displayed_frame_index;

    display_first_frame();
}

static void timer_isr(void *data)
{
#if CONFIG_ENABLE_TIMER
    try_display_frame();
#endif
}

static void gpio_isr(void *data)
{
#if !CONFIG_ENABLE_PROFILE
    int pin;
    struct event *event;
    XTime time;
    static int debounce_pin = -1;
    static XTime debounce_time = 0;

    pin = read_pin();

    /* Debounce */
    XTime_GetTime(&time);
    if (pin == debounce_pin && time < debounce_time) {
        DEBUG_PRINT_ARG("\nBTN%d debounced\n", pin);
        return;
    }
    debounce_pin = pin;
    debounce_time = time + MS_TO_COUNT(CONFIG_GPIO_DEBOUNCE_MS);

    DEBUG_PRINT_ARG("\nBTN%d pressed\n", pin);

    switch (pin) {
    case 0:
    case 1:
    case 2:
    case 3:
#if CONFIG_ENABLE_TIMER
        /* Disable timer IRQ to handle event at exact frame */
        timer_disable_irq();
#endif
        if ((event = malloc(sizeof(struct event))) == NULL)
            error_and_exit("Cannot allocate event");
        event->next = NULL;
        event->fsm_input = pin;
        *event_queue.tail = event;
        event_queue.tail = &event->next;
    }
#endif /* !CONFIG_ENABLE_PROFILE */
}

int main(int argc, const char *argv[])
{
    struct event *event;
    void *tmp;

    /* Find video(s) */
    f_mount(&fatfs, CONFIG_VIDEO_DIRECTORY, 1);
#if CONFIG_ENABLE_PROFILE
    filename_list = malloc(sizeof(struct node));
    filename_list->next = filename_list;
    filename_list->data = malloc(strlen(CONFIG_VIDEO_DIRECTORY) + strlen(CONFIG_VIDEO_PROFILE) + 1);
    strcpy(filename_list->data, CONFIG_VIDEO_DIRECTORY);
    strcat(filename_list->data, CONFIG_VIDEO_PROFILE);
#else
    filename_list = NULL;
    init_filename_list();
#endif

    /* Load first video and display first frame */
#if CONFIG_ENABLE_HW_IDCT
    hw_idct_init();
#endif
    vdma_init(CONFIG_VBUFF_WIDTH, CONFIG_VBUFF_HEIGHT, CONFIG_VBUFF_LENGTH);
    memset(&current_video, 0, sizeof(current_video));
    load_video();

    /* Initialize event queue and enable interrupts */
    current_state = FSM_STATE_PAUSED;
#if CONFIG_ENABLE_PROFILE
    event_queue.head = malloc(sizeof(struct event));
    event_queue.head->next = NULL;
    event_queue.head->fsm_input = FSM_INPUT_PAUSE_PLAY;
#else
    event_queue.head = NULL;
#endif
    event_queue.tail = &event_queue.head;
    timer_gpio_init(timer_isr, gpio_isr);

    while (1) {
        /* Process event queue */

        gpio_disable_irq();
        event = event_queue.head;
        event_queue.head = NULL;
        event_queue.tail = &event_queue.head;
        gpio_enable_irq();

        if (event) {
            do {
                transition_fsm(event->fsm_input);
                tmp = (void *)event->next;
                free(event);
                event = (struct event *)tmp;
            } while(event);
#if CONFIG_ENABLE_TIMER
            /* FIXME: timing bug if gpio event occurs just before this */
            /* Since it is disabled by gpio_isr() */
            timer_enable_irq();
#endif
        }

        /* Display next frame */

        mjpeg423_decode(&current_video);
#if !CONFIG_ENABLE_TIMER
        if (current_state == FSM_STATE_PLAYING)
            try_display_frame();
#endif

#if CONFIG_ENABLE_PROFILE
        if (current_state == FSM_STATE_END) {
            decoder_print_profile();
#if CONFIG_ENABLE_HW_IDCT
            hw_idct_print_profile();
#endif
            break;
        }
#endif /* CONFIG_ENABLE_PROFILE */
    }

    return 0;
}
