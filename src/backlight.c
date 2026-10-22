#include "backlight.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
static int read_value(const char *dir,const char *name,int *value)
{
    char path[PATH_MAX],line[80],*end; FILE *fp; long n;
    if(snprintf(path,sizeof(path),"%s/%s",dir,name)>=(int)sizeof(path))return -1;
    fp=fopen(path,"r"); if(!fp)return -1;
    if(!fgets(line,sizeof(line),fp)) { fclose(fp); return -1; }
    fclose(fp); errno=0; n=strtol(line,&end,10);
    if(end==line || errno || n<0 || n>INT_MAX)return -1;
    while(isspace((unsigned char)*end))end++;
    if(*end)return -1;
    *value=(int)n; return 0;
}
static int read_path(const char *path,int *value)
{
    char line[80],*end; FILE *fp; long n;
    fp=fopen(path,"r"); if(!fp)return -1;
    if(!fgets(line,sizeof(line),fp)) { fclose(fp); return -1; }
    fclose(fp); errno=0; n=strtol(line,&end,10);
    if(end==line || errno || n<0 || n>INT_MAX)return -1;
    while(isspace((unsigned char)*end))end++;
    if(*end)return -1;
    *value=(int)n; return 0;
}
static int path_join(char *out,size_t size,const char *dir,const char *name)
{ return snprintf(out,size,"%s/%s",dir,name)>=(int)size ? -1 : 0; }
static int probe(Backlight *b,const char *dir)
{
    char brightness[PATH_MAX]; int max,raw;
    if(strlen(dir)>=sizeof(b->directory) || path_join(brightness,sizeof(brightness),dir,"brightness")<0 ||
       read_value(dir,"max_brightness",&max)<0 || max<=0 || read_path(brightness,&raw)<0 || raw>max)return -1;
    strcpy(b->directory,dir); strcpy(b->brightness_path,brightness); b->maximum=max; b->raw=raw;
    b->restore=raw ? raw : max; b->available=1; return 0;
}
static int probe_exact(Backlight *b,const char *brightness,const char *max_path)
{
    char directory[PATH_MAX],*slash; int max,raw;
    if(strlen(brightness)>=sizeof(b->brightness_path) || read_path(brightness,&raw)<0 ||
       read_path(max_path,&max)<0 || max<=0 || raw>max)return -1;
    snprintf(directory,sizeof(directory),"%s",brightness);
    slash=strrchr(directory,'/');
    if(!slash || slash==directory)return -1;
    *slash=0; strcpy(b->directory,directory); strcpy(b->brightness_path,brightness);
    b->maximum=max; b->raw=raw; b->restore=raw ? raw : max; b->available=1; return 0;
}
static int scan_root(Backlight *b,const char *root)
{
    DIR *dir=opendir(root); struct dirent *e;
    if(!dir)return -1;
    while((e=readdir(dir))) {
        char path[PATH_MAX];
        if(e->d_name[0]=='.')continue;
        if(path_join(path,sizeof(path),root,e->d_name)<0)continue;
        if(!probe(b,path)) { closedir(dir); return 0; }
    }
    closedir(dir); return -1;
}
static int scan_nested_root(Backlight *b,const char *root)
{
    DIR *dir=opendir(root); struct dirent *e;
    if(!dir)return -1;
    while((e=readdir(dir))) {
        char path[PATH_MAX];
        if(e->d_name[0]=='.')continue;
        if(path_join(path,sizeof(path),root,e->d_name)<0)continue;
        if(!scan_root(b,path)) { closedir(dir); return 0; }
    }
    closedir(dir); return -1;
}
static int probe_device(Backlight *b,const char *path)
{
    if(!path || !path[0] || strlen(path)>=sizeof(b->device_path) || access(path,W_OK)<0)return -1;
    strcpy(b->device_path,path);
    b->maximum=127; b->raw=127; b->restore=127; b->available=1;
    return 0;
}
int backlight_init(Backlight *b)
{
    const char *override=getenv("BACKLIGHT_DIR");
    const char *exact=getenv("BACKLIGHT_BRIGHTNESS");
    const char *max_path=getenv("BACKLIGHT_MAX_BRIGHTNESS");
    const char *roots[]={"/sys/class/backlight","/sys/class/leds","/sys/devices/platform"};
    size_t i;
    memset(b,0,sizeof(*b));
    if(override)return probe(b,override);
    if(exact) {
        char fallback[PATH_MAX];
        if(!max_path) {
            snprintf(fallback,sizeof(fallback),"%s",exact);
            { char *slash=strrchr(fallback,'/'); if(slash) { strcpy(slash+1,"max_brightness"); max_path=fallback; } }
        }
        if(max_path && probe_exact(b,exact,max_path)==0)return 0;
    }
    for(i=0;i<sizeof(roots)/sizeof(roots[0]);i++) {
        if(!scan_root(b,roots[i]) || !scan_nested_root(b,roots[i]))return 0;
    }
    /* GEC6818/S5P6818 images often expose the LCD backlight through the
       one-wire device instead of the Linux backlight class.  Its documented
       range is 0..127 and it is write-only, so keep the last requested value
       in memory while the application is running. */
    if(probe_device(b,getenv("BACKLIGHT_DEVICE"))==0)return 0;
    if(probe_device(b,"/dev/backlight-1wire")==0)return 0;
    return -1;
}
int backlight_percent(const Backlight *b)
{ return b->available ? (int)(((long long)b->raw*100+b->maximum/2)/b->maximum) : -1; }
static int write_raw(Backlight *b,int raw)
{
    char path[PATH_MAX],line[32]; int fd,length; ssize_t n;
    if(!b->available || raw<0 || raw>b->maximum) { errno=EINVAL; return -1; }
    if(b->device_path[0])snprintf(path,sizeof(path),"%s",b->device_path);
    else if(b->brightness_path[0])snprintf(path,sizeof(path),"%s",b->brightness_path);
    else if(snprintf(path,sizeof(path),"%s/brightness",b->directory)>=(int)sizeof(path))return -1;
    fd=open(path,O_WRONLY); if(fd<0)return -1;
    length=snprintf(line,sizeof(line),"%d\n",raw);
    do { n=write(fd,line,(size_t)length); } while(n<0 && errno==EINTR);
    if(n!=length) { int saved=errno; close(fd); errno=saved; return -1; }
    if(close(fd)<0)return -1;
    b->raw=raw; return 0;
}
int backlight_set(Backlight *b,int percent)
{
    int raw;
    if(!b->available) { errno=ENODEV; return -1; }
    if(percent<0)percent=0;
    if(percent>100)percent=100;
    raw=(int)(((long long)b->maximum*percent+50)/100);
    if(percent>0 && !raw)raw=1;
    if(write_raw(b,raw)<0)return -1;
    if(raw)b->restore=raw;
    return 0;
}
int backlight_blank(Backlight *b)
{ if(b->raw>0)b->restore=b->raw; return write_raw(b,0); }
int backlight_wake(Backlight *b)
{ return write_raw(b,b->restore>0 ? b->restore : b->maximum); }
