#ifndef __CONVERT_MANAGER_H
#define __CONVERT_MANAGER_H

#include "video_manager.h"

typedef struct VideoConvertOpr
{
    const char *name;
    struct VideoConvertOpr *ptNext;
    int (*isSupport)(int iPixelFormatIn,int iPixelFormatOut);
    int (*Convert)(PT_VideoBuf ptVideoBufIn,PT_VideoBuf ptVideoBufOut);
    int (*ConvertExit)(PT_VideoBuf ptVideoBufOut);
    void (*release)(struct VideoConvertOpr *ptVdieoConvertOpr);
}T_VideoConvertOpr,*PT_VideoConvertOpr;

int VideoConvertInit(void);
PT_VideoConvertOpr GetVideoConvertForFormat(int iPixelFormatIn,int iPixelFormatOut);

int Yuv2RgbInit(void);
int Mjpeg2RgbInit(void);
int Rgb2RgbInit(void);
int RegisterVideoConvertOpr(PT_VideoConvertOpr ptVideoCOnvertOpr);

#endif // !__CONVERT_MANAGER_H