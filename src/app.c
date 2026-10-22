#include "app.h"
#include "ui.h"
#include "player.h"
#include "photo.h"
#include "video.h"
#include "music.h"
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

static Favorites favorites;
static int favorites_initialized;
static PlayMode modes[2]={MODE_ONCE,MODE_ONCE};
static unsigned random_seed;

static void ensure_favorites(void)
{
    char path[PATH_MAX];
    if(favorites_initialized)return;
    favorites_initialized=1;
    if(extras_state_file(path,sizeof(path),"favorites.dat")<0 || favorites_open(&favorites,path)<0)
        perror("favorites unavailable");
}
int app_is_favorite(MediaKind kind,const char *path)
{ ensure_favorites(); return favorites_has(&favorites,kind,path); }
int app_toggle_favorite(MediaKind kind,const char *path)
{ ensure_favorites(); return favorites_toggle(&favorites,kind,path); }

int app_scan(MediaKind kind,MediaList *list)
{
    const char *root=getenv("MEDIA_ROOT");
    const char *sub=kind==MEDIA_VIDEO ? "video" : kind==MEDIA_MUSIC ? "music" : "photo";
    char requested[PATH_MAX],absolute[PATH_MAX];
    *list=(MediaList){0};
    if(!root)root="/mnt/udisk";
    if(snprintf(requested,sizeof(requested),"%s/%s",root,sub)>=(int)sizeof(requested))return -1;
    if(!realpath(requested,absolute))return -1;
    return media_scan(absolute,kind,list);
}
int app_open_list(MediaKind kind,const char *initial,MediaList *list,size_t *index)
{
    struct stat st; size_t i;
    *index=0; *list=(MediaList){0};
    if(initial && (stat(initial,&st)<0 || !S_ISREG(st.st_mode)))return -1;
    if(app_scan(kind,list)<0 && !initial)return -1;
    if(!initial)return 0;
    for(i=0;i<list->count;i++)if(!strcmp(list->paths[i],initial)) { *index=i; return 0; }
    /* A saved favorite may belong to a previously configured media directory. */
    media_free(list); list->paths=calloc(1,sizeof(*list->paths));
    if(!list->paths)return -1;
    list->paths[0]=strdup(initial);
    if(!list->paths[0]) { media_free(list); return -1; }
    list->count=1; return 0;
}
typedef struct { int percent,volume,playing,full; char line[100]; } ControlView;
static void controls(Player *p,const char *status,ControlView *v)
{
    char line[100]; int full=!v->full;
    int percent=p->duration>0 ? (int)fmin(100,p->position*100/p->duration) : 0;
    int playing=p->pid>0 && !p->paused;
    unsigned current=(unsigned)p->position,total=(unsigned)p->duration;
    if(total)snprintf(line,sizeof(line),"%02u:%02u / %02u:%02u   %s",current/60,current%60,total/60,total%60,status);
    else snprintf(line,sizeof(line),"%02u:%02u / --:--   %s",current/60,current%60,status);
    if(full) {
        lcd_rect(0,352,800,128,UI_BG);
        ui_button(16,430,110,40,"上一首",0); ui_button(248,430,110,40,"停止",0);
        ui_button(364,430,110,40,"下一首",0); ui_button(480,430,72,40,"音量-",0);
        ui_button(558,430,72,40,"音量+",0);
    }
    if(full || percent!=v->percent)ui_bar(24,370,752,percent);
    if(full || strcmp(line,v->line)) {
        lcd_rect(24,394,752,22,UI_BG); lcd_text(24,398,line,2,UI_FG);
        strcpy(v->line,line);
    }
    if(full || playing!=v->playing)ui_button(132,430,110,40,playing ? "暂停" : "播放",playing);
    if(full || p->volume!=v->volume) {
        char volume[30]; ui_bar(646,446,138,p->volume);
        lcd_rect(646,422,138,18,UI_BG);
        snprintf(volume,sizeof(volume),"音量 %d",p->volume); lcd_text(646,428,volume,1,UI_FG);
    }
    v->percent=percent; v->playing=playing; v->volume=p->volume; v->full=1;
}
static void tools(MediaKind kind,const char *path,PlayMode mode,const char *notice)
{
    char label[40];
    lcd_rect(0,288,800,64,UI_BG);
    ui_button(16,300,116,40,path && app_is_favorite(kind,path) ? "已收藏" : "收藏",path && app_is_favorite(kind,path));
    snprintf(label,sizeof(label),"模式 %s",mode_name(mode)); ui_button(140,300,192,40,label,0);
    if(kind==MEDIA_VIDEO)ui_button(340,300,116,40,"截图",0);
    lcd_text(kind==MEDIA_VIDEO ? 468 : 348,316,notice,1,UI_FG);
}
static void take_screenshot(Player *p,char *notice,size_t size)
{
    Bmp image={0}; char path[PATH_MAX];
    if(p->pid<=0 || p->duration<=0) { snprintf(notice,size,"请先播放视频"); return; }
    if(lcd_capture(&image,0,64,800,224)<0 || snapshot_save(&image,path,sizeof(path))<0) {
        snprintf(notice,size,"截图保存失败"); perror("screenshot");
    } else { snprintf(notice,size,"截图已保存到相册"); printf("Screenshot: %s\n",path); }
    bmp_free(&image);
}
static void play_media_at(MediaKind kind,const char *initial)
{
    MediaList list={0}; Player p; Touch t; ControlView view={0}; size_t index=0;
    int video=kind==MEDIA_VIDEO,chrome_dirty=1,canvas_dirty=1;
    long long refreshed=0; const char *status="正在播放"; char notice[48]="";
    PlayMode *mode=&modes[video ? 0 : 1];
    player_init(&p);
    if(!random_seed)random_seed=(unsigned)time(NULL)^(unsigned)getpid();
    if(app_open_list(kind,initial,&list,&index)<0)status=initial ? "文件不存在" : "目录不存在";
    else if(!list.count)status="没有媒体文件";
    lcd_begin(); lcd_rect(0,0,800,480,UI_BG); lcd_end();
    if(list.count && player_start(&p,list.paths[index],video)<0) { status="启动失败"; p.failed=1; }
    while(app_running) {
        int got; player_update(&p);
        if(p.ended) {
            int autoplay=!p.failed && p.duration>0 && *mode!=MODE_ONCE;
            status=p.failed ? "播放失败" : "播放结束";
            p.ended=0; canvas_dirty=1;
            if(autoplay) {
                index=playback_next(*mode,index,list.count,0,&random_seed);
                if(player_start(&p,list.paths[index],video)<0)status="启动失败";
                chrome_dirty=1;
            }
        }
        if(p.pid>0)status=p.paused ? "已暂停" : "正在播放";
        lcd_begin();
        if(chrome_dirty) {
            ui_header(video ? "视频" : "音乐",list.count ? media_name(list.paths[index]) : status);
            tools(kind,list.count ? list.paths[index] : NULL,*mode,notice); chrome_dirty=0;
        }
        if(canvas_dirty) {
            if(!video || !p.pid) {
                lcd_rect(0,64,800,224,UI_BG);
                lcd_text(20,170,list.count && !video ? "音乐播放器" : status,2,UI_FG);
            }
            canvas_dirty=0;
        }
        controls(&p,status,&view); lcd_end();
        if(monotonic_ms()-refreshed>=500) {
            /* mplayer may clear fb0 once on startup. Restore only overwritten UI
               pixels; never clear/redraw the video canvas on a periodic timer. */
            if(!video)lcd_refresh(0,0,800,64);
            lcd_refresh(0,288,800,192); refreshed=monotonic_ms();
        }
        got=event_poll(&t,50);
        if(got<0) { app_running=0; break; }
        if(!got)continue;
        if(ui_back(&t))break;
        if(ui_hit(&t,140,300,192,40)) {
            *mode=(PlayMode)((*mode+1)%MODE_COUNT); chrome_dirty=1; continue;
        }
        if(!list.count)continue;
        if(ui_hit(&t,16,300,116,40)) {
            snprintf(notice,sizeof(notice),app_toggle_favorite(kind,list.paths[index])<0 ? "收藏保存失败" : "收藏已更新");
            chrome_dirty=1;
        } else if(video && ui_hit(&t,340,300,116,40)) {
            take_screenshot(&p,notice,sizeof(notice)); chrome_dirty=1;
        } else if(ui_hit(&t,24,354,752,40)) {
            if(p.pid>0 && p.duration>0 && player_seek(&p,(t.end_x-24)*100/752)<0) {
                strcpy(notice,"定位失败"); chrome_dirty=1;
            }
        } else if(ui_hit(&t,646,418,138,62))player_volume(&p,(t.end_x-646)*100/138);
        else if(ui_hit(&t,480,430,72,40))player_volume(&p,p.volume-10);
        else if(ui_hit(&t,558,430,72,40))player_volume(&p,p.volume+10);
        else if(ui_hit(&t,248,430,110,40)) {
            player_stop(&p); status="已停止"; canvas_dirty=1;
        } else if(ui_hit(&t,132,430,110,40)) {
            if(p.pid>0)player_pause(&p);
            else { if(player_start(&p,list.paths[index],video)<0)status="启动失败"; canvas_dirty=1; }
        } else {
            int dir=0;
            if(ui_hit(&t,16,430,110,40))dir=-1;
            else if(ui_hit(&t,364,430,110,40))dir=1;
            else if(t.y>=64 && t.y<288 && abs(t.end_x-t.x)>100)dir=t.end_x<t.x ? 1 : -1;
            if(dir) {
                index=playback_next(*mode,index,list.count,dir,&random_seed);
                if(player_start(&p,list.paths[index],video)<0)status="启动失败";
                notice[0]=0; chrome_dirty=canvas_dirty=1;
            }
        }
    }
    player_stop(&p); media_free(&list);
}
void play_media(MediaKind kind) { play_media_at(kind,NULL); }
static void show_favorites(void)
{
    size_t page=0; int dirty=1; Touch t; const char *notice="点击文件打开";
    ensure_favorites(); lcd_begin(); lcd_rect(0,0,800,480,UI_BG); lcd_end();
    while(app_running) {
        size_t i,pages=(favorites.count+4)/5; int got;
        if(!pages)pages=1;
        if(page>=pages)page=pages-1;
        if(dirty) {
            char title[64]; snprintf(title,sizeof(title),"收藏列表 %zu / %zu",page+1,pages);
            lcd_begin(); lcd_rect(0,0,800,480,UI_BG); ui_header(title,notice);
            if(!favorites.count)ui_message(favorites.file[0] ? "暂无收藏" : "收藏存储不可用");
            for(i=0;i<5 && page*5+i<favorites.count;i++) {
                Favorite *entry=&favorites.items[page*5+i]; char label[48];
                const char *kind=entry->kind==MEDIA_VIDEO ? "视频" : entry->kind==MEDIA_MUSIC ? "音乐" : "相册";
                snprintf(label,sizeof(label),"%s %.37s",kind,media_name(entry->path));
                ui_button(16,80+(int)i*62,610,50,label,0); ui_button(642,80+(int)i*62,140,50,"删除",0);
            }
            ui_button(16,420,144,44,"上一页",0); ui_button(176,420,144,44,"下一页",0); lcd_end(); dirty=0;
        }
        got=event_poll(&t,100);
        if(got<0) { app_running=0; break; }
        if(!got)continue;
        if(ui_back(&t))break;
        if(ui_hit(&t,16,420,144,44)) { if(page)page--; dirty=1; }
        else if(ui_hit(&t,176,420,144,44)) { if(page+1<pages)page++; dirty=1; }
        else for(i=0;i<5 && page*5+i<favorites.count;i++) {
            Favorite *entry=&favorites.items[page*5+i];
            if(ui_hit(&t,642,80+(int)i*62,140,50)) {
                notice=favorites_toggle(&favorites,entry->kind,entry->path)<0 ? "删除失败" : "已取消收藏";
                dirty=1; break;
            }
            if(ui_hit(&t,16,80+(int)i*62,610,50)) {
                char *path=strdup(entry->path); MediaKind kind=entry->kind;
                if(!path) { notice="内存不足"; dirty=1; break; }
                if(kind==MEDIA_PHOTO)play_photo_at(path); else play_media_at(kind,path);
                free(path); dirty=1; break;
            }
        }
    }
}
static void draw_clock(void)
{
    char text[48]; time_t now=time(NULL); struct tm tm;
    if(localtime_r(&now,&tm))strftime(text,sizeof(text),"%Y-%m-%d %H:%M:%S",&tm);
    else strcpy(text,"--:--:--");
    lcd_rect(40,80,340,42,UI_PANEL); lcd_rect(40,80,6,42,UI_ACCENT);
    lcd_text(62,93,text,2,UI_ACCENT);
}
static int draw_menu_asset(void)
{
    char path[PATH_MAX]; const char *root=getenv("ICON_ROOT");
    if(!root || !root[0])root="/mnt/udisk/icons";
    if(snprintf(path,sizeof(path),"%s/menu.bmp",root)>=(int)sizeof(path))return 0;
    return show_bmp(0,0,path)==0;
}
static void draw_menu(void)
{
    lcd_begin();
    if(draw_menu_asset()) {
        /* The static image contains the complete Chinese UI. Keep only the
           clock live so the menu still shows the board's current time. */
        lcd_rect(40,80,340,42,UI_PANEL); draw_clock();
    } else {
        lcd_rect(0,0,800,480,UI_BG);
        lcd_rect(0,0,800,68,UI_PANEL); lcd_rect(0,66,800,2,UI_ACCENT);
        lcd_text(32,12,"媒体中心",2,UI_FG);
        lcd_text(32,43,"GEC6818  /  MEDIA PLAYER",1,UI_MUTED);
        ui_button_dual(400,76,174,48,"收藏列表","FAVORITES",0);
        ui_button_dual(590,76,170,48,"亮度设置","DISPLAY",0);
        ui_card(40,140,340,120,"视频","VIDEO  /  LOCAL",UI_ACCENT_2);
        ui_card(420,140,340,120,"音乐","MUSIC  /  AUDIO",UI_ACCENT);
        ui_card(40,300,340,120,"相册","PHOTO  /  ALBUM",0xD49BFF);
        ui_card(420,300,340,120,"退出","EXIT  /  CLOSE",UI_WARNING);
        draw_clock();
    }
    lcd_end();
}
void app_menu(void)
{
    Touch t; time_t clock_tick=time(NULL); ensure_favorites(); draw_menu();
    while(app_running) {
        int got=event_poll(&t,100); time_t now=time(NULL);
        if(now!=clock_tick) { draw_clock(); clock_tick=now; }
        if(got<0) { app_running=0; break; }
        if(!got)continue;
        if(ui_hit(&t,40,140,340,120))play_video();
        else if(ui_hit(&t,420,140,340,120))play_music();
        else if(ui_hit(&t,40,300,340,120))play_photo();
        else if(ui_hit(&t,420,300,340,120))break;
        else if(ui_hit(&t,400,76,174,48))show_favorites();
        else if(ui_hit(&t,590,76,170,48))app_settings();
        else continue;
        draw_menu();
    }
    favorites_free(&favorites); favorites_initialized=0;
}
