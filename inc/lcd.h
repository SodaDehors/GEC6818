#ifndef LCD_H
#define LCD_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bmp.h"
#define LCD_WIDTH 800
#define LCD_HEIGHT 480
int init_lcd(void);
void uinit_lcd(void);
int show_bmp(int x, int y, const char *path);
void lcd_rect(int x, int y, int w, int h, uint32_t color);
void lcd_pixel(int x, int y, uint32_t color);
void lcd_pixel_direct(int x, int y, uint32_t color);
uint32_t lcd_read_pixel(int x, int y);
void lcd_text(int x, int y, const char *text, int scale, uint32_t color);
int lcd_text_width(const char *text, int scale);
int lcd_text_height(int scale);
int lcd_capture(Bmp *image,int x,int y,int width,int height);
/* Batch GUI draws; keep video-owned framebuffer rows untouched. */
void lcd_begin(void);
void lcd_end(void);
void lcd_refresh(int x,int y,int width,int height);
#endif
