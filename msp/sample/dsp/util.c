/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ax_dsp_api.h>
#include <ax_base_type.h>
#include "util.h"
#include <ax_sys_api.h>

#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align)-1))

/* Load image file and alloc memory */
AX_BOOL LoadImage(const AX_CHAR *pszImge, AX_U64 *pPhyAddr, AX_VOID **ppVirAddr, AX_U32 *pImgSize)
{
    FILE* fp = fopen(pszImge, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        AX_U32 nFileSize = ftell(fp);
        if (pImgSize && *pImgSize > 0 && *pImgSize != nFileSize) {
             printf("%s: file size not right, %d != %d\n", __func__, *pImgSize, nFileSize);
             fclose(fp);
             return AX_FALSE;
        }
        fseek(fp, 0, SEEK_SET);
        AX_S32 ret = AX_SYS_MemAlloc(pPhyAddr, ppVirAddr, nFileSize, SAMPLE_PHY_MEM_ALIGN_SIZE, NULL);
        if (0 != ret) {
            printf("%s AX_SYS_MemAlloc fail, ret=0x%x\n", __func__, ret);
            fclose(fp);
            return AX_FALSE;
        }
        if (fread(*ppVirAddr, 1, nFileSize, fp) != nFileSize) {
            printf("%s fread fail, %s\n", __func__, strerror(errno));
            fclose(fp);
            return AX_FALSE;
        }
        fclose(fp);

        if (pImgSize) {
            *pImgSize = nFileSize;
        }
        return AX_TRUE;
    } else {
        printf("%s fopen %s fail, %s\n", __func__, pszImge,  strerror(errno));
        return AX_FALSE;
    }
}

static int Get_FileSize(const AX_CHAR *filename)
{
    struct stat statbuf;
    AX_S32 size;
    stat(filename, &statbuf);
    size = statbuf.st_size;
    return size;
}

AX_S32 DSP_LoadPic(const AX_CHAR *pszBinFileName, AX_VOID *vaddr)
{
    FILE *fp;
    AX_S32 fSz;
    AX_S32 ret;
    fSz = Get_FileSize(pszBinFileName);
    fp = fopen(pszBinFileName, "rb");
    if (!fp) {
        printf("DSP_LoadPic:open %s fail!\n", pszBinFileName);
        return AX_DSP_OPEN_FAIL;
    }
    ret = fread((void *)vaddr, fSz, 1, fp);
    if (ret != 1) {
        printf("DSP_LoadPic:ret = %x, fSz =  %x\n", ret, fSz);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return fSz;
}

AX_S32 DSP_SavePic(const char *pszBinFileName, AX_VOID *vaddr, AX_U32 size)
{
    FILE *fp;
    AX_S32 ret;
    fp = fopen(pszBinFileName, "wb+");
    if (!fp) {
        printf("DSP_SavePic:open %s fail!\n", pszBinFileName);
        return AX_DSP_OPEN_FAIL;
    }
    ret = fwrite((void *)vaddr, size, 1, fp);
    if (ret != 1) {
        printf("DSP_SavePic:ret = %x, fSz =  %x\n", ret, size);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return size;
}

