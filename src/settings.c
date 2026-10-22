#include "app.h"
#include "ui.h"
#include "backlight.h"
void app_settings(void)
{
    Backlight light; Touch t; int available=backlight_init(&light)==0;
    int dirty=1,blanked=available && light.raw==0;
    const char *notice=available ? "拖动滑块调节亮度" : "未找到背光控制接口";
    lcd_rect(0,0,800,480,UI_BG);
    while(app_running) {
        int got;
        if(dirty) {
            char text[80]; int percent=backlight_percent(&light);
            lcd_begin(); ui_header("显示设置",notice);
            lcd_rect(0,64,800,416,UI_BG);
            if(available)snprintf(text,sizeof(text),"屏幕亮度 %d / 100",percent);
            else snprintf(text,sizeof(text),"亮度功能不可用");
            lcd_text(80,144,text,2,UI_FG);
            ui_bar(80,220,640,available ? percent : 0);
            ui_button(80,280,160,52,"变暗",0); ui_button(560,280,160,52,"变亮",0);
            ui_button(280,280,240,52,"息屏",blanked);
            lcd_text(80,370,"触摸屏幕唤醒",2,UI_ACCENT);
            lcd_text(80,406,"范围：10 到 100",1,UI_FG);
            lcd_end(); dirty=0;
        }
        got=event_poll(&t,100);
        if(got<0) { app_running=0; break; }
        if(!got)continue;
        if(blanked) {
            if(backlight_wake(&light)<0)notice="唤醒失败，请检查权限";
            else { blanked=0; notice="屏幕已恢复"; }
            dirty=1; continue; /* Wake tap must not also activate a button. */
        }
        if(ui_back(&t))break;
        if(!available)continue;
        if(ui_hit(&t,280,280,240,52)) {
            if(backlight_blank(&light)<0)notice="息屏失败";
            else { blanked=1; notice="触摸屏幕唤醒"; }
        } else {
            int value=backlight_percent(&light),change=1;
            if(ui_hit(&t,80,200,640,60))value=(t.end_x-80)*100/640;
            else if(ui_hit(&t,80,280,160,52))value-=10;
            else if(ui_hit(&t,560,280,160,52))value+=10;
            else change=0;
            if(change) {
                if(value<10)value=10;
                if(value>100)value=100;
                notice=backlight_set(&light,value)<0 ? "写入失败，请检查权限" : "亮度已更新";
            }
        }
        dirty=1;
    }
    if(blanked && backlight_wake(&light)<0)perror("restore backlight");
}
