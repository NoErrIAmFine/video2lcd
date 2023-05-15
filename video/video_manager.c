#include <string.h>

#include "config.h"
#include "video_manager.h"


static PT_VideoOpr g_ptVideoOprHead = NULL;

int RegisterVideoOpr(PT_VideoOpr ptVideoOpr)
{
    PT_VideoOpr ptTmp;

    if(!g_ptVideoOprHead)
    {
        g_ptVideoOprHead = ptVideoOpr;
        ptVideoOpr->ptNext = NULL;
    }
    else
    {
        ptTmp = g_ptVideoOprHead;
        while(ptTmp->ptNext)
        {
            ptTmp = ptTmp->ptNext;
        }
        ptTmp->ptNext = ptVideoOpr;
        ptVideoOpr->ptNext = NULL;
    }
    return 0;
}

void ShowVideoOpr(void)
{
    int i = 0;
    PT_VideoOpr ptTmp;

    ptTmp = g_ptVideoOprHead;
    while(ptTmp)
    {
        printf("%02d %s\n",i++,ptTmp->name);
        ptTmp = ptTmp->ptNext;
    }
}

PT_VideoOpr GetVideoOpr(char *pcName)
{
    PT_VideoOpr ptTmp;

    ptTmp = g_ptVideoOprHead;
    while(ptTmp)
    {
        if(!strcmp(pcName,ptTmp->name))
            return ptTmp;
        ptTmp = ptTmp->ptNext;
    }
    return NULL;
}

int VideoDeviceInit(const char *pcTypeName,const char *pcDevName,PT_VideoDevice ptVideoDevice)
{
    int iret;
    PT_VideoOpr ptVdieoOpr = g_ptVideoOprHead;

    /* 根据pcTypeName,找到相应的PT_VideoOpr */
    while(ptVdieoOpr){
        if(!strcmp(ptVdieoOpr->name,pcTypeName)){
            iret = ptVdieoOpr->InitDevice(pcDevName,ptVideoDevice);
            return iret;
        }
        ptVdieoOpr = ptVdieoOpr->ptNext;
    }
    return -1;
}

int VideoInit(void)
{
    int iError;

    iError = V4l2Init();

    return iError;
}