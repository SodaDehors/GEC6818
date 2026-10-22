#include "ui.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
static int text_width(const char *text,int scale)
{
    return lcd_text_width(text,scale);
}
static int centered_x(int x,int w,const char *text,int scale)
{
    int tw=text_width(text,scale),left=x+(w-tw)/2;
    if(left<x+8)left=x+8;
    if(left+tw>x+w-8)left=x+w-8-tw;
    return left<x+8 ? x+8 : left;
}
static int fit_scale(const char *text,int width,int preferred)
{
    int scale;
    for(scale=preferred;scale>1;scale--)if(text_width(text,scale)<=width)return scale;
    return 1;
}
static const char *english_title(const char *title)
{
    if(!strncmp(title,"视频",6) || !strncmp(title,"VIDEO",5))return "VIDEO";
    if(!strncmp(title,"音乐",6) || !strncmp(title,"MUSIC",5))return "MUSIC";
    if(!strncmp(title,"相册",6) || !strncmp(title,"PHOTO",5))return "PHOTO";
    if(!strncmp(title,"收藏列表",12) || !strncmp(title,"FAVORITES",9))return "FAVORITES";
    if(!strncmp(title,"显示设置",12) || !strncmp(title,"DISPLAY",7))return "DISPLAY";
    return "GEC6818  /  MEDIA PLAYER";
}
static void copy_fitting(char *out,size_t size,const char *text,int width)
{
    size_t len;
    if(!size)return;
    snprintf(out,size,"%s",text ? text : "");
    len=strlen(out);
    while(len && text_width(out,1)>width) {
        out[--len]=0;
        while(len && (((unsigned char)out[len]&0xC0)==0x80))out[--len]=0;
    }
}
static const char *icon_file_name(const char *title)
{
    if(!strcmp(title,"视频") || !strcmp(title,"VIDEO"))return "video.bmp";
    if(!strcmp(title,"音乐") || !strcmp(title,"MUSIC"))return "music.bmp";
    if(!strcmp(title,"相册") || !strcmp(title,"PHOTO"))return "photo.bmp";
    if(!strcmp(title,"收藏列表") || !strcmp(title,"FAVORITES"))return "favorites.bmp";
    if(!strcmp(title,"亮度设置") || !strcmp(title,"DISPLAY"))return "display.bmp";
    if(!strcmp(title,"退出") || !strcmp(title,"EXIT"))return "exit.bmp";
    return NULL;
}
static int draw_icon_file(int x,int y,const char *title)
{
    char path[PATH_MAX]; const char *root=getenv("ICON_ROOT");
    const char *name=icon_file_name(title);
    if(!name)return 0;
    if(!root || !root[0])root="/mnt/udisk/icons";
    snprintf(path,sizeof(path),"%s/%s",root,name);
    return show_bmp(x,y,path)==0;
}
static void card_icon(int x,int y,int w,int h,const char *title,uint32_t accent)
{
    int ix=x+20,iy=y+25;
    if(draw_icon_file(ix,iy,title)) { (void)w; (void)h; return; }
    lcd_rect(ix,iy,58,58,accent);
    if(!strcmp(title,"视频") || !strcmp(title,"VIDEO")) {
        lcd_rect(ix+10,iy+13,38,30,UI_PANEL);
        lcd_rect(ix+15,iy+18,28,20,accent);
        lcd_rect(ix+26,iy+21,4,14,UI_PANEL);
        lcd_rect(ix+30,iy+25,5,6,UI_PANEL);
    } else if(!strcmp(title,"音乐") || !strcmp(title,"MUSIC")) {
        lcd_rect(ix+34,iy+10,5,31,UI_PANEL);
        lcd_rect(ix+38,iy+10,14,5,UI_PANEL);
        lcd_rect(ix+12,iy+36,18,10,UI_PANEL);
        lcd_rect(ix+34,iy+34,18,10,UI_PANEL);
    } else if(!strcmp(title,"相册") || !strcmp(title,"PHOTO")) {
        lcd_rect(ix+9,iy+12,40,34,UI_PANEL);
        lcd_rect(ix+15,iy+18,28,22,accent);
        lcd_rect(ix+18,iy+31,9,7,UI_PANEL);
        lcd_rect(ix+28,iy+25,7,13,UI_PANEL);
        lcd_rect(ix+36,iy+30,7,8,UI_PANEL);
    } else {
        lcd_rect(ix+14,iy+11,6,36,UI_PANEL);
        lcd_rect(ix+20,iy+11,18,5,UI_PANEL);
        lcd_rect(ix+20,iy+42,18,5,UI_PANEL);
        lcd_rect(ix+35,iy+25,17,6,UI_PANEL);
        lcd_rect(ix+45,iy+20,7,16,UI_PANEL);
    }
    (void)w; (void)h;
}
void ui_button(int x,int y,int w,int h,const char *label,int active)
{
    int scale=fit_scale(label,w-16,2),left=centered_x(x,w,label,scale);
    lcd_rect(x,y,w,h,UI_BORDER);
    lcd_rect(x+2,y+2,w-4,h-4,active ? UI_ACCENT : UI_PANEL_2);
    lcd_rect(x+2,y+2,w-4,4,active ? UI_ACCENT_2 : UI_BORDER);
    lcd_text(left,y+(h-lcd_text_height(scale))/2,label,scale,active ? UI_BG : UI_FG);
}
void ui_button_dual(int x,int y,int w,int h,const char *primary,const char *secondary,int active)
{
    int scale=fit_scale(primary,w-16,2),left=centered_x(x,w,primary,scale);
    int sub=fit_scale(secondary,w-16,1),sub_left=centered_x(x,w,secondary,sub);
    int gap=1,total;
    if(lcd_text_height(scale)+gap+lcd_text_height(sub)>h-8) {
        scale=fit_scale(primary,w-16,1);
        left=centered_x(x,w,primary,scale);
    }
    total=lcd_text_height(scale)+gap+lcd_text_height(sub);
    lcd_rect(x,y,w,h,UI_BORDER);
    lcd_rect(x+2,y+2,w-4,h-4,active ? UI_ACCENT : UI_PANEL_2);
    lcd_rect(x+2,y+2,w-4,4,active ? UI_ACCENT_2 : UI_BORDER);
    lcd_text(left,y+(h-total)/2,primary,scale,active ? UI_BG : UI_FG);
    if(secondary && secondary[0])lcd_text(sub_left,y+(h-total)/2+lcd_text_height(scale)+gap,secondary,sub,active ? UI_BG : UI_MUTED);
}
void ui_card(int x,int y,int w,int h,const char *title,const char *subtitle,uint32_t accent)
{
    lcd_rect(x,y,w,h,UI_BORDER);
    lcd_rect(x+2,y+2,w-4,h-4,UI_PANEL);
    lcd_rect(x+2,y+2,6,h-4,accent);
    card_icon(x,y,w,h,title,accent);
    {
        int scale=fit_scale(title,w-112,2);
        lcd_text(x+96,y+20,title,scale,UI_FG);
    }
    lcd_text(x+96,y+h-28,subtitle,1,UI_MUTED);
}
void ui_header(const char *title,const char *name)
{
    char display[80]; int scale=fit_scale(title,620,2);
    copy_fitting(display,sizeof(display),name ? name : "",430);
    lcd_rect(0,0,800,64,UI_PANEL); lcd_rect(0,62,800,2,UI_ACCENT);
    lcd_text(20,7,title,scale,UI_ACCENT);
    lcd_text(20,39,english_title(title),1,UI_MUTED);
    lcd_text(220,23,display,1,UI_FG);
    ui_button_dual(680,8,104,46,"返回","BACK",0);
}
void ui_bar(int x,int y,int w,int percent)
{
    if(percent<0)percent=0;
    if(percent>100)percent=100;
    lcd_rect(x,y,w,16,UI_BORDER);
    lcd_rect(x+2,y+2,w-4,12,UI_PANEL_2);
    if(percent>0)lcd_rect(x+2,y+2,(w-4)*percent/100,12,UI_ACCENT);
}
int ui_hit(const Touch *t,int x,int y,int w,int h)
{
    return t->x>=x && t->x<x+w && t->y>=y && t->y<y+h;
}
int ui_back(const Touch *t) { return ui_hit(t,680,0,120,64); }
void ui_message(const char *message)
{
    int scale=fit_scale(message,752,2);
    lcd_rect(0,64,800,288,UI_BG);
    lcd_text(centered_x(24,752,message,scale),180,message,scale,UI_FG);
}
