#include "app.h"
#include "ui.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include <errno.h>
volatile sig_atomic_t app_running=1;
static int target_mode,inside,closed,mode_clicked,iterations,video_run,video_stage;
static char log_path[PATH_MAX],media_root[PATH_MAX];
void lcd_begin(void) {}
void lcd_end(void) {}
void lcd_refresh(int x,int y,int w,int h) { (void)x;(void)y;(void)w;(void)h; }
void lcd_pixel(int x,int y,uint32_t c) { (void)x;(void)y;(void)c; }
void lcd_pixel_direct(int x,int y,uint32_t c) { (void)x;(void)y;(void)c; }
uint32_t lcd_read_pixel(int x,int y) { (void)x;(void)y; return 0; }
void lcd_rect(int x,int y,int w,int h,uint32_t c) { (void)x;(void)y;(void)w;(void)h;(void)c; }
int show_bmp(int x,int y,const char *path) { (void)x;(void)y;(void)path; return -1; }
int lcd_capture(Bmp *b,int x,int y,int w,int h)
{
    size_t i; (void)x;(void)y;
    *b=(Bmp){w,h,NULL}; b->pixels=malloc((size_t)w*h*sizeof(*b->pixels));
    assert(b->pixels); for(i=0;i<(size_t)w*h;i++)b->pixels[i]=0x123456;
    return 0;
}
static int starts(char paths[8][PATH_MAX])
{
    FILE *f=fopen(log_path,"r"); int count=0;
    if(!f)return 0;
    while(count<8 && fgets(paths[count],PATH_MAX,f)) { paths[count][strcspn(paths[count],"\r\n")]=0; count++; }
    fclose(f); return count;
}
int event_poll(Touch *t,int ms)
{
    char paths[8][PATH_MAX]; int count;
    (void)ms; usleep(10000);
    if(!inside) { *t=(Touch){video_run ? 100 : 500,200,video_run ? 100 : 500,200}; inside=1; return 1; }
    if(closed) { *t=(Touch){500,350,500,350}; return 1; }
    if(video_run) {
        if(video_stage==0) {
            if(++iterations<12)return 0;
            *t=(Touch){400,320,400,320}; video_stage=1; return 1;
        }
        if(video_stage==1) {
            char dir[PATH_MAX+16],path[PATH_MAX+280]; DIR *d; struct dirent *e; int found=0;
            snprintf(dir,sizeof(dir),"%s/photo",media_root); d=opendir(dir); assert(d);
            while((e=readdir(d)))if(strstr(e->d_name,".bmp")) {
                Bmp b={0}; snprintf(path,sizeof(path),"%s/%s",dir,e->d_name);
                assert(bmp_load(path,&b)==0 && b.width==800 && b.height==224 && b.pixels[0]==0x123456);
                bmp_free(&b); found++;
            }
            closedir(d); assert(found==1);
            *t=(Touch){50,320,50,320}; video_stage=2; return 1;
        }
        if(video_stage==2) {
            char path[PATH_MAX+32]; Favorites f;
            snprintf(path,sizeof(path),"%s/.gec6818/favorites.dat",media_root);
            assert(favorites_open(&f,path)==0 && f.count==1 && f.items[0].kind==MEDIA_VIDEO);
            favorites_free(&f); *t=(Touch){300,450,300,450}; video_stage=3; return 1;
        }
        { int status; errno=0; assert(waitpid(-1,&status,WNOHANG)==-1 && errno==ECHILD); }
        *t=(Touch){710,25,710,25}; closed=1; return 1;
    }
    if(target_mode && !mode_clicked) { *t=(Touch){200,320,200,320}; mode_clicked=1; return 1; }
    count=starts(paths); iterations++;
    if((target_mode==0 && iterations>=45) || (target_mode && count>=3)) {
        assert(count==(target_mode ? 3 : 1));
        assert(strstr(paths[0],"01.mp3"));
        if(target_mode==MODE_SINGLE)assert(!strcmp(paths[0],paths[1]) && !strcmp(paths[1],paths[2]));
        if(target_mode==MODE_LIST || target_mode==MODE_RANDOM)
            assert(strstr(paths[1],"02.mp3") && !strcmp(paths[0],paths[2]));
        *t=(Touch){710,25,710,25}; closed=1; return 1;
    }
    assert(iterations<200); return 0;
}
int main(void)
{
    char root[]="/tmp/gec6818-flow-XXXXXX",dir[PATH_MAX],path[PATH_MAX],state[PATH_MAX]; int i;
    assert(mkdtemp(root)); setenv("MEDIA_ROOT",root,1); strcpy(media_root,root);
    snprintf(dir,sizeof(dir),"%s/music",root); assert(mkdir(dir,0700)==0);
    for(i=1;i<=2;i++) {
        FILE *f; snprintf(path,sizeof(path),"%s/music/%02d.mp3",root,i);
        f=fopen(path,"w"); assert(f); fclose(f);
    }
    snprintf(log_path,sizeof(log_path),"%s/player.log",root);
    setenv("TEST_PLAYER_LOG",log_path,1); setenv("TEST_AUTO_EXIT","1",1);
    setenv("MPLAYER_BIN","./build/tests/fake_player",1);
    for(target_mode=0;target_mode<MODE_COUNT;target_mode++) {
        FILE *f=fopen(log_path,"w"); assert(f); fclose(f);
        inside=closed=mode_clicked=iterations=0; app_running=1; app_menu();
        assert(closed);
    }
    unsetenv("TEST_AUTO_EXIT"); video_run=1; inside=closed=iterations=0;
    snprintf(path,sizeof(path),"%s/video",root); assert(mkdir(path,0700)==0);
    snprintf(path,sizeof(path),"%s/video/01.avi",root);
    { FILE *f=fopen(path,"w"); assert(f); fclose(f); }
    app_menu(); assert(closed);
    unlink(path); snprintf(path,sizeof(path),"%s/video",root); rmdir(path);
    snprintf(path,sizeof(path),"%s/photo",root);
    { DIR *d=opendir(path); struct dirent *e; assert(d);
      while((e=readdir(d)))if(strstr(e->d_name,".bmp")) { char shot[PATH_MAX+280]; snprintf(shot,sizeof(shot),"%s/%s",path,e->d_name); unlink(shot); }
      closedir(d); rmdir(path); }
    for(i=1;i<=2;i++) { snprintf(path,sizeof(path),"%s/music/%02d.mp3",root,i); unlink(path); }
    rmdir(dir); unlink(log_path);
    snprintf(state,sizeof(state),"%s/.gec6818/favorites.dat",root); unlink(state);
    snprintf(state,sizeof(state),"%s/.gec6818",root); rmdir(state); rmdir(root);
    puts("PASS EOF mode flow, screenshot button, video favorite, stop/reap and menu return"); return 0;
}
