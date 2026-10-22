#include "lcd.h"
#include <ctype.h>
/* Original compact 5x7 uppercase font; no runtime font library is needed. */
static const unsigned char glyphs[][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14},
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
void lcd_text(int x,int y,const char *text,int scale,uint32_t color)
{
    const unsigned char *p=(const unsigned char *)text;
    if(scale<1)return;
    while(*p && x+5*scale<=800) {
        unsigned char c=(unsigned char)toupper(*p++); unsigned char rows[7]={0}; int i,j;
        if(c>=128) { while((*p&192)==128)p++; c='?'; }
        if(c>='0' && c<='9')for(i=0;i<7;i++)rows[i]=glyphs[c-'0'][i];
        else if(c>='A' && c<='Z')for(i=0;i<7;i++)rows[i]=glyphs[c-'A'+10][i];
        else if(c=='-')rows[3]=31;
        else if(c=='_')rows[6]=31;
        else if(c=='+') { rows[1]=rows[2]=rows[4]=rows[5]=4; rows[3]=31; }
        else if(c=='.')rows[6]=4;
        else if(c==':')rows[2]=rows[5]=4;
        else if(c=='/')for(i=0;i<7;i++)rows[i]=1u<<(i*4/6);
        else if(c!=' ') { rows[0]=14; rows[1]=17; rows[2]=1; rows[3]=2; rows[4]=4; rows[6]=4; }
        for(i=0;i<7;i++)for(j=0;j<5;j++)if(rows[i]&(1u<<(4-j)))
            lcd_rect(x+j*scale,y+i*scale,scale,scale,color);
        x+=6*scale;
    }
}
