#include "extras.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

const char *mode_name(PlayMode mode)
{
    static const char *names[]={"单次播放","单曲循环","列表循环","随机播放"};
    return mode>=0 && mode<MODE_COUNT ? names[mode] : "单次播放";
}
size_t playback_next(PlayMode mode,size_t current,size_t count,int direction,unsigned *seed)
{
    unsigned n;
    if(!count)return 0;
    current%=count;
    if(!direction && (mode==MODE_ONCE || mode==MODE_SINGLE))return current;
    if(mode==MODE_RANDOM && count>1) {
        n=*seed ? *seed : 0x41C64E6Du;
        n^=n<<13; n^=n>>17; n^=n<<5; *seed=n;
        /* Do not repeat the current song when other entries exist. */
        return (current+1+n%(count-1))%count;
    }
    return direction<0 ? (current+count-1)%count : (current+1)%count;
}
const char *filter_name(ImageFilter filter)
{
    static const char *names[]={"原图","灰度","反色"};
    return filter>=0 && filter<FILTER_COUNT ? names[filter] : "原图";
}
uint32_t filter_pixel(uint32_t rgb,ImageFilter filter)
{
    if(filter==FILTER_GRAY) {
        unsigned gray=(77*((rgb>>16)&255)+150*((rgb>>8)&255)+29*(rgb&255))>>8;
        return gray*0x010101u;
    }
    return filter==FILTER_INVERT ? (rgb^0xFFFFFFu)&0xFFFFFFu : rgb&0xFFFFFFu;
}
static uint32_t read_u32(const unsigned char *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static void write_u32(unsigned char *p,uint32_t n)
{ int i; for(i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i)); }
void favorites_free(Favorites *f)
{
    size_t i; for(i=0;i<f->count;i++)free(f->items[i].path);
    free(f->items); memset(f,0,sizeof(*f));
}
int favorites_open(Favorites *f,const char *file)
{
    FILE *fp; unsigned char head[8],record[5]; size_t n;
    memset(f,0,sizeof(*f));
    if(!file || strlen(file)>=sizeof(f->file)-16) { errno=ENAMETOOLONG; return -1; }
    strcpy(f->file,file); fp=fopen(file,"rb");
    if(!fp) { if(errno==ENOENT)return 0; goto fail; }
    if(fread(head,1,8,fp)!=8 || memcmp(head,"GECFAV1\n",8))goto corrupt;
    while((n=fread(record,1,5,fp))!=0) {
        uint32_t length; char *path; Favorite *grown;
        if(n!=5 || record[0]>MEDIA_PHOTO || f->count>=1024)goto corrupt;
        length=read_u32(record+1);
        if(!length || length>=PATH_MAX)goto corrupt;
        path=malloc(length+1);
        if(!path)goto close_fail;
        if(fread(path,1,length,fp)!=length || memchr(path,0,length)) { free(path); goto corrupt; }
        path[length]=0;
        grown=realloc(f->items,(f->count+1)*sizeof(*grown));
        if(!grown) { free(path); goto close_fail; }
        f->items=grown; f->items[f->count++]=(Favorite){(MediaKind)record[0],path};
    }
    if(ferror(fp))goto close_fail;
    fclose(fp); return 0;
corrupt:
    errno=EINVAL;
close_fail:
    { int saved=errno; fclose(fp); errno=saved; }
fail:
    favorites_free(f); return -1;
}
int favorites_has(const Favorites *f,MediaKind kind,const char *path)
{
    size_t i;
    for(i=0;i<f->count;i++)if(f->items[i].kind==kind && !strcmp(f->items[i].path,path))return 1;
    return 0;
}
static int save_favorites(const Favorites *f)
{
    char temp[PATH_MAX]; FILE *fp; int fd,result=-1,saved; size_t i;
    if(!f->file[0]) { errno=EACCES; return -1; }
    if(snprintf(temp,sizeof(temp),"%s.XXXXXX",f->file)>=(int)sizeof(temp))return -1;
    fd=mkstemp(temp);
    if(fd<0)return -1;
    fp=fdopen(fd,"wb");
    if(!fp) { saved=errno; close(fd); unlink(temp); errno=saved; return -1; }
    if(fwrite("GECFAV1\n",1,8,fp)!=8)goto done;
    for(i=0;i<f->count;i++) {
        unsigned char record[5]; size_t length=strlen(f->items[i].path);
        record[0]=(unsigned char)f->items[i].kind; write_u32(record+1,(uint32_t)length);
        if(fwrite(record,1,5,fp)!=5 || fwrite(f->items[i].path,1,length,fp)!=length)goto done;
    }
    if(fflush(fp)<0 || fsync(fileno(fp))<0)goto done;
    result=0;
done:
    saved=errno;
    if(fclose(fp)<0) { result=-1; saved=errno; }
    if(!result && rename(temp,f->file)<0) { result=-1; saved=errno; }
    if(result)unlink(temp);
    errno=saved; return result;
}
int favorites_toggle(Favorites *f,MediaKind kind,const char *path)
{
    Favorites candidate; size_t i,found=f->count; char *added=NULL;
    if(!path || !path[0] || strlen(path)>=PATH_MAX || kind<MEDIA_VIDEO || kind>MEDIA_PHOTO) { errno=EINVAL; return -1; }
    for(i=0;i<f->count;i++)if(f->items[i].kind==kind && !strcmp(f->items[i].path,path)) { found=i; break; }
    if(found==f->count && f->count>=1024) { errno=ENOSPC; return -1; }
    candidate=*f;
    candidate.count=found<f->count ? f->count-1 : f->count+1;
    candidate.items=malloc((candidate.count ? candidate.count : 1)*sizeof(*candidate.items));
    if(!candidate.items)return -1;
    if(found<f->count) {
        size_t j=0;
        for(i=0;i<f->count;i++)if(i!=found)candidate.items[j++]=f->items[i];
    } else {
        if(f->count)memcpy(candidate.items,f->items,f->count*sizeof(*f->items));
        added=strdup(path);
        if(!added) { free(candidate.items); return -1; }
        candidate.items[f->count]=(Favorite){kind,added};
    }
    if(save_favorites(&candidate)<0) { free(added); free(candidate.items); return -1; }
    if(found<f->count)free(f->items[found].path);
    free(f->items); *f=candidate; return 0;
}
static int ensure_directory(const char *path)
{
    struct stat st;
    if(mkdir(path,0755)<0 && errno!=EEXIST)return -1;
    if(stat(path,&st)<0)return -1;
    if(!S_ISDIR(st.st_mode)) { errno=ENOTDIR; return -1; }
    return 0;
}
int extras_state_file(char *out,size_t size,const char *name)
{
    char dir[PATH_MAX]; const char *override=getenv("GEC_DATA_DIR"),*root=getenv("MEDIA_ROOT");
    if(!root)root="/mnt/udisk";
    if(snprintf(dir,sizeof(dir),override ? "%s" : "%s/.gec6818",override ? override : root)>=(int)sizeof(dir))return -1;
    if(ensure_directory(dir)<0)return -1;
    if(snprintf(out,size,"%s/%s",dir,name)>=(int)size) { errno=ENAMETOOLONG; return -1; }
    return 0;
}
int snapshot_save(const Bmp *image,char *out,size_t size)
{
    const char *dir=getenv("SCREENSHOT_DIR"),*root=getenv("MEDIA_ROOT");
    char folder[PATH_MAX],stamp[40]; struct timespec ts; struct tm tm;
    FILE *fp; int fd,attempt,result,saved;
    if(!root)root="/mnt/udisk";
    if(snprintf(folder,sizeof(folder),dir ? "%s" : "%s/photo",dir ? dir : root)>=(int)sizeof(folder))return -1;
    if(ensure_directory(folder)<0)return -1;
    clock_gettime(CLOCK_REALTIME,&ts); localtime_r(&ts.tv_sec,&tm);
    strftime(stamp,sizeof(stamp),"%Y%m%d_%H%M%S",&tm);
    for(attempt=0;attempt<100;attempt++) {
        if(snprintf(out,size,"%s/shot_%s_%03ld_%02d.bmp",folder,stamp,ts.tv_nsec/1000000,attempt)>=(int)size) { errno=ENAMETOOLONG; return -1; }
        fd=open(out,O_WRONLY|O_CREAT|O_EXCL,0644);
        if(fd>=0)break;
        if(errno!=EEXIST)return -1;
    }
    if(fd<0)return -1;
    fp=fdopen(fd,"wb");
    if(!fp) { saved=errno; close(fd); unlink(out); errno=saved; return -1; }
    result=bmp_write(fp,image);
    if(!result && fflush(fp)<0)result=-1;
    if(!result && fsync(fileno(fp))<0)result=-1;
    saved=errno;
    if(fclose(fp)<0) { result=-1; saved=errno; }
    if(result)unlink(out);
    errno=saved; return result;
}
