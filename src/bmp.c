#include "bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
static uint32_t le32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
           (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static unsigned le16(const unsigned char *p) { return p[0] | (unsigned)p[1]<<8; }
void bmp_free(Bmp *image)
{
    if(image) { free(image->pixels); image->pixels=NULL; image->width=image->height=0; }
}
int bmp_load(const char *path,Bmp *image)
{
    unsigned char h[54], *row=NULL;
    FILE *fp;
    int32_t w, signed_h;
    int height,y,x,bpp,result=-1;
    size_t stride;
    uint32_t offset,dib;
    if(!image || !path) return -1;
    *image=(Bmp){0,0,NULL};
    fp=fopen(path,"rb");
    if(!fp) return -1;
    if(fread(h,1,sizeof(h),fp)!=sizeof(h) || h[0]!='B' || h[1]!='M') goto done;
    w=(int32_t)le32(h+18); signed_h=(int32_t)le32(h+22);
    offset=le32(h+10); dib=le32(h+14); bpp=(int)le16(h+28);
    /* Bound allocations before multiplying; reject compressed BMPs. */
    if(dib<40 || dib>1048576 || offset<14+dib || le16(h+26)!=1 ||
       le32(h+30)!=0 || (bpp!=24 && bpp!=32) || w<=0 || w>8192 ||
       signed_h==0 || signed_h==INT32_MIN || signed_h>8192 || signed_h < -8192) goto done;
    height=signed_h<0 ? -signed_h : signed_h;
    if((size_t)w*height>16000000u) goto done;
    stride=((size_t)w*(bpp/8)+3)&~(size_t)3;
    image->pixels=malloc((size_t)w*height*sizeof(*image->pixels)); row=malloc(stride);
    if(!image->pixels || !row || offset>0x7fffffffu || fseek(fp,(long)offset,SEEK_SET)) goto done;
    for(y=0;y<height;y++) {
        int dest=signed_h>0 ? height-1-y : y;
        if(fread(row,1,stride,fp)!=stride) goto done;
        for(x=0;x<w;x++) {
            unsigned char *p=row+(size_t)x*(bpp/8);
            image->pixels[(size_t)dest*w+x]=(uint32_t)p[2]<<16 | (uint32_t)p[1]<<8 | p[0];
        }
    }
    image->width=w; image->height=height; result=0;
done:
    free(row); fclose(fp);
    if(result) bmp_free(image);
    return result;
}
