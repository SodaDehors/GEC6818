#include "media.h"
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static int accepts(const char *name,MediaKind kind)
{
    const char *ext=strrchr(name,'.');
    const char *const video[]={".avi",".mp4",".mkv",".mov",".mpg",".mpeg",NULL};
    const char *const music[]={".mp3",".wav",".ogg",".flac",".aac",".m4a",NULL};
    const char *const photo[]={".bmp",NULL};
    const char *const *types=kind==MEDIA_VIDEO ? video : kind==MEDIA_MUSIC ? music : photo;
    int i;
    if(!ext)return 0;
    for(i=0;types[i];i++)if(!strcasecmp(ext,types[i]))return 1;
    return 0;
}
const char *media_name(const char *path)
{
    const char *s=strrchr(path,'/'); return s ? s+1 : path;
}
static int compare(const void *a,const void *b)
{
    return strcmp(*(const char *const *)a,*(const char *const *)b);
}
void media_free(MediaList *list)
{
    size_t i;
    for(i=0;i<list->count;i++)free(list->paths[i]);
    free(list->paths); *list=(MediaList){NULL,0};
}
int media_scan(const char *directory,MediaKind kind,MediaList *list)
{
    DIR *dir; struct dirent *entry;
    *list=(MediaList){NULL,0};
    dir=opendir(directory);
    if(!dir)return -1;
    while((entry=readdir(dir))) {
        struct stat st; char *path; char **grown;
        size_t length;
        if(entry->d_name[0]=='.' || !accepts(entry->d_name,kind))continue;
        /* Cap memory usage for a small embedded system. */
        if(list->count>=1024) { fprintf(stderr,"Media list limited to 1024 entries\n"); break; }
        length=strlen(directory)+strlen(entry->d_name)+2;
        path=malloc(length);
        if(!path)goto fail;
        snprintf(path,length,"%s/%s",directory,entry->d_name);
        if(stat(path,&st)<0 || !S_ISREG(st.st_mode)) { free(path); continue; }
        grown=realloc(list->paths,(list->count+1)*sizeof(*grown));
        if(!grown) { free(path); goto fail; }
        list->paths=grown; list->paths[list->count++]=path;
    }
    closedir(dir);
    if(list->count>1)qsort(list->paths,list->count,sizeof(*list->paths),compare);
    return 0;
fail:
    closedir(dir); media_free(list); return -1;
}
