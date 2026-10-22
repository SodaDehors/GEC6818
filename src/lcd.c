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
int init_lcd(void)
{
    const char *path=getenv("LCD_DEVICE");
    lcd_fd=open(path ? path : "/dev/fb0",O_RDWR);
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
    return 0;
}
void uinit_lcd(void)
{
    if(memory) munmap(memory,mapped_size);
    if(lcd_fd>=0) close(lcd_fd);
    memory=NULL; lcd_fd=-1; mapped_size=0;
}
void lcd_pixel(int x,int y,uint32_t rgb)
{
    uint32_t native;
    if(!memory || x<0 || y<0 || x>=800 || y>=480) return;
    native=((rgb>>16)&255)<<var.red.offset | ((rgb>>8)&255)<<var.green.offset | (rgb&255)<<var.blue.offset;
    if(var.transp.length==8 && var.transp.offset<=24) native|=255u<<var.transp.offset;
    memcpy(memory+(size_t)(y+var.yoffset)*fix.line_length+(x+var.xoffset)*4,&native,4);
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
