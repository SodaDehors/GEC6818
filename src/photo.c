#include "photo.h"
#include "app.h"
#include "bmp.h"
#include "ui.h"
#include "player.h"
#include <math.h>
static void draw_photo(const Bmp *image,int zoom,int *pan_x,int *pan_y)
{
    int x,y,w,h,left,top,max_x,max_y;
    double fit,scale;
    lcd_rect(0,64,800,336,0x000000);
    if(!image->pixels) { ui_message("CANNOT READ BMP IMAGE"); return; }
    fit=fmin(800.0/image->width,336.0/image->height);
    scale=fit*(zoom==0 ? 1.0 : zoom==1 ? 1.5 : 2.0);
    w=(int)(image->width*scale); h=(int)(image->height*scale);
    if(w<1)w=1;
    if(h<1)h=1;
    max_x=w>800 ? (w-800+1)/2 : 0; max_y=h>336 ? (h-336+1)/2 : 0;
    if(*pan_x>max_x)*pan_x=max_x;
    if(*pan_x < -max_x)*pan_x=-max_x;
    if(*pan_y>max_y)*pan_y=max_y;
    if(*pan_y < -max_y)*pan_y=-max_y;
    left=(800-w)/2+*pan_x; top=64+(336-h)/2+*pan_y;
    for(y=64;y<400;y++)for(x=0;x<800;x++) {
        int sx,sy;
        if(x<left || x>=left+w || y<top || y>=top+h)continue;
        sx=(int)((int64_t)(x-left)*image->width/w);
        sy=(int)((int64_t)(y-top)*image->height/h);
        lcd_pixel(x,y,image->pixels[(size_t)sy*image->width+sx]);
    }
}
void play_photo(void)
{
    MediaList list={0}; Bmp image={0}; Touch t;
    size_t index=0; int automatic=0,zoom=0,pan_x=0,pan_y=0,load=1,dirty=1;
    long long next=monotonic_ms()+3000;
    const char *empty="NO BMP IMAGES";
    if(app_scan(MEDIA_PHOTO,&list)<0)empty="PHOTO DIRECTORY MISSING";
    lcd_rect(0,0,800,480,UI_BG);
    while(app_running) {
        int got,dir=0;
        if(automatic && list.count && monotonic_ms()>=next) {
            index=(index+1)%list.count; load=dirty=1; next=monotonic_ms()+3000;
        }
        if(load && list.count) {
            bmp_free(&image); bmp_load(list.paths[index],&image);
            zoom=pan_x=pan_y=0; load=0;
        }
        if(dirty) {
            char title[64];
            snprintf(title,sizeof(title),"PHOTO %zu/%zu",list.count ? index+1 : 0,list.count);
            ui_header(title,list.count ? media_name(list.paths[index]) : empty);
            if(list.count)draw_photo(&image,zoom,&pan_x,&pan_y); else ui_message(empty);
            lcd_rect(0,400,800,80,UI_BG);
            ui_button(16,420,104,44,"PREV",0); ui_button(128,420,104,44,"NEXT",0);
            ui_button(240,420,144,44,automatic ? "AUTO ON" : "AUTO OFF",automatic);
            ui_button(392,420,104,44,"ZOOM+",zoom>0); ui_button(504,420,104,44,"ZOOM-",0);
            ui_button(616,420,80,44,"FIT",0);
            lcd_text(708,434,zoom==0 ? "1X" : zoom==1 ? "1.5X" : "2X",2,UI_ACCENT);
            dirty=0;
        }
        got=event_poll(&t,50);
        if(got<0) { app_running=0; break; }
        if(!got)continue;
        if(ui_back(&t))break;
        if(!list.count)continue;
        if(ui_hit(&t,16,420,104,44))dir=-1;
        else if(ui_hit(&t,128,420,104,44))dir=1;
        else if(ui_hit(&t,240,420,144,44)) { automatic=!automatic; next=monotonic_ms()+3000; }
        else if(ui_hit(&t,392,420,104,44)) { if(zoom<2)zoom++; automatic=0; }
        else if(ui_hit(&t,504,420,104,44)) { if(zoom>0)zoom--; }
        else if(ui_hit(&t,616,420,80,44))zoom=pan_x=pan_y=0;
        else if(t.y>=64 && t.y<400) {
            if(zoom) { pan_x+=t.end_x-t.x; pan_y+=t.end_y-t.y; automatic=0; }
            else if(abs(t.end_x-t.x)>100)dir=t.end_x<t.x ? 1 : -1;
        }
        if(dir) {
            index=dir>0 ? (index+1)%list.count : (index+list.count-1)%list.count;
            load=1; next=monotonic_ms()+3000;
        }
        dirty=1;
    }
    bmp_free(&image); media_free(&list);
}
