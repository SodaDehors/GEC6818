#ifndef MEDIA_H
#define MEDIA_H
#include <stddef.h>
typedef enum { MEDIA_VIDEO, MEDIA_MUSIC, MEDIA_PHOTO } MediaKind;
typedef struct { char **paths; size_t count; } MediaList;
int media_scan(const char *directory,MediaKind kind,MediaList *list);
void media_free(MediaList *list);
const char *media_name(const char *path);
#endif
