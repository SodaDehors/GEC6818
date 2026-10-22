#include "lcd.h"
#include "font_data.h"
#include <ctype.h>

static int style_for(int scale)
{
    if(scale<=1)return 0;
    if(scale==2)return 1;
    return 2;
}
static int char_index(uint32_t codepoint)
{
    int i,fallback=0;
    if(codepoint<128)codepoint=(unsigned char)toupper((unsigned char)codepoint);
    for(i=0;i<FONT_GLYPH_COUNT;i++) {
        if(font_codepoints[i]==codepoint)return i;
        if(font_codepoints[i]=='?')fallback=i;
    }
    return fallback;
}
static const uint8_t *font_pixels(int style)
{
    if(style==0)return font_pixels_0;
    if(style==1)return font_pixels_1;
    return font_pixels_2;
}
static uint32_t blend(uint32_t bg,uint32_t fg,unsigned a)
{
    unsigned r=(((fg>>16)&255)*a+((bg>>16)&255)*(255-a)+127)/255;
    unsigned g=(((fg>>8)&255)*a+((bg>>8)&255)*(255-a)+127)/255;
    unsigned b=((fg&255)*a+(bg&255)*(255-a)+127)/255;
    return (r<<16)|(g<<8)|b;
}
static uint32_t next_codepoint(const char **text)
{
    const unsigned char *p=(const unsigned char *)*text;
    uint32_t codepoint;
    if(p[0]<0x80) {
        *text=(const char *)(p+1); return p[0];
    }
    if((p[0]&0xE0)==0xC0 && (p[1]&0xC0)==0x80) {
        codepoint=((uint32_t)(p[0]&0x1F)<<6)|(p[1]&0x3F);
        *text=(const char *)(p+2); return codepoint;
    }
    if((p[0]&0xF0)==0xE0 && (p[1]&0xC0)==0x80 && (p[2]&0xC0)==0x80) {
        codepoint=((uint32_t)(p[0]&0x0F)<<12)|((uint32_t)(p[1]&0x3F)<<6)|(p[2]&0x3F);
        *text=(const char *)(p+3); return codepoint;
    }
    if((p[0]&0xF8)==0xF0 && (p[1]&0xC0)==0x80 && (p[2]&0xC0)==0x80 && (p[3]&0xC0)==0x80) {
        codepoint=((uint32_t)(p[0]&0x07)<<18)|((uint32_t)(p[1]&0x3F)<<12)|
                  ((uint32_t)(p[2]&0x3F)<<6)|(p[3]&0x3F);
        *text=(const char *)(p+4); return codepoint;
    }
    *text=(const char *)(p+1);
    return '?';
}
static int next_char(const char **text)
{
    return char_index(next_codepoint(text));
}
int lcd_text_width(const char *text,int scale)
{
    const char *p=text; int style=style_for(scale),width=0;
    while(*p)width+=font_glyphs[style][next_char(&p)].advance;
    return width;
}
int lcd_text_height(int scale)
{
    int style=style_for(scale);
    return font_glyphs[style][char_index('A')].height;
}
void lcd_text(int x,int y,const char *text,int scale,uint32_t color)
{
    const char *p=text; int style=style_for(scale);
    const uint8_t *pixels=font_pixels(style);
    while(*p) {
        int index=next_char(&p); const FontGlyph *g=&font_glyphs[style][index];
        int row,col; const uint8_t *glyph=pixels+g->offset;
        for(row=0;row<g->height;row++)for(col=0;col<g->width;col++) {
            unsigned alpha=glyph[row*g->width+col];
            int xx=x+col,yy=y+row;
            if(alpha && xx>=0 && xx<800 && yy>=0 && yy<480)
                lcd_pixel(xx,yy,alpha==255 ? color : blend(lcd_read_pixel(xx,yy),color,alpha));
        }
        x+=g->advance;
        if(x>=800)break;
    }
}
