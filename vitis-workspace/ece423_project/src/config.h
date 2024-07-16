#ifndef _CONFIG_H
#define _CONFIG_H

#define CONFIG_ENABLE_TIMER   (1)
/*
 * Don't forget to comment-out
 * DEBUG_JPEG in common/util.h
 */
#define CONFIG_ENABLE_PROFILE (0)
#define CONFIG_ENABLE_HW_IDCT (1)

#define CONFIG_GPIO_DEBOUNCE_MS (100)

#define CONFIG_PLAYER_SKIP_LEN (120)

#define CONFIG_VBUFF_WIDTH  (1280)
#define CONFIG_VBUFF_HEIGHT (720)
#define CONFIG_VBUFF_LENGTH (2)

#define CONFIG_VIDEO_DIRECTORY ("3:/")
#define CONFIG_VIDEO_EXTENSION (".MPG")
#define CONFIG_VIDEO_IGNORE    ("_V1_17~1.MPG")
#define CONFIG_VIDEO_PROFILE   ("V1_1730.MPG")

#endif /* _CONFIG_H */
