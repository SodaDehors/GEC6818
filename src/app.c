#include "app.h"
#include "ui.h"
#include "player.h"
#include "photo.h"
#include "video.h"
#include "music.h"
#include <string.h>
#include <limits.h>
#include <math.h>

/* Keep the same directories in both the program and the handoff instructions. */
int app_scan(MediaKind kind,MediaList *list)
{
    const char *root=getenv("MEDIA_ROOT");
    const char *sub=kind==MEDIA_VIDEO ? "video" : kind==MEDIA_MUSIC ? "music" : "photo";
    char requested[PATH_MAX],absolute[PATH_MAX];
    if(!root)root="/mnt/udisk";
    if(snprintf(requested,sizeof(requested),"%s/%s",root,sub)>=(int)sizeof(requested))return -1;
    if(!realpath(requested,absolute))return -1;
    return media_scan(absolute,kind,list);
}
static void controls(Player *p,const char *status)
{
    char line[80];
    int percent=p->duration>0 ? (int)fmin(100,p->position*100/p->duration) : 0;
    unsigned current=(unsigned)p->position,total=(unsigned)p->duration;
    lcd_rect(0,352,800,128,UI_BG);
    ui_bar(24,370,752,percent);
    if(total)snprintf(line,sizeof(line),"%02u:%02u / %02u:%02u   %s",current/60,current%60,total/60,total%60,status);
    else snprintf(line,sizeof(line),"%02u:%02u / --:--   %s",current/60,current%60,status);
    lcd_text(24,398,line,2,UI_FG);
    ui_button(16,430,110,40,"PREV",0);
    ui_button(132,430,110,40,p->pid>0 && !p->paused ? "PAUSE" : "PLAY",p->pid>0);
    ui_button(248,430,110,40,"STOP",0); ui_button(364,430,110,40,"NEXT",0);
    ui_button(480,430,72,40,"VOL-",0); ui_button(558,430,72,40,"VOL+",0);
    ui_bar(646,446,138,p->volume);
    snprintf(line,sizeof(line),"VOL %d",p->volume); lcd_text(646,428,line,1,UI_FG);
}
void play_media(MediaKind kind)
{
    MediaList list={0}; Player p; Touch t;
    size_t index=0;
    int video=kind==MEDIA_VIDEO, dirty=1;
    long long redraw=0;
    const char *status="PLAYING";
    player_init(&p);
    if(app_scan(kind,&list)<0)status="DIRECTORY MISSING";
    else if(!list.count)status="NO MEDIA FILES";
    lcd_rect(0,0,800,480,UI_BG);
    if(list.count && player_start(&p,list.paths[index],video)<0) { status="START FAILED"; p.failed=1; }
    while(app_running) {
        int got;
        player_update(&p);
        if(p.ended) { status=p.failed ? "PLAYBACK FAILED" : "FINISHED"; dirty=1; p.ended=0; }
        if(p.pid>0)status=p.paused ? "PAUSED" : "PLAYING";
        if(dirty || monotonic_ms()-redraw>=500) {
            ui_header(video ? "VIDEO" : "MUSIC",list.count ? media_name(list.paths[index]) : status);
            if(!video)ui_message(list.count ? "MUSIC PLAYER" : status);
            else if(!p.pid)ui_message(status);
            controls(&p,status); dirty=0; redraw=monotonic_ms();
        }
        got=event_poll(&t,50);
        if(got<0) { app_running=0; break; }
        if(!got)continue;
        if(ui_back(&t))break;
        if(!list.count)continue;
        if(ui_hit(&t,24,354,752,40)) {
            if(p.pid>0 && p.duration>0)player_seek(&p,(t.end_x-24)*100/752);
        } else if(ui_hit(&t,646,418,138,62))player_volume(&p,(t.end_x-646)*100/138);
        else if(ui_hit(&t,480,430,72,40))player_volume(&p,p.volume-10);
        else if(ui_hit(&t,558,430,72,40))player_volume(&p,p.volume+10);
        else if(ui_hit(&t,248,430,110,40)) {
            player_stop(&p); status="STOPPED"; ui_message("STOPPED");
        } else if(ui_hit(&t,132,430,110,40)) {
            if(p.pid>0)player_pause(&p);
            else if(player_start(&p,list.paths[index],video)<0)status="START FAILED";
        } else {
            int dir=0;
            if(ui_hit(&t,16,430,110,40))dir=-1;
            else if(ui_hit(&t,364,430,110,40))dir=1;
            else if(t.y>=64 && t.y<352 && abs(t.end_x-t.x)>100)dir=t.end_x<t.x ? 1 : -1;
            if(dir) {
                index=dir>0 ? (index+1)%list.count : (index+list.count-1)%list.count;
                if(player_start(&p,list.paths[index],video)<0)status="START FAILED";
            }
        }
        dirty=1;
    }
    player_stop(&p); media_free(&list);
}
static void draw_menu(void)
{
    lcd_rect(0,0,800,480,UI_BG);
    lcd_text(40,35,"GEC6818 MEDIA CENTER",3,UI_FG);
    lcd_text(40,82,"TOUCH A MODULE TO START",2,UI_ACCENT);
    ui_button(40,140,340,120,"VIDEO",0);
    ui_button(420,140,340,120,"MUSIC",0);
    ui_button(40,300,340,120,"PHOTO",0);
    ui_button(420,300,340,120,"EXIT",0);
}
void app_menu(void)
{
    Touch t; draw_menu();
    while(app_running) {
        int got=event_poll(&t,100);
        if(got<0) { app_running=0; break; }
        if(!got)continue;
        if(ui_hit(&t,40,140,340,120))play_video();
        else if(ui_hit(&t,420,140,340,120))play_music();
        else if(ui_hit(&t,40,300,340,120))play_photo();
        else if(ui_hit(&t,420,300,340,120))break;
        draw_menu();
    }
}
