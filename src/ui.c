#include "ui.h"
#include <string.h>
void ui_button(int x,int y,int w,int h,const char *label,int active)
{
    int left=x+(w-(int)strlen(label)*12)/2;
    lcd_rect(x,y,w,h,active ? UI_ACCENT : 0x29415A);
    lcd_text(left,y+(h-14)/2,label,2,active ? UI_BG : UI_FG);
}
void ui_header(const char *title,const char *name)
{
    char display[51];
    snprintf(display,sizeof(display),"%.50s",name);
    lcd_rect(0,0,800,64,UI_BG); lcd_text(20,10,title,2,UI_ACCENT);
    lcd_text(20,40,display,2,UI_FG);
    ui_button(680,10,104,40,"BACK",0);
}
void ui_bar(int x,int y,int w,int percent)
{
    if(percent<0)percent=0;
    if(percent>100)percent=100;
    lcd_rect(x,y,w,16,0x29415A); lcd_rect(x,y,w*percent/100,16,UI_ACCENT);
}
int ui_hit(const Touch *t,int x,int y,int w,int h)
{
    return t->x>=x && t->x<x+w && t->y>=y && t->y<y+h;
}
int ui_back(const Touch *t) { return ui_hit(t,680,0,120,64); }
void ui_message(const char *message)
{
    lcd_rect(0,64,800,288,UI_BG); lcd_text(20,180,message,2,UI_FG);
}
