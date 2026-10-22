#include "lcd.h"
#include "bmp.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <string.h>
static int lcd_fd=-1;
static unsigned char *memory;
static size_t mapped_size;
static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;
static uint32_t *shadow;
static int batch_depth;
static int first_dirty[480],last_dirty[480];
static uint32_t native_color(uint32_t rgb)
{
    uint32_t result=((rgb>>16)&255)<<var.red.offset | ((rgb>>8)&255)<<var.green.offset | (rgb&255)<<var.blue.offset;
    if(var.transp.length==8 && var.transp.offset<=24)result|=255u<<var.transp.offset;
    return result;
}
void lcd_begin(void)
{
    int y;
    if(batch_depth++==0)for(y=0;y<480;y++) { first_dirty[y]=800; last_dirty[y]=-1; }
}
void lcd_end(void)
{
    uint32_t row[800]; int x,y;
    if(batch_depth<=0)return;
    if(--batch_depth || !memory || !shadow)return;
    for(y=0;y<480;y++)if(last_dirty[y]>=first_dirty[y]) {
        for(x=first_dirty[y];x<=last_dirty[y];x++)row[x]=native_color(shadow[(size_t)y*800+x]);
        memcpy(memory+(size_t)(y+var.yoffset)*fix.line_length+(first_dirty[y]+var.xoffset)*4,
               row+first_dirty[y],(size_t)(last_dirty[y]-first_dirty[y]+1)*4);
    }
}
void lcd_refresh(int x,int y,int width,int height)
{
    int xx,yy;
    if(!memory || !shadow || batch_depth || x<0 || y<0 || width<0 || height<0 || x>800-width || y>480-height)return;
    for(yy=y;yy<y+height;yy++)for(xx=x;xx<x+width;xx++) {
        uint32_t wanted=native_color(shadow[(size_t)yy*800+xx]),actual;
        unsigned char *pixel=memory+(size_t)(yy+var.yoffset)*fix.line_length+(xx+var.xoffset)*4;
        memcpy(&actual,pixel,4);
        if(actual!=wanted)memcpy(pixel,&wanted,4);
    }
}
int lcd_capture(Bmp *image,int x,int y,int width,int height)
{
    int xx,yy;
    *image=(Bmp){0,0,NULL};
    if(!memory || x<0 || y<0 || width<=0 || height<=0 || x>800-width || y>480-height)return -1;
    image->pixels=malloc((size_t)width*height*sizeof(*image->pixels));
    if(!image->pixels)return -1;
    image->width=width; image->height=height;
    for(yy=0;yy<height;yy++)for(xx=0;xx<width;xx++) {
        uint32_t native;
        memcpy(&native,memory+(size_t)(y+yy+var.yoffset)*fix.line_length+(x+xx+var.xoffset)*4,4);
        image->pixels[(size_t)yy*width+xx]=((native>>var.red.offset)&255)<<16 |
            ((native>>var.green.offset)&255)<<8 | ((native>>var.blue.offset)&255);
    }
    return 0;
}
int init_lcd(void)
{
    const char *path=getenv("LCD_DEVICE");
    lcd_fd=open(path ? path : "/dev/fb0",O_RDWR|O_CLOEXEC);
    if(lcd_fd<0) { perror("open framebuffer"); return -1; }
    if(ioctl(lcd_fd,FBIOGET_FSCREENINFO,&fix)<0 || ioctl(lcd_fd,FBIOGET_VSCREENINFO,&var)<0) {
        perror("framebuffer info"); uinit_lcd(); return -1;
    }
    if(var.xres!=800 || var.yres!=480 || var.bits_per_pixel!=32 ||
       var.red.length!=8 || var.green.length!=8 || var.blue.length!=8 ||
       var.red.offset>24 || var.green.offset>24 || var.blue.offset>24 ||
       var.red.msb_right || var.green.msb_right || var.blue.msb_right ||
       (uint64_t)(var.xoffset+800)*4>fix.line_length ||
       (uint64_t)(var.yoffset+480)*fix.line_length>fix.smem_len) {
        fprintf(stderr,"Expected 800x480 framebuffer with 32-bit RGB channels\n");
        uinit_lcd(); return -1;
    }
    mapped_size=fix.smem_len;
    memory=mmap(NULL,mapped_size,PROT_READ|PROT_WRITE,MAP_SHARED,lcd_fd,0);
    if(memory==MAP_FAILED) { memory=NULL; perror("mmap framebuffer"); uinit_lcd(); return -1; }
    shadow=calloc(800*480,sizeof(*shadow));
    if(!shadow) { uinit_lcd(); return -1; }
    batch_depth=0;
    return 0;
}
void uinit_lcd(void)
{
    if(memory) munmap(memory,mapped_size);
    if(lcd_fd>=0) close(lcd_fd);
    free(shadow); shadow=NULL; batch_depth=0;
    memory=NULL; lcd_fd=-1; mapped_size=0;
}
void lcd_pixel(int x,int y,uint32_t rgb)
{
    uint32_t native;
    if(!memory || x<0 || y<0 || x>=800 || y>=480) return;
    if(shadow)shadow[(size_t)y*800+x]=rgb;
    if(batch_depth) {
        if(x<first_dirty[y])first_dirty[y]=x;
        if(x>last_dirty[y])last_dirty[y]=x;
        return;
    }
    native=native_color(rgb);
    memcpy(memory+(size_t)(y+var.yoffset)*fix.line_length+(x+var.xoffset)*4,&native,4);
}
void lcd_pixel_direct(int x,int y,uint32_t rgb)
{
    uint32_t native;
    if(x<0 || y<0 || x>=800 || y>=480)return;
    if(shadow)shadow[(size_t)y*800+x]=rgb;
    if(!memory)return;
    native=native_color(rgb);
    memcpy(memory+(size_t)(y+var.yoffset)*fix.line_length+(x+var.xoffset)*4,&native,4);
}
uint32_t lcd_read_pixel(int x,int y)
{
    uint32_t native;
    if(x<0 || y<0 || x>=800 || y>=480)return 0;
    if(shadow)return shadow[(size_t)y*800+x];
    if(!memory)return 0;
    memcpy(&native,memory+(size_t)(y+var.yoffset)*fix.line_length+(x+var.xoffset)*4,4);
    return ((native>>var.red.offset)&255)<<16 |
        ((native>>var.green.offset)&255)<<8 | ((native>>var.blue.offset)&255);
}
void lcd_rect(int x,int y,int w,int h,uint32_t color)
{
    int xx,yy,xe=(int)((int64_t)x+w>800 ? 800 : (int64_t)x+w);
    int ye=(int)((int64_t)y+h>480 ? 480 : (int64_t)y+h);
    if(x<0)x=0;
    if(y<0)y=0;
    for(yy=y;yy<ye;yy++) for(xx=x;xx<xe;xx++) lcd_pixel(xx,yy,color);
}
int show_bmp(int x,int y,const char *path)
{
    Bmp image; int xx,yy;
    if(bmp_load(path,&image)<0) { fprintf(stderr,"Cannot read BMP: %s\n",path); return -1; }
    for(yy=0;yy<image.height;yy++) for(xx=0;xx<image.width;xx++)
        lcd_pixel(x+xx,y+yy,image.pixels[(size_t)yy*image.width+xx]);
    bmp_free(&image); return 0;
}
