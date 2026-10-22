#ifndef APP_H
#define APP_H
#include <signal.h>
#include "media.h"
extern volatile sig_atomic_t app_running;
int app_scan(MediaKind kind,MediaList *list);
void play_media(MediaKind kind);
void app_menu(void);
#endif
