#ifndef BMP_H
#define BMP_H
#include <stdint.h>
#include <stdio.h>
typedef struct { int width, height; uint32_t *pixels; } Bmp;
int bmp_load(const char *path, Bmp *image);
void bmp_free(Bmp *image);
int bmp_write(FILE *fp,const Bmp *image);
#endif
