#include "photo.h"
#include "app.h"
#include "bmp.h"
#include "ui.h"
#include "player.h"
#include <math.h>
static void draw_photo(const Bmp *image,int zoom,int *pan_x,int *pan_y,ImageFilter filter)
{
    int x,y,w,h,left,top,max_x,max_y; double fit,scale;
    lcd_rect(0,64,800,288,0x000000);
    if(!image->pixels) { lcd_text(20,180,"图片读取失败",2,UI_FG); return; }
    fit=fmin(800.0/image->width,288.0/image->height);
    scale=fit*(zoom==0 ? 1.0 : zoom==1 ? 1.5 : 2.0);
    w=(int)(image->width*scale); h=(int)(image->height*scale);
    if(w<1)w=1;
    if(h<1)h=1;
    max_x=w>800 ? (w-800+1)/2 : 0; max_y=h>288 ? (h-288+1)/2 : 0;
    if(*pan_x>max_x)*pan_x=max_x;
    if(*pan_x < -max_x)*pan_x=-max_x;
    if(*pan_y>max_y)*pan_y=max_y;
    if(*pan_y < -max_y)*pan_y=-max_y;
    left=(800-w)/2+*pan_x; top=64+(288-h)/2+*pan_y;
    for(y=64;y<352;y++)for(x=0;x<800;x++) {
        int sx,sy;
        if(x<left || x>=left+w || y<top || y>=top+h)continue;
        sx=(int)((int64_t)(x-left)*image->width/w); sy=(int)((int64_t)(y-top)*image->height/h);
        lcd_pixel(x,y,filter_pixel(image->pixels[(size_t)sy*image->width+sx],filter));
    }
}
void play_photo_at(const char *initial)
{
    MediaList list={0}; Bmp image={0}; Touch t; size_t index=0;
    int automatic=0,zoom=0,pan_x=0,pan_y=0,load=1,dirty=1;
    ImageFilter filter=FILTER_ORIGINAL; char notice[48]="";
    long long next=monotonic_ms()+3000; const char *empty="没有图片";
    if(app_open_list(MEDIA_PHOTO,initial,&list,&index)<0)empty=initial ? "文件不存在" : "相册目录不存在";
    lcd_begin(); lcd_rect(0,0,800,480,UI_BG); lcd_end();
    while(app_running) {
        int got,dir=0;
        if(automatic && list.count && monotonic_ms()>=next) {
            index=(index+1)%list.count; load=dirty=1; next=monotonic_ms()+3000;
        }
        if(load && list.count) {
            bmp_free(&image); bmp_load(list.paths[index],&image); zoom=pan_x=pan_y=0; load=0;
        }
        if(dirty) {
            char title[64],label[40];
            snprintf(title,sizeof(title),"相册 %zu/%zu",list.count ? index+1 : 0,list.count);
            lcd_begin(); ui_header(title,list.count ? media_name(list.paths[index]) : empty);
            if(list.count)draw_photo(&image,zoom,&pan_x,&pan_y,filter); else {
                lcd_rect(0,64,800,288,UI_BG); lcd_text(20,180,empty,2,UI_FG);
            }
            lcd_rect(0,352,800,128,UI_BG);
            snprintf(label,sizeof(label),"滤镜 %s",filter_name(filter)); ui_button(16,360,224,40,label,filter!=FILTER_ORIGINAL);
            ui_button(248,360,128,40,list.count && app_is_favorite(MEDIA_PHOTO,list.paths[index]) ? "已收藏" : "收藏",list.count && app_is_favorite(MEDIA_PHOTO,list.paths[index]));
            lcd_text(390,376,notice,1,UI_FG);
            ui_button(16,420,104,44,"上一张",0); ui_button(128,420,104,44,"下一张",0);
            ui_button(240,420,144,44,automatic ? "轮播 开启" : "轮播 关闭",automatic);
            ui_button(392,420,104,44,"放大",zoom>0); ui_button(504,420,104,44,"缩小",0);
            ui_button(616,420,80,44,"适应",0);
            lcd_text(708,434,zoom==0 ? "1倍" : zoom==1 ? "1.5倍" : "2倍",1,UI_ACCENT);
            lcd_end(); dirty=0;
        }
        got=event_poll(&t,50);
        if(got<0) { app_running=0; break; }
        if(!got)continue;
        if(ui_back(&t))break;
        if(!list.count)continue;
        if(ui_hit(&t,16,420,104,44))dir=-1;
        else if(ui_hit(&t,128,420,104,44))dir=1;
        else if(ui_hit(&t,240,420,144,44)) { automatic=!automatic; next=monotonic_ms()+3000; dirty=1; }
        else if(ui_hit(&t,392,420,104,44)) { if(zoom<2)zoom++; automatic=0; dirty=1; }
        else if(ui_hit(&t,504,420,104,44)) { if(zoom>0)zoom--; dirty=1; }
        else if(ui_hit(&t,616,420,80,44)) { zoom=pan_x=pan_y=0; dirty=1; }
        else if(ui_hit(&t,16,360,224,40)) { filter=(ImageFilter)((filter+1)%FILTER_COUNT); dirty=1; }
        else if(ui_hit(&t,248,360,128,40)) {
            snprintf(notice,sizeof(notice),app_toggle_favorite(MEDIA_PHOTO,list.paths[index])<0 ? "收藏保存失败" : "收藏已更新"); dirty=1;
        } else if(t.y>=64 && t.y<352) {
            if(zoom && (t.end_x!=t.x || t.end_y!=t.y)) { pan_x+=t.end_x-t.x; pan_y+=t.end_y-t.y; automatic=0; dirty=1; }
            else if(!zoom && abs(t.end_x-t.x)>100)dir=t.end_x<t.x ? 1 : -1;
        }
        if(dir) {
            index=dir>0 ? (index+1)%list.count : (index+list.count-1)%list.count;
            load=dirty=1; notice[0]=0; next=monotonic_ms()+3000;
        }
    }
    bmp_free(&image); media_free(&list);
}
void play_photo(void) { play_photo_at(NULL); }
