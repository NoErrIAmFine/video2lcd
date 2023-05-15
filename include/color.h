#ifndef __COLOR_H
#define __COLOR_H

typedef struct Myrgb16 {
	unsigned short blue:5;
	unsigned short green:6;
	unsigned short red:5;
} Myrgb16;
typedef struct Myrgb24 {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
} Myrgb24;
typedef struct Myrgb32 {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char alpha;
} Myrgb32;

typedef struct MyYUV422 {
	unsigned char y0;
	unsigned char u;
	unsigned char y1;
	unsigned char v;
} MyYUV422;

typedef struct MyYUV444 {
	unsigned char y;
	unsigned char u;
	unsigned char v;
} MyYUV444;

#define CLIP(color) (unsigned char)(((color)>0xFF)?0xff:(((color)<0)?0:(color)))

unsigned char
RGB24_TO_Y(unsigned char r, unsigned char g, unsigned char b);

unsigned char
YR_TO_V(unsigned char r, unsigned char y);

unsigned char
YB_TO_U(unsigned char b, unsigned char y);

unsigned char
R_FROMYV(unsigned char y, unsigned char v);

unsigned char
G_FROMYUV(unsigned char y, unsigned char u, unsigned char v);

unsigned char
B_FROMYU(unsigned char y, unsigned char u);

#define YfromRGB(r,g,b) CLIP((77*(r)+150*(g)+29*(b))>>8)
#define UfromRGB(r,g,b) CLIP(((128*(b)-85*(g)-43*(r))>>8 )+128)
#define VfromRGB(r,g,b) CLIP(((128*(r)-107*(g)-21*(b))>>8) +128)

#define PACKRGB16(r,g,b) (__u16) ((((b) & 0xF8) << 8 ) | (((g) & 0xFC) << 3 ) | (((r) & 0xF8) >> 3 ))
#define UNPACK16(pixel,r,g,b) r=((pixel)&0xf800) >> 8; 	g=((pixel)&0x07e0) >> 3; b=(((pixel)&0x001f) << 3)

void initLut(void);
void freeLut(void);

#endif // !__COLOR_H