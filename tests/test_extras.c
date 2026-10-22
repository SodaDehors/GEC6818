#include "extras.h"
#include "backlight.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
static void text_file(const char *dir,const char *name,const char *text)
{
    char path[PATH_MAX]; FILE *fp;
    snprintf(path,sizeof(path),"%s/%s",dir,name); fp=fopen(path,"w"); assert(fp);
    assert(fputs(text,fp)>=0); assert(fclose(fp)==0);
}
int main(void)
{
    char root[]="/tmp/gec6818-extras-XXXXXX",state[PATH_MAX],path[PATH_MAX],shot[PATH_MAX],shot2[PATH_MAX];
    char saved[PATH_MAX+32]; Favorites f,g; Backlight light; unsigned seed=123; size_t i,current=0;
    uint32_t colors[]={0xFF0000,0x00FF00,0x0000FF,0xFFFFFF,0x000000,0x123456};
    Bmp image={3,2,colors},loaded={0};
    assert(mkdtemp(root)); setenv("MEDIA_ROOT",root,1);
    assert(extras_state_file(state,sizeof(state),"favorites.dat")==0);
    assert(favorites_open(&f,state)==0 && f.count==0);
    assert(favorites_toggle(&f,MEDIA_MUSIC,"/a song 'quote'\n.mp3")==0);
    assert(favorites_toggle(&f,MEDIA_PHOTO,"/照片.bmp")==0 && f.count==2);
    assert(favorites_has(&f,MEDIA_MUSIC,"/a song 'quote'\n.mp3"));
    assert(favorites_open(&g,state)==0 && g.count==2);
    assert(!strcmp(g.items[1].path,"/照片.bmp")); favorites_free(&g);
    /* Failed saves must not report success or mutate the list. */
    strcpy(saved,f.file); snprintf(f.file,sizeof(f.file),"%s/missing/favorites",root);
    assert(favorites_toggle(&f,MEDIA_VIDEO,"/new.avi")<0 && f.count==2);
    assert(favorites_toggle(&f,MEDIA_PHOTO,"/照片.bmp")<0 && f.count==2);
    strcpy(f.file,saved);
    assert(favorites_toggle(&f,MEDIA_PHOTO,"/照片.bmp")==0 && f.count==1);
    assert(favorites_toggle(&f,MEDIA_MUSIC,"/a song 'quote'\n.mp3")==0 && !f.count);
    favorites_free(&f); assert(favorites_open(&f,state)==0 && !f.count); favorites_free(&f);
    { FILE *fp=fopen(state,"wb"); assert(fp); fputs("bad",fp); fclose(fp); }
    assert(favorites_open(&f,state)<0 && !f.file[0]);
    assert(playback_next(MODE_ONCE,2,3,0,&seed)==2);
    assert(playback_next(MODE_SINGLE,2,3,0,&seed)==2);
    assert(playback_next(MODE_SINGLE,2,3,1,&seed)==0);
    assert(playback_next(MODE_LIST,2,3,0,&seed)==0);
    assert(playback_next(MODE_LIST,0,3,-1,&seed)==2);
    for(i=0;i<100;i++) { size_t next=playback_next(MODE_RANDOM,current,5,0,&seed); assert(next<5 && next!=current); current=next; }
    assert(playback_next(MODE_RANDOM,0,1,0,&seed)==0);
    assert(playback_next(MODE_LIST,0,0,0,&seed)==0);
    assert(filter_pixel(0x123456,FILTER_ORIGINAL)==0x123456);
    assert(filter_pixel(0x123456,FILTER_INVERT)==0xEDCBA9);
    { uint32_t gray=filter_pixel(0xFF0000,FILTER_GRAY); assert((gray&255)==((gray>>8)&255) && (gray&255)==((gray>>16)&255)); }
    assert(snapshot_save(&image,shot,sizeof(shot))==0);
    assert(snapshot_save(&image,shot2,sizeof(shot2))==0 && strcmp(shot,shot2));
    assert(bmp_load(shot,&loaded)==0 && loaded.width==3 && loaded.height==2);
    assert(!memcmp(loaded.pixels,colors,sizeof(colors))); bmp_free(&loaded);
    snprintf(path,sizeof(path),"%s/light",root); assert(mkdir(path,0700)==0);
    setenv("BACKLIGHT_DIR",path,1); text_file(path,"max_brightness","255\n"); text_file(path,"brightness","128\n");
    assert(backlight_init(&light)==0 && backlight_percent(&light)==50);
    assert(backlight_set(&light,150)==0 && light.raw==255);
    assert(backlight_set(&light,-1)==0 && light.raw==0);
    assert(backlight_set(&light,60)==0 && light.raw==153);
    assert(backlight_blank(&light)==0 && light.raw==0);
    assert(backlight_wake(&light)==0 && light.raw==153);
    text_file(path,"max_brightness","0\n"); assert(backlight_init(&light)<0 && !light.available);
    text_file(path,"max_brightness","1\n"); text_file(path,"brightness","1\n");
    assert(backlight_init(&light)==0 && backlight_set(&light,10)==0 && light.raw==1);
    text_file(path,"max_brightness","not a number\n"); assert(backlight_init(&light)<0);
    snprintf(saved,sizeof(saved),"%s/backlight-1wire",root); text_file(root,"backlight-1wire","127\n");
    unsetenv("BACKLIGHT_DIR"); setenv("BACKLIGHT_DEVICE",saved,1);
    assert(backlight_init(&light)==0 && light.maximum==127 && backlight_percent(&light)==100);
    assert(backlight_set(&light,50)==0 && light.raw==64);
    unsetenv("BACKLIGHT_DEVICE"); unlink(saved);
    text_file(root,"not-a-dir","keep"); snprintf(saved,sizeof(saved),"%s/not-a-dir",root);
    setenv("SCREENSHOT_DIR",saved,1); assert(snapshot_save(&image,saved,sizeof(saved))<0);
    unlink(shot); unlink(shot2); unlink(state);
    snprintf(saved,sizeof(saved),"%s/brightness",path); unlink(saved);
    snprintf(saved,sizeof(saved),"%s/max_brightness",path); unlink(saved); rmdir(path);
    snprintf(saved,sizeof(saved),"%s/.gec6818",root); rmdir(saved);
    snprintf(saved,sizeof(saved),"%s/photo",root); rmdir(saved);
    snprintf(saved,sizeof(saved),"%s/not-a-dir",root); unlink(saved); rmdir(root);
    puts("PASS favorites persistence/rollback/corruption, modes, filters, snapshots, backlight"); return 0;
}
