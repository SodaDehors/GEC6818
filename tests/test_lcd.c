#include <assert.h>
#include "../src/lcd.c"
int main(void)
{
    Bmp capture={0}; uint32_t native,red=0xFF0000FFu,video=0xFF332211u;
    fix.line_length=805*4; var.xoffset=3; var.yoffset=2;
    var.red.offset=0; var.green.offset=8; var.blue.offset=16; var.transp.length=8; var.transp.offset=24;
    mapped_size=(size_t)fix.line_length*482; memory=calloc(1,mapped_size); shadow=calloc(800*480,sizeof(*shadow));
    assert(memory && shadow);
    memcpy(memory+(size_t)(100+var.yoffset)*fix.line_length+var.xoffset*4,&video,4);
    lcd_begin(); lcd_rect(0,0,800,64,0xFF0000); lcd_rect(0,288,800,192,0x00FF00);
    memcpy(&native,memory+(size_t)var.yoffset*fix.line_length+var.xoffset*4,4); assert(native==0);
    lcd_end();
    memcpy(&native,memory+(size_t)var.yoffset*fix.line_length+var.xoffset*4,4); assert(native==red);
    memcpy(&native,memory+(size_t)(100+var.yoffset)*fix.line_length+var.xoffset*4,4); assert(native==video);
    memset(memory+(size_t)var.yoffset*fix.line_length+var.xoffset*4,0,4);
    lcd_refresh(0,0,800,64);
    memcpy(&native,memory+(size_t)var.yoffset*fix.line_length+var.xoffset*4,4); assert(native==red);
    memcpy(&native,memory+(size_t)(100+var.yoffset)*fix.line_length+var.xoffset*4,4); assert(native==video);
    assert(lcd_capture(&capture,0,100,1,1)==0 && capture.pixels[0]==0x112233); bmp_free(&capture);
    assert(lcd_capture(&capture,-1,0,10,10)<0 && !capture.pixels);
    free(memory); free(shadow); memory=NULL; shadow=NULL;
    puts("PASS batched framebuffer writes/GUI restoration/video preservation/capture/color layout"); return 0;
}
