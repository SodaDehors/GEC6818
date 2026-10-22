#ifndef UI_H
#define UI_H
#include "event.h"
#define UI_BG 0x101C2C
#define UI_FG 0xEFF5FF
#define UI_ACCENT 0x46CFAB
void ui_button(int x,int y,int w,int h,const char *label,int active);
void ui_header(const char *title,const char *name);
void ui_bar(int x,int y,int w,int percent);
int ui_hit(const Touch *t,int x,int y,int w,int h);
int ui_back(const Touch *t);
void ui_message(const char *message);
#endif
