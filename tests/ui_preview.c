/* Software framebuffer for layout checks; not a substitute for board tests. */
#include "app.h"
#include "ui.h"
#include "backlight.h"
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <string.h>
volatile sig_atomic_t app_running=1;
static uint32_t frame[480][800];
static unsigned long draws;
void lcd_begin(void) {}
void lcd_end(void) {}
void lcd_refresh(int x,int y,int w,int h) { (void)x; (void)y; (void)w; (void)h; }
int lcd_capture(Bmp *image,int x,int y,int w,int h)
{
    int xx,yy; *image=(Bmp){0};
    if(x<0 || y<0 || w<=0 || h<=0 || x>800-w || y>480-h)return -1;
    image->pixels=malloc((size_t)w*h*sizeof(*image->pixels)); if(!image->pixels)return -1;
    image->width=w; image->height=h;
    for(yy=0;yy<h;yy++)for(xx=0;xx<w;xx++)image->pixels[(size_t)yy*w+xx]=frame[y+yy][x+xx];
    return 0;
}
void lcd_pixel(int x,int y,uint32_t rgb)
{ if(x>=0 && x<800 && y>=0 && y<480) { frame[y][x]=rgb; draws++; } }
void lcd_pixel_direct(int x,int y,uint32_t rgb)
{ lcd_pixel(x,y,rgb); }
uint32_t lcd_read_pixel(int x,int y)
{ return x>=0 && x<800 && y>=0 && y<480 ? frame[y][x] : 0; }
void lcd_rect(int x,int y,int w,int h,uint32_t color)
{ int xx,yy; for(yy=y;yy<y+h;yy++)for(xx=x;xx<x+w;xx++)lcd_pixel(xx,yy,color); }
int show_bmp(int x,int y,const char *path)
{
    Bmp image={0}; int xx,yy;
    if(bmp_load(path,&image)<0)return -1;
    for(yy=0;yy<image.height;yy++)for(xx=0;xx<image.width;xx++)
        lcd_pixel(x+xx,y+yy,image.pixels[(size_t)yy*image.width+xx]);
    bmp_free(&image); return 0;
}
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
    static int index,idled; static unsigned long before_idle; (void)ms;
    static const struct { int x,y; const char *screen; } script[]={
        {100,200,"menu"},{710,25,"video-empty"},
        {500,200,NULL},{710,25,"music-empty"},
        {100,350,NULL},{100,380,"photo"},
        {300,380,"photo-gray"},{710,25,"photo-favorite"},
        {450,90,NULL},{100,100,"favorites"},
        {430,440,"favorite-photo"},{300,440,"photo-zoom"},
        {710,25,"photo-auto"},{710,100,NULL},{710,25,"favorites-empty"},
        {650,90,NULL},{100,300,"display"},
        {400,300,"display-darker"},{710,25,"display-blank"},
        {710,25,"display-awake"},{500,350,NULL}};
    assert(index<(int)(sizeof(script)/sizeof(script[0])));
    if(index==3 && !idled) { before_idle=draws; idled=1; usleep(650000); return 0; }
    if(index==3)assert(draws==before_idle); /* No half-second clear/repaint flicker. */
    if(index==6) {
        uint32_t rgb=frame[180][400];
        assert((rgb&255)==((rgb>>8)&255) && (rgb&255)==((rgb>>16)&255));
    }
    if(script[index].screen)capture(script[index].screen);
    *t=(Touch){script[index].x,script[index].y,script[index].x,script[index].y};
    index++; return 1;
}
int main(void)
{
    unsigned char h[54]={0}; FILE *f; int x,y;
    char state[]="/tmp/gec6818-ui-XXXXXX",path[PATH_MAX];
    assert(mkdtemp(state)); setenv("GEC_DATA_DIR",state,1);
    mkdir("build/preview",0700); mkdir("build/preview/media",0700);
    mkdir("build/preview/icons",0700); setenv("ICON_ROOT","build/preview/icons",1);
    mkdir("build/preview/media/photo",0700); mkdir("build/preview/media/music",0700);
    mkdir("build/preview/media/video",0700);
    h[0]='B'; h[1]='M'; h[10]=54; h[14]=40; h[18]=64; h[22]=64; h[26]=1; h[28]=24;
    f=fopen("build/preview/media/photo/01.bmp","wb"); assert(f); fwrite(h,1,54,f);
    for(y=0;y<64;y++)for(x=0;x<64;x++) {
        unsigned char p[3]={(unsigned char)(x*4),(unsigned char)(y*4),(unsigned char)200};
        fwrite(p,1,3,f);
    }
    fclose(f); setenv("MEDIA_ROOT","build/preview/media",1);
    mkdir("build/preview/backlight",0700); setenv("BACKLIGHT_DIR","build/preview/backlight",1);
    f=fopen("build/preview/backlight/max_brightness","w"); assert(f); fputs("255\n",f); fclose(f);
    f=fopen("build/preview/backlight/brightness","w"); assert(f); fputs("128\n",f); fclose(f);
    app_menu();
    snprintf(path,sizeof(path),"%s/favorites.dat",state);
    { Favorites check; assert(favorites_open(&check,path)==0 && !check.count); favorites_free(&check); }
    unlink(path); rmdir(state);
    { Backlight b; assert(backlight_init(&b)==0 && b.raw>0); }
    puts("PASS UI navigation/favorites/filter/brightness/wake and idle flicker regression"); return 0;
}
