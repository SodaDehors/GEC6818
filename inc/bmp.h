#ifndef BMP_H
#define BMP_H
#include <stdint.h>
typedef struct { int width, height; uint32_t *pixels; } Bmp;
int bmp_load(const char *path, Bmp *image);
void bmp_free(Bmp *image);
#endif
