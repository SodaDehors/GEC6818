#include "bmp.h"
#include "media.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
static void le(unsigned char *p,unsigned n,int bytes)
{ int i; for(i=0;i<bytes;i++)p[i]=(unsigned char)(n>>(i*8)); }
static void fixture(const char *path,int top_down,int bpp,int truncate)
{
    unsigned char h[70]={0},rows[16]={0}; FILE *f;
    int pixel=bpp/8,stride=4;
    h[0]='B'; h[1]='M'; le(h+2,78,4); le(h+10,70,4); le(h+14,40,4);
    le(h+18,1,4); le(h+22,top_down ? (unsigned)-2 : 2,4);
    le(h+26,1,2); le(h+28,(unsigned)bpp,2);
    /* Width 1 requires padding for 24-bit BMP; first row is red, second blue. */
    rows[2]=255; rows[stride]=255;
    if(pixel==4) { rows[3]=123; rows[stride+3]=123; }
    f=fopen(path,"wb"); assert(f); assert(fwrite(h,1,sizeof(h),f)==sizeof(h));
    assert(fwrite(rows,1,truncate ? 2 : 8,f)==(size_t)(truncate ? 2 : 8)); fclose(f);
}
int main(void)
{
    char folder[]="/tmp/gec6818-core-XXXXXX",path[256]; Bmp b; MediaList list;
    assert(mkdtemp(folder));
    snprintf(path,sizeof(path),"%s/01.bmp",folder); fixture(path,0,24,0);
    assert(bmp_load(path,&b)==0 && b.width==1 && b.height==2);
    assert(b.pixels[0]==0x0000ff && b.pixels[1]==0xff0000); bmp_free(&b);
    snprintf(path,sizeof(path),"%s/02.BMP",folder); fixture(path,1,32,0);
    assert(bmp_load(path,&b)==0 && b.pixels[0]==0xff0000 && b.pixels[1]==0x0000ff); bmp_free(&b);
    snprintf(path,sizeof(path),"%s/03.bmp",folder); fixture(path,0,24,1);
    assert(bmp_load(path,&b)<0 && !b.pixels);
    snprintf(path,sizeof(path),"%s/folder.bmp",folder); assert(mkdir(path,0700)==0);
    snprintf(path,sizeof(path),"%s/a song 'quoted'.mp3",folder);
    { FILE *f=fopen(path,"wb"); assert(f); fclose(f); }
    assert(media_scan(folder,MEDIA_PHOTO,&list)==0 && list.count==3);
    assert(!strcmp(media_name(list.paths[0]),"01.bmp")); media_free(&list);
    assert(media_scan(folder,MEDIA_MUSIC,&list)==0 && list.count==1);
    assert(!strcmp(media_name(list.paths[0]),"a song 'quoted'.mp3")); media_free(&list);
    assert(media_scan(folder,MEDIA_VIDEO,&list)==0 && list.count==0); media_free(&list);
    assert(media_scan("/path/that/does/not/exist",MEDIA_PHOTO,&list)<0 && !list.count);
    unlink(path);
    snprintf(path,sizeof(path),"%s/01.bmp",folder); unlink(path);
    snprintf(path,sizeof(path),"%s/02.BMP",folder); unlink(path);
    snprintf(path,sizeof(path),"%s/03.bmp",folder); unlink(path);
    snprintf(path,sizeof(path),"%s/folder.bmp",folder); rmdir(path); rmdir(folder);
    puts("PASS BMP padding/orientation/offset/truncation and media scanning"); return 0;
}
