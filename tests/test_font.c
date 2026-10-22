/* Regression: every embedded glyph must retain empty margins on all sides. */
#include <assert.h>
#include "../src/font.c"
static unsigned writes;
void lcd_pixel(int x,int y,uint32_t color)
{ assert(x>=0 && x<800 && y>=0 && y<480); (void)color; writes++; }
uint32_t lcd_read_pixel(int x,int y)
{ (void)x; (void)y; return 0xFFFFFF; }
int main(void)
{
    int style,i,x,y;
    for(style=0;style<FONT_STYLE_COUNT;style++) {
        const uint8_t *pixels=font_pixels(style);
        for(i=0;i<FONT_GLYPH_COUNT;i++) {
            const FontGlyph *g=&font_glyphs[style][i];
            for(y=0;y<g->height;y++)for(x=0;x<g->width;x++)
                if(x<2 || y<2 || x>=g->width-2 || y>=g->height-2)
                    assert(pixels[g->offset+y*g->width+x]==0);
        }
    }
    assert(char_index(0x1F600)==char_index('?'));
    assert(lcd_text_width("\xF0\x9F\x98\x80",2)==lcd_text_width("?",2));
    lcd_text(0,0,"视频 音乐 相册 收藏 亮度",2,0x102030);
    assert(writes>500);
    lcd_text(799,479,"视频",2,0);
    puts("PASS complete glyph margins, UTF-8 fallback and clipped screen drawing");
    return 0;
}
