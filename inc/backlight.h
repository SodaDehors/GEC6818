#ifndef BACKLIGHT_H
#define BACKLIGHT_H
#include <limits.h>
typedef struct {
    char directory[PATH_MAX];
    char brightness_path[PATH_MAX];
    char device_path[PATH_MAX];
    int maximum,raw,restore,available;
} Backlight;
/* BACKLIGHT_DIR overrides autodetection; BACKLIGHT_BRIGHTNESS can name the
   exact sysfs file on board images that do not expose /sys/class/backlight. */
int backlight_init(Backlight *b);
int backlight_percent(const Backlight *b);
int backlight_set(Backlight *b,int percent);
int backlight_blank(Backlight *b);
int backlight_wake(Backlight *b);
#endif
