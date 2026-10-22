#ifndef EVENT_H
#define EVENT_H
#include "lcd.h"
typedef struct { int x, y, end_x, end_y; } Touch;
int init_event(void);
void close_event(void);
/* 1 = completed touch, 0 = timeout, -1 = device/error. */
int event_poll(Touch *touch, int timeout_ms);
int get_pos(int *x, int *y, int *end_x, int *end_y);
#endif
