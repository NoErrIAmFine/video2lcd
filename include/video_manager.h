#ifndef __VIDEO_MANAGER_H
#define __VIDEO_MANAGER_H

#include <config.h>
#include "pic_operation.h"

#define NB_BUFFER 4

struct VideoDevice;
struct VideoOpr;
typedef struct VideoDevice T_VideoDevice,*PT_VideoDevice;
typedef struct VideoOpr T_VideoOpr,*PT_VideoOpr;

struct VideoDevice
{
    char *name;
    int iFd;
    int iPixelFormat;
    unsigned int icapabilities;
    int iWidth;
    int iHeight;
    int iVideoBufCnt;
    int iVideoBufLen;
    int iVideoBufCur;
    unsigned char *pucVideoBuf[NB_BUFFER];
    PT_VideoOpr ptVideoOpr;
};

typedef struct VideoBuf
{
    T_PixelDatas tPixelDatas;
    int iPixelFormat;
}T_VideoBuf,*PT_VideoBuf;

struct VideoOpr
{
    char *name;
    struct VideoOpr *ptNext;
    int (*InitDevice)(const char *dev_name,PT_VideoDevice ptVideoDevice);
    int (*ExitDevice)(PT_VideoDevice ptVideoDevice);
    int (*GetFrame)(PT_VideoDevice ptVideoDevice,PT_VideoBuf ptVdieoBuf);
    int (*PutFrame)(PT_VideoDevice ptVideoDevice,PT_VideoBuf ptVdieoBuf);
    int (*StartDevice)(PT_VideoDevice ptVideoDevice);
    int (*StopDevice)(PT_VideoDevice ptVideoDevice);
    int (*GetFormat)(PT_VideoDevice ptVideoDevice);
};

int RegisterVideoOpr(PT_VideoOpr ptVideoOpr);
int VideoDeviceInit(const char *pcTypeName,const char *pcDevName,PT_VideoDevice ptVideoDevice);
int VideoInit(void);
int V4l2Init(void);

#endif // !__VDIEO_MANAGER_H