#ifndef UI_H
#define UI_H
#include "event.h"
#include <stdint.h>
#define UI_BG       0xF4F7FB
#define UI_PANEL    0xFFFFFF
#define UI_PANEL_2  0xEAF0F7
#define UI_BORDER   0xD2DDEA
#define UI_FG       0x1D2D49
#define UI_MUTED    0x607088
#define UI_ACCENT   0x12A8A6
#define UI_ACCENT_2 0x3E66D5
#define UI_WARNING  0xE7655B
void ui_button(int x,int y,int w,int h,const char *label,int active);
void ui_button_dual(int x,int y,int w,int h,const char *primary,const char *secondary,int active);
void ui_card(int x,int y,int w,int h,const char *title,const char *subtitle,uint32_t accent);
void ui_header(const char *title,const char *name);
void ui_bar(int x,int y,int w,int percent);
int ui_hit(const Touch *t,int x,int y,int w,int h);
int ui_back(const Touch *t);
void ui_message(const char *message);
#endif
