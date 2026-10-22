#ifndef LCD_H
#define LCD_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define LCD_WIDTH 800
#define LCD_HEIGHT 480
int init_lcd(void);
void uinit_lcd(void);
int show_bmp(int x, int y, const char *path);
void lcd_rect(int x, int y, int w, int h, uint32_t color);
void lcd_pixel(int x, int y, uint32_t color);
void lcd_text(int x, int y, const char *text, int scale, uint32_t color);
#endif
