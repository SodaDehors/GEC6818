#ifndef APP_H
#define APP_H
#include <signal.h>
#include "media.h"
#include "extras.h"
extern volatile sig_atomic_t app_running;
int app_scan(MediaKind kind,MediaList *list);
int app_open_list(MediaKind kind,const char *initial,MediaList *list,size_t *index);
int app_is_favorite(MediaKind kind,const char *path);
int app_toggle_favorite(MediaKind kind,const char *path);
void play_media(MediaKind kind);
void play_photo_at(const char *path);
void app_settings(void);
void app_menu(void);
#endif
