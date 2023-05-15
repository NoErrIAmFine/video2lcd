#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdio.h>

#define FB_DEVICE_NAME "/dev/fb0"
#define DEFAULT_DIR     "/"

#define COLOR_BACKGROUND 0xE7DBB5   /* 泛黄的纸 */
#define COLOR_FOREGROUND 0x524438   /* 褐色的字体 */

#if 0
#define DBG_PRINTF(...)
#else
#define DBG_PRINTF printf
#endif


#endif