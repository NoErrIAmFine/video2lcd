#include <linux/videodev2.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>

#include "video_manager.h"
#include "disp_manager.h"

static T_VideoOpr g_tV4l2VideoOpr;
static int V4l2GetFrameForReadWrite(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf);
static int V4l2PutFrameForReadWrite(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf);

static unsigned int g_aiSupportedFormat[] = {
    V4L2_PIX_FMT_YUYV,V4L2_PIX_FMT_MJPEG,V4L2_PIX_FMT_RGB565,V4L2_PIX_FMT_RGB24
};

static int isSupportThisSupport(int iPixelFormat)
{
    int i;
    for(i = 0 ;i < sizeof(g_aiSupportedFormat) ; i++){
        if(iPixelFormat == g_aiSupportedFormat[i])
            return 1;
    }
    return 0;
}

static int V4l2GetFrameForStream(PT_VideoDevice ptVideoDevice,PT_VideoBuf ptVdieoBuf)
{
    int iret;
    struct pollfd tFds[1];
    struct v4l2_buffer tV4l2Buf;

    tFds[0].fd      = ptVideoDevice->iFd;
    tFds[0].events  = POLLIN;

    iret = poll(tFds,1,5000);
    if(iret < 0){
        perror("poll error\n");
        return errno;
    }
    if(iret == 0){
        DBG_PRINTF("%s-%s:poll timeout!\n",__FILE__,__func__);
        return -1;
    }

    /* 一切正常，取出buffer进行显示 */
    memset(&tV4l2Buf,0,sizeof(tV4l2Buf));
    tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tV4l2Buf.memory = V4L2_MEMORY_MMAP;
    iret = ioctl(ptVideoDevice->iFd,VIDIOC_DQBUF,&tV4l2Buf);
    if(iret < 0){
        DBG_PRINTF("%s-%s:unable to dequeue buffer!\n",__FILE__,__func__);
        return -1;
    }
    ptVideoDevice->iVideoBufCur = tV4l2Buf.index;

    ptVdieoBuf->iPixelFormat        = ptVideoDevice->iPixelFormat;
    ptVdieoBuf->tPixelDatas.iWidth  = ptVideoDevice->iWidth;
    ptVdieoBuf->tPixelDatas.iHeight = ptVideoDevice->iHeight;
    switch(ptVideoDevice->iPixelFormat){
    case V4L2_PIX_FMT_YUYV:
        ptVdieoBuf->tPixelDatas.iBpp = 16;
        break;
    case V4L2_PIX_FMT_MJPEG:
        ptVdieoBuf->tPixelDatas.iBpp = 0;
        break;
    case V4L2_PIX_FMT_RGB565:
        ptVdieoBuf->tPixelDatas.iBpp = 16;
        break;
    default:
        DBG_PRINTF("%s-%s:unsupported pix format!\n",__FILE__,__func__);
        ptVdieoBuf->tPixelDatas.iBpp = 0;
    }
    ptVdieoBuf->tPixelDatas.iLineBytes    = ptVideoDevice->iWidth * ptVdieoBuf->tPixelDatas.iBpp / 8;
    ptVdieoBuf->tPixelDatas.iTotalBytes   = tV4l2Buf.bytesused;
    ptVdieoBuf->tPixelDatas.aucPixelDatas = ptVideoDevice->pucVideoBuf[tV4l2Buf.index];
    return 0;
}

static int V4l2PutFrameForStream(PT_VideoDevice ptVideoDevice,PT_VideoBuf ptVdieoBuf)
{
    int iret;
    struct v4l2_buffer tV4l2Buf;
    
    memset(&tV4l2Buf,0,sizeof(tV4l2Buf));
    tV4l2Buf.index  = ptVideoDevice->iVideoBufCur;
    tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tV4l2Buf.memory = V4L2_MEMORY_MMAP;
    iret = ioctl(ptVideoDevice->iFd,VIDIOC_QBUF,&tV4l2Buf);
    if(iret < 0){
        DBG_PRINTF("%s-%s:unable to queue buffer!\n",__FILE__,__func__);
        return -1;
    }
    ptVideoDevice->iVideoBufCur = -1;
    return 0;
}

static int V4l2GetFrameForReadWrite(PT_VideoDevice ptVideoDevice,PT_VideoBuf ptVdieoBuf)
{
    int iret;
    iret = read(ptVideoDevice->iFd,ptVideoDevice->pucVideoBuf[0],ptVideoDevice->iVideoBufLen);
    if(iret < 0){
        perror("read failed!\n");
        return errno;
    }

    ptVdieoBuf->iPixelFormat        = ptVideoDevice->iPixelFormat;
    ptVdieoBuf->tPixelDatas.iWidth  = ptVideoDevice->iWidth;
    ptVdieoBuf->tPixelDatas.iHeight = ptVideoDevice->iHeight;
    switch(ptVideoDevice->iPixelFormat){
    case V4L2_PIX_FMT_YUYV:
        ptVdieoBuf->tPixelDatas.iBpp = 16;
        break;
    case V4L2_PIX_FMT_MJPEG:
        ptVdieoBuf->tPixelDatas.iBpp = 0;
        break;
    case V4L2_PIX_FMT_RGB565:
        ptVdieoBuf->tPixelDatas.iBpp = 16;
        break;
    default:
        DBG_PRINTF("%s-%s:unsupported pix format!\n",__FILE__,__func__);
        ptVdieoBuf->tPixelDatas.iBpp = 0;
    }
    ptVdieoBuf->tPixelDatas.iLineBytes    = ptVideoDevice->iWidth * ptVdieoBuf->tPixelDatas.iBpp / 8;
    ptVdieoBuf->tPixelDatas.iTotalBytes   = ptVideoDevice->iVideoBufLen;
    ptVdieoBuf->tPixelDatas.aucPixelDatas = ptVideoDevice->pucVideoBuf[0];
    return 0;
}

static int V4l2PutFrameForReadWrite(PT_VideoDevice ptVideoDevice,PT_VideoBuf ptVdieoBuf)
{
    return 0;
}


static int V4l2InitDevice(const char *strDevName,PT_VideoDevice ptVideoDevice)
{
    int i;
    int iFd;
    int iError;
    int iLcdWidth,iLcdHeight,iLcdBpp;
    struct v4l2_capability tV4l2Cap;
    struct v4l2_fmtdesc tFmtDesc;
    struct v4l2_format tV4l2Format;
    struct v4l2_requestbuffers tV4l2ReqBufs;
    struct v4l2_buffer tV4l2Buf;

    iFd = open(strDevName,O_RDWR);
    if(iFd < 0){
        DBG_PRINTF("can't open file %s\n",strDevName);
        return -1;
    }
    ptVideoDevice->iFd = iFd;
    
    /* 确定设备是否为捕获设备 */
    memset(&tV4l2Cap,0,sizeof(tV4l2Cap));
    iError = ioctl(iFd,VIDIOC_QUERYCAP,&tV4l2Cap);
    if(iError < 0){
        DBG_PRINTF("Error opening device %s:failed to query cap!\n",strDevName);
        goto err_exit;
    }
    if(!(tV4l2Cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
        DBG_PRINTF("%s is not a device capture device\n",strDevName);
        goto err_exit;
    }

    /* 将该参数记录到 ptVideoDevice 中 */
    ptVideoDevice->icapabilities = tV4l2Cap.capabilities;

    if(tV4l2Cap.capabilities & V4L2_CAP_STREAMING){
        DBG_PRINTF("%s suports streaming i/o\n",strDevName);
    }

     if(tV4l2Cap.capabilities & V4L2_CAP_READWRITE){
        DBG_PRINTF("%s suports read write i/o\n",strDevName);
    }

    /* enum format */
    memset(&tFmtDesc,0,sizeof(tFmtDesc));
    tFmtDesc.index = 0;
    tFmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while(0 == (iError = ioctl(iFd,VIDIOC_ENUM_FMT,&tFmtDesc))){
        DBG_PRINTF("tFmtDesc.pixelformat is %d\n",tFmtDesc.pixelformat);
        if(isSupportThisSupport(tFmtDesc.pixelformat)){
            ptVideoDevice->iPixelFormat = tFmtDesc.pixelformat;
            break;
        }
        tFmtDesc.index++;
    }

    if(!ptVideoDevice->iPixelFormat){
        DBG_PRINTF("no supported format existed in the device!\n");
        goto err_exit;
    }

    /*set format */
    /* 先读出lcd的宽度和高度 */
    GetDispResolution(&iLcdWidth,&iLcdHeight,&iLcdBpp);
    memset(&tV4l2Format,0,sizeof(tV4l2Format));
    tV4l2Format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tV4l2Format.fmt.pix.pixelformat = ptVideoDevice->iPixelFormat;
    tV4l2Format.fmt.pix.width       =  iLcdWidth;
    tV4l2Format.fmt.pix.height      =  iLcdHeight;
    tV4l2Format.fmt.pix.field       =  V4L2_FIELD_ANY;

    /* 如果驱动程序发现无法设置某些参数（比如分辨率），它会调整这些参数，
     * 并且返回给应用程序，所以调用成功后，需要重新读取参数
     */
    iError = ioctl(iFd,VIDIOC_S_FMT,&tV4l2Format);
    if(iError){
        DBG_PRINTF("%s-%s:unable to set format!\n",__FILE__,__func__);
        goto err_exit;
    }
    ptVideoDevice->iWidth  = tV4l2Format.fmt.pix.width;
    ptVideoDevice->iHeight = tV4l2Format.fmt.pix.height;
    printf("%s-%d-ptVideoDevice->iWidth:%d\n",__func__,__LINE__,ptVideoDevice->iWidth);
    printf("%s-%d-ptVideoDevice->iHeight:%d\n",__func__,__LINE__,ptVideoDevice->iHeight);
    /* 申请buffer */
    memset(&tV4l2ReqBufs,0,sizeof(tV4l2ReqBufs));
    tV4l2ReqBufs.count  = NB_BUFFER;
    tV4l2ReqBufs.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tV4l2ReqBufs.memory = V4L2_MEMORY_MMAP;

    iError = ioctl(iFd,VIDIOC_REQBUFS,&tV4l2ReqBufs);
    if(iError){
        DBG_PRINTF("%s-%s:unable to requeset buffers!\n",__FILE__,__func__);
        goto err_exit;
    }
    ptVideoDevice->iVideoBufCnt = tV4l2ReqBufs.count;

    /* 设置buffer，只有streaming设备需要设置*/
    if(tV4l2Cap.capabilities & V4L2_CAP_STREAMING){
        for(i = 0 ; i < ptVideoDevice->iVideoBufCnt ; i++){
            memset(&tV4l2Buf,0,sizeof(tV4l2Buf));
            tV4l2Buf.index  = i;
            tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            tV4l2Buf.memory = V4L2_MEMORY_MMAP;
            iError = ioctl(iFd,VIDIOC_QUERYBUF,&tV4l2Buf);
            if(iError){
                DBG_PRINTF("%s-%s:unable to query buffer!\n",__FILE__,__func__);
                goto err_exit;
            }
            ptVideoDevice->iVideoBufLen = tV4l2Buf.length;
            printf("%s-%d-tV4l2Buf.length:%d\n",__func__,__LINE__,tV4l2Buf.length);
            ptVideoDevice->pucVideoBuf[i] = mmap(NULL,tV4l2Buf.length,PROT_READ,MAP_SHARED,iFd,tV4l2Buf.m.offset);
            if(ptVideoDevice->pucVideoBuf[i] == MAP_FAILED){
                DBG_PRINTF("%s-%s:unable to map buffer!\n",__FILE__,__func__);
                goto err_exit;
            }
        }

        for(i = 0 ; i < ptVideoDevice->iVideoBufCnt ; i++){
            memset(&tV4l2Buf,0,sizeof(tV4l2Buf));
            tV4l2Buf.index  = i;
            tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            tV4l2Buf.memory = V4L2_MEMORY_MMAP;
            iError = ioctl(iFd,VIDIOC_QBUF,&tV4l2Buf);
            if(iError){
                DBG_PRINTF("%s-%s:unable to queue buffer!\n",__FILE__,__func__);
                goto err_exit;
            }
        }
    }else if(tV4l2Cap.capabilities & V4L2_CAP_READWRITE){
        ptVideoDevice->iVideoBufCnt = 1;
        /* 尽量选择一个最大的缓冲区 */
        ptVideoDevice->iVideoBufLen = ptVideoDevice->iHeight * ptVideoDevice->iWidth * 4;
        ptVideoDevice->pucVideoBuf[0] = malloc(ptVideoDevice->iVideoBufLen);
        if(!ptVideoDevice->pucVideoBuf[0]){
            DBG_PRINTF("%s-%s:malloc failed!\n",__FILE__,__func__);
            return -ENOMEM;
        }
        g_tV4l2VideoOpr.GetFrame = V4l2GetFrameForReadWrite;
        g_tV4l2VideoOpr.PutFrame = V4l2PutFrameForReadWrite;
    }
    ptVideoDevice->ptVideoOpr = &g_tV4l2VideoOpr;
    return 0;
err_exit:
    close(iFd);
    return -1;
}

static int V4l2ExitDevice(PT_VideoDevice ptVideoDevice)
{
    int i;
    // int iError;
    struct v4l2_buffer tV4l2Buf;

    
    if(ptVideoDevice->icapabilities & V4L2_CAP_STREAMING){
        for(i = 0 ; i < ptVideoDevice->iVideoBufCnt ; i++){
            if(!ptVideoDevice->pucVideoBuf[i])
                continue;
            // memset(&tV4l2Buf,0,sizeof(tV4l2Buf));
            // tV4l2Buf.index  = i;
            // tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            // tV4l2Buf.memory = V4L2_MEMORY_MMAP;
            // iError = ioctl(iFd,VIDIOC_QUERYBUF,&tV4l2Buf);
            // if(iError){
            //     DBG_PRINTF("unable to query buffer!\n");
            //     goto err_exit;
            // }
            munmap(ptVideoDevice->pucVideoBuf[i],ptVideoDevice->iVideoBufLen);
            ptVideoDevice->pucVideoBuf[i] = NULL;
        }
    }else if(ptVideoDevice->icapabilities & V4L2_CAP_READWRITE){
        free(ptVideoDevice->pucVideoBuf[0]);
    }

    close(ptVideoDevice->iFd);

    return 0;
}

static int V4l2StartDevice(PT_VideoDevice ptVideoDevice)
{
    int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError;

    iError = ioctl(ptVideoDevice->iFd,VIDIOC_STREAMON,&iType);
    if(iError){
        DBG_PRINTF("unable to start capture!\n");
        printf("errno is :%d\n",iError);
        return -1;
    }
    return 0;
}

static int V4l2StopDevice(PT_VideoDevice ptVideoDevice)
{
    int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError;

    iError = ioctl(ptVideoDevice->iFd,VIDIOC_STREAMOFF,&iType);
    if(iError){
        DBG_PRINTF("unable to stop capture!\n");
        return -1;
    }
    return 0;
}

static int V4l2GetFormat(PT_VideoDevice ptVideoDevice)
{
    return ptVideoDevice->iPixelFormat;
}

/* 构造一个VideoOpr结构体 */
static T_VideoOpr g_tV4l2VideoOpr = {
    .name       = "v4l2",
    .InitDevice = V4l2InitDevice,
    .ExitDevice = V4l2ExitDevice, 
    .GetFrame   = V4l2GetFrameForStream,
    .PutFrame   = V4l2PutFrameForStream, 
    .StartDevice= V4l2StartDevice,
    .StopDevice = V4l2StopDevice,
    .GetFormat  = V4l2GetFormat,
};

int V4l2Init(void)
{
    if(RegisterVideoOpr(&g_tV4l2VideoOpr))
        return -1;
    
    return 0;
}