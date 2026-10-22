/* Software framebuffer for layout checks; not a substitute for board tests. */
#include "app.h"
#include "ui.h"
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
volatile sig_atomic_t app_running=1;
static uint32_t frame[480][800];
void lcd_pixel(int x,int y,uint32_t rgb)
{ if(x>=0 && x<800 && y>=0 && y<480)frame[y][x]=rgb; }
void lcd_rect(int x,int y,int w,int h,uint32_t color)
{ int xx,yy; for(yy=y;yy<y+h;yy++)for(xx=x;xx<x+w;xx++)lcd_pixel(xx,yy,color); }
static void capture(const char *name)
{
    char path[256]; int x,y; FILE *fp;
    snprintf(path,sizeof(path),"build/preview/%s.ppm",name); fp=fopen(path,"wb"); assert(fp);
    fputs("P6\n800 480\n255\n",fp);
    for(y=0;y<480;y++)for(x=0;x<800;x++) {
        unsigned char rgb[3]={(unsigned char)(frame[y][x]>>16),(unsigned char)(frame[y][x]>>8),(unsigned char)frame[y][x]};
        fwrite(rgb,1,3,fp);
    }
    fclose(fp);
}
int event_poll(Touch *t,int ms)
{
    static int index; (void)ms;
    static const struct { int x,y; const char *screen; } script[]={
        {100,200,"menu"},{710,25,"video-empty"},
        {500,200,NULL},{710,25,"music-empty"},
        {100,350,NULL},{430,440,"photo"},
        {300,440,"photo-zoom"},{710,25,"photo-auto"},
        {500,350,NULL}};
    assert(index<(int)(sizeof(script)/sizeof(script[0])));
    if(script[index].screen)capture(script[index].screen);
    *t=(Touch){script[index].x,script[index].y,script[index].x,script[index].y};
    index++; return 1;
}
int main(void)
{
    unsigned char h[54]={0}; FILE *f; int x,y;
    mkdir("build/preview",0700); mkdir("build/preview/media",0700);
    mkdir("build/preview/media/photo",0700); mkdir("build/preview/media/music",0700);
    mkdir("build/preview/media/video",0700);
    h[0]='B'; h[1]='M'; h[10]=54; h[14]=40; h[18]=64; h[22]=64; h[26]=1; h[28]=24;
    f=fopen("build/preview/media/photo/01.bmp","wb"); assert(f); fwrite(h,1,54,f);
    for(y=0;y<64;y++)for(x=0;x<64;x++) {
        unsigned char p[3]={(unsigned char)(x*4),(unsigned char)(y*4),(unsigned char)200};
        fwrite(p,1,3,f);
    }
    fclose(f); setenv("MEDIA_ROOT","build/preview/media",1);
    app_menu(); puts("PASS UI navigation/empty states/photo controls; previews in build/preview"); return 0;
}
