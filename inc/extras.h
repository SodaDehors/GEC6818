#ifndef EXTRAS_H
#define EXTRAS_H
#include "media.h"
#include "bmp.h"
#include <limits.h>
#include <stdio.h>
typedef enum { MODE_ONCE, MODE_SINGLE, MODE_LIST, MODE_RANDOM, MODE_COUNT } PlayMode;
typedef enum { FILTER_ORIGINAL, FILTER_GRAY, FILTER_INVERT, FILTER_COUNT } ImageFilter;
typedef struct { MediaKind kind; char *path; } Favorite;
typedef struct { Favorite *items; size_t count; char file[PATH_MAX]; } Favorites;
const char *mode_name(PlayMode mode);
/* direction=0: automatic EOF transition; +/-1: user selection. */
size_t playback_next(PlayMode mode,size_t current,size_t count,int direction,unsigned *seed);
uint32_t filter_pixel(uint32_t rgb,ImageFilter filter);
const char *filter_name(ImageFilter filter);
int favorites_open(Favorites *f,const char *file);
void favorites_free(Favorites *f);
int favorites_has(const Favorites *f,MediaKind kind,const char *path);
/* Save atomically; on failure leave the in-memory list unchanged. */
int favorites_toggle(Favorites *f,MediaKind kind,const char *path);
/* Parent media directory must already exist. GEC_DATA_DIR overrides the state directory. */
int extras_state_file(char *out,size_t size,const char *name);
int snapshot_save(const Bmp *image,char *out,size_t size);
#endif
