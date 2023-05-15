#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "disp_manager.h"
#include "video_manager.h"
#include "convert_manager.h"
#include "render.h"

/* usage：video2lcd </dev/videoX> ... */
int main(int argc,char *argv[])
{
    int iError;
    int iLcdWidth,iLcdHeight,iLcdBpp;
    int iPixelFormatOfVideo;
    int iPixelFormatOfDisp;
    int iTopLeftX,iTopLeftY;
    float k;
    T_VideoDevice tVideoDevice;
    T_VideoBuf tVideoBuf;
    T_VideoBuf tConvertBuf;
    T_VideoBuf tZoomBuf;
    T_VideoBuf tFrameBuf;
    PT_VideoBuf ptVideoBufCur;
    PT_VideoConvertOpr ptVideoConvertOpr;

    if(2 != argc){
        printf("usage:video2lcd </dev/videoX>\n");
        return -1;
    }

    /* 初始化 */
    /* 注册显示设备 */
    if(DisplayInit()){
        printf("%s-%s:DisplayInit failed!\n",__FILE__,__func__);
        return -1;
    }
    /* 可能有多个显示设置，需指定显示设备并对其进行初始化 */
    SelectAndInitDefaultDispDev("fb");
    GetDispResolution(&iLcdWidth,&iLcdHeight,&iLcdBpp);
    GetVideoBufForDisplay(&tFrameBuf);
    iPixelFormatOfDisp = tFrameBuf.iPixelFormat;

    if(VideoInit()){
        printf("%s-%s:VideoInit failed!\n",__FILE__,__func__);
        return -1;
    }

    if((iError = VideoDeviceInit("v4l2",argv[1],&tVideoDevice))){
        printf("%s-%s:VideoDeviceInit failed!\n",__FILE__,__func__);
        printf("Error code is %d\n",iError);
        return -1;
    }

    if(VideoConvertInit()){
        printf("%s-%s:VideoConvertInit failed!\n",__FILE__,__func__);
        return -1;
    }

    iPixelFormatOfVideo = tVideoDevice.ptVideoOpr->GetFormat(&tVideoDevice);
    ptVideoConvertOpr = GetVideoConvertForFormat(iPixelFormatOfVideo,iPixelFormatOfDisp);
    if(!ptVideoConvertOpr){
        printf("%s-%s:GetVideoConvertForFormat failed!\n",__FILE__,__func__);
        return -1;
    }

    /* 启动设备 */
    if(tVideoDevice.ptVideoOpr->StartDevice(&tVideoDevice)){
        printf("%s-%s:StartDevice failed!\n",__FILE__,__func__);
        return -1;
    }

    memset(&tVideoBuf,0,sizeof(tVideoBuf));
    memset(&tConvertBuf,0,sizeof(tVideoBuf));
    tConvertBuf.tPixelDatas.iBpp = iLcdBpp;
    tConvertBuf.iPixelFormat = iPixelFormatOfDisp;
    memset(&tZoomBuf,0,sizeof(tVideoBuf));
    while(1){
        /* 读入摄像头数据 */
        if(tVideoDevice.ptVideoOpr->GetFrame(&tVideoDevice,&tVideoBuf)){
            printf("%s-%s:GetFrame failed!\n",__FILE__,__func__);
            return -1;
        }
        ptVideoBufCur = &tVideoBuf;

        /* 转换为RGB */
        if(iPixelFormatOfDisp != iPixelFormatOfVideo){
            iError = ptVideoConvertOpr->Convert(&tVideoBuf,&tConvertBuf);
            if(iError){
                printf("%s-%s:Convert buffer failed!\n",__FILE__,__func__);
                return -1;
            }
            ptVideoBufCur = &tConvertBuf;
        }
        /* 如果图像数据与显示器不符，缩放视频数据以适配显示器 */
        /* 似乎只能缩小，不能放大，呵呵 */
        if(ptVideoBufCur->tPixelDatas.iWidth > iLcdWidth || ptVideoBufCur->tPixelDatas.iHeight > iLcdHeight){
            /* 确定缩放后的分辨率 */
            k = (float)ptVideoBufCur->tPixelDatas.iHeight / ptVideoBufCur->tPixelDatas.iWidth;
            tZoomBuf.tPixelDatas.iWidth = iLcdWidth;
            tZoomBuf.tPixelDatas.iHeight = iLcdWidth * k;
            if(tZoomBuf.tPixelDatas.iHeight > iLcdHeight){
                tZoomBuf.tPixelDatas.iHeight = iLcdHeight;
                tZoomBuf.tPixelDatas.iWidth = iLcdHeight / k;
            }
            tZoomBuf.tPixelDatas.iBpp = iLcdBpp;
            tZoomBuf.tPixelDatas.iLineBytes = tZoomBuf.tPixelDatas.iWidth * tZoomBuf.tPixelDatas.iBpp / 8;
            tZoomBuf.tPixelDatas.iTotalBytes = tZoomBuf.tPixelDatas.iLineBytes * tZoomBuf.tPixelDatas.iHeight;
            if(!tZoomBuf.tPixelDatas.aucPixelDatas){
                tZoomBuf.tPixelDatas.aucPixelDatas = malloc(tZoomBuf.tPixelDatas.iTotalBytes);
                if(!tZoomBuf.tPixelDatas.aucPixelDatas){
                    printf("%s-%s:malloc failed!\n",__FILE__,__func__);
                    return errno;
                }
            }
            
            PicZoom(&ptVideoBufCur->tPixelDatas,&tZoomBuf.tPixelDatas);
            ptVideoBufCur = &tZoomBuf;
        }

        /* 将数据合并到显存中 */
        /* 计算居中显示时，左上角座标 */
        iTopLeftX = (iLcdWidth - ptVideoBufCur->tPixelDatas.iWidth) / 2;
        iTopLeftY = (iLcdHeight - ptVideoBufCur->tPixelDatas.iHeight) / 2;
        PicMerge(iTopLeftX,iTopLeftY,&ptVideoBufCur->tPixelDatas,&tFrameBuf.tPixelDatas);
        
        /* 测试，将显存中的数据刷入显示器 */
        // memset(tFrameBuf.tPixelDatas.aucPixelDatas,0xff,tFrameBuf.tPixelDatas.iLineBytes * 20);
        FlushPixelDatasToDev(&tFrameBuf.tPixelDatas);
        iError = tVideoDevice.ptVideoOpr->PutFrame(&tVideoDevice,&tVideoBuf);
    }
}