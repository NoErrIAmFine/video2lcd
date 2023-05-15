#include <linux/videodev2.h>
#include <stdlib.h>
#include <errno.h>

#include "convert_manager.h"
#include "video_manager.h"
#include "color.h"

static int isSupportYuv2Ggb(int iPixelFormatIn,int iPixelFormatOut)
{
    if(iPixelFormatIn != V4L2_PIX_FMT_YUYV){
        return 0;
    }
    if((iPixelFormatOut != V4L2_PIX_FMT_RGB565) && (iPixelFormatOut != V4L2_PIX_FMT_RGB32)){
        return 0;
    }
    return 1;
}

static int Pyuv422torgb565(unsigned char *input_ptr,unsigned char *output_ptr,unsigned int image_width,unsigned int image_height)
{
    unsigned int i,size;
    unsigned char *buff = input_ptr;
    unsigned char *output_pt = output_ptr;
    unsigned char Y,Y1,U,V;
    unsigned int r,g,b;
    unsigned int color;

    size = image_width * image_height;

    for(i = size ; i > 0 ; i--){
        Y  = buff[0];
        Y1 = buff[1];
        U  = buff[2];
        V  = buff[3];
        buff += 4;
        r = R_FROMYV(Y,V);
        g = G_FROMYUV(Y,U,V);
        b = B_FROMYU(Y,U);
        r >>= 3;
        g >>= 2;
        b >>= 3;
        color = (r << 11) | (g << 5) | b;
        *output_pt++ = color & 0xff;
        *output_pt++ = (color >> 8) & 0xff;

        r = R_FROMYV(Y1,V);
        g = G_FROMYUV(Y1,U,V);
        b = B_FROMYU(Y1,U);
        r >>= 3;
        g >>= 2;
        b >>= 3;
        color = (r << 11) | (g << 5) | b;
        *output_pt++ = color & 0xff;
        *output_pt++ = (color >> 8) & 0xff;
    }
    return 0;
}

static int Pyuv422torgb32(unsigned char *input_ptr,unsigned char *output_ptr,unsigned int image_width,unsigned int image_height)
{
    unsigned int i,size;
    unsigned char *buff = input_ptr;
    unsigned int *output_pt = (unsigned int *)output_ptr;
    unsigned char Y,Y1,U,V;
    unsigned int r,g,b;
    unsigned int color;

    size = image_width * image_height;

    for(i = size ; i > 0 ; i--){
        Y  = buff[0];
        Y1 = buff[1];
        U  = buff[2];
        V  = buff[3];
        buff += 4;
        r = R_FROMYV(Y,V);
        g = G_FROMYUV(Y,U,V);
        b = B_FROMYU(Y,U);
        color = (r << 16) | (g << 8) | b;
        *output_pt++ = color;

        r = R_FROMYV(Y1,V);
        g = G_FROMYUV(Y1,U,V);
        b = B_FROMYU(Y1,U);
        color = (r << 16) | (g << 8) | b;
        *output_pt++ = color;
    }
    return 0;
}

static int Yuv2GgbConvert(PT_VideoBuf ptVideoBufIn,PT_VideoBuf ptVideoBufOut)
{
    PT_PixelDatas ptPixelDatasIn  = &ptVideoBufIn->tPixelDatas;
    PT_PixelDatas ptPixelDatasOut = &ptVideoBufOut->tPixelDatas;

    ptPixelDatasOut->iWidth  = ptPixelDatasIn->iWidth;
    ptPixelDatasOut->iHeight = ptPixelDatasIn->iHeight;

    if(ptVideoBufOut->iPixelFormat == V4L2_PIX_FMT_RGB565){
        ptPixelDatasOut->iBpp = 16;
        ptPixelDatasOut->iLineBytes = ptPixelDatasOut->iWidth * ptPixelDatasOut->iBpp / 8;
        ptPixelDatasOut->iTotalBytes = ptPixelDatasOut->iLineBytes * ptPixelDatasOut->iHeight;
        if(!ptPixelDatasOut->aucPixelDatas){
            ptPixelDatasOut->aucPixelDatas = malloc(ptPixelDatasOut->iTotalBytes);
            if(!ptPixelDatasOut->aucPixelDatas){
                DBG_PRINTF("%s-%s:malloc failed!\n",__FILE__,__func__);
                return errno;
            }
        }
        Pyuv422torgb565(ptPixelDatasIn->aucPixelDatas,ptPixelDatasOut->aucPixelDatas,ptPixelDatasOut->iWidth,ptPixelDatasOut->iHeight);
    }else if(ptVideoBufOut->iPixelFormat == V4L2_PIX_FMT_RGB32){
        ptPixelDatasOut->iBpp = 32;
        ptPixelDatasOut->iLineBytes = ptPixelDatasOut->iWidth * ptPixelDatasOut->iBpp / 8;
        ptPixelDatasOut->iTotalBytes = ptPixelDatasOut->iLineBytes * ptPixelDatasOut->iHeight;
        if(!ptPixelDatasOut->aucPixelDatas){
            ptPixelDatasOut->aucPixelDatas = malloc(ptPixelDatasOut->iTotalBytes);
            if(!ptPixelDatasOut->aucPixelDatas){
                DBG_PRINTF("%s-%s:malloc failed!\n",__FILE__,__func__);
                return errno;
            }
        }
        Pyuv422torgb32(ptPixelDatasIn->aucPixelDatas,ptPixelDatasOut->aucPixelDatas,ptPixelDatasOut->iWidth,ptPixelDatasOut->iHeight);
    }
    return 0;
}

static int Yuv2GgbConvertExit(PT_VideoBuf ptVideoBufOut)
{
    if(ptVideoBufOut->tPixelDatas.aucPixelDatas){
        free(ptVideoBufOut->tPixelDatas.aucPixelDatas);
        ptVideoBufOut->tPixelDatas.aucPixelDatas = NULL;
    }
    return 0;
}

static void Yuv2GgbRelease(struct VideoConvertOpr * ptVdieoConvertOpr)
{
    freeLut();
}

static T_VideoConvertOpr g_tYuv2GgbConvert = {
    .name       = "yuv2rgb",
    .isSupport  = isSupportYuv2Ggb,
    .Convert    = Yuv2GgbConvert,
    .ConvertExit= Yuv2GgbConvertExit,
    .release    = Yuv2GgbRelease,
};

int Yuv2RgbInit(void)
{
    int ierror;
    initLut();
    ierror = RegisterVideoConvertOpr(&g_tYuv2GgbConvert);

    return ierror;
}