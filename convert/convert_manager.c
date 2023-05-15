#include <string.h>
#include <convert_manager.h>

static PT_VideoConvertOpr g_ptVideoConvertOprHead;

int RegisterVideoConvertOpr(PT_VideoConvertOpr ptVideoConvertOpr)
{
    PT_VideoConvertOpr ptTmp;

    if(!g_ptVideoConvertOprHead)
    {
        g_ptVideoConvertOprHead = ptVideoConvertOpr;
        ptVideoConvertOpr->ptNext = NULL;
    }else{
        ptTmp = g_ptVideoConvertOprHead;
        while(ptTmp->ptNext){
            ptTmp = ptTmp->ptNext;
        }
        ptTmp->ptNext = ptVideoConvertOpr;
        ptVideoConvertOpr->ptNext = NULL;
    }
    return 0;
}

int UnregisterVideoConvertOpr(PT_VideoConvertOpr ptVideoConvertOpr)
{
    struct VideoConvertOpr **ptNext;

    ptNext = &ptVideoConvertOpr->ptNext;

    while(*ptNext){
        if(*ptNext == ptVideoConvertOpr){
            *ptNext = (*ptNext)->ptNext;
            return 0;
        }
        ptNext = &(*ptNext)->ptNext;
    }
    return 0;
}

void ShowVideoConvertOpr(void)
{
    int i = 0;
    PT_VideoConvertOpr ptTmp;

    ptTmp = g_ptVideoConvertOprHead;
    while(ptTmp){
        printf("%02d %s\n",i++,ptTmp->name);
        ptTmp = ptTmp->ptNext;
    }
}

PT_VideoConvertOpr GetVideoConvertOpr(char *pcName)
{
    PT_VideoConvertOpr ptTmp;

    ptTmp = g_ptVideoConvertOprHead;
    while(ptTmp)
    {
        if(!strcmp(pcName,ptTmp->name))
            return ptTmp;
        ptTmp = ptTmp->ptNext;
    }
    return NULL;
}

PT_VideoConvertOpr GetVideoConvertForFormat(int iPixelFormatIn,int iPixelFormatOut)
{
    PT_VideoConvertOpr ptTmp = g_ptVideoConvertOprHead;

    while(ptTmp){
        if(ptTmp->isSupport(iPixelFormatIn,iPixelFormatOut)){
            return ptTmp;
        }
        ptTmp = ptTmp->ptNext;
    }
    return NULL;
}

int VideoConvertInit(void)
{
    int iError;

    iError = Yuv2RgbInit();
    iError |= Mjpeg2RgbInit();
    iError |= Rgb2RgbInit();

    return iError;
}