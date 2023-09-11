/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __COMMON_VDEC_LOG_H__
#define __COMMON_VDEC_LOG_H__


#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#ifdef __linux
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#endif

#include "ax_vdec_type.h"
#include "ax_sys_log.h"


#ifdef __cplusplus
extern "C" {
#endif



#ifndef gettid
#define gettid() syscall(__NR_gettid)
#endif


#define AX_SAMPLE_VDEC_LOG_TAG "sample_vdec"

#define SAMPLE_DBG_LOG(str, arg...)  do { \
        AX_SYS_LogPrint_Ex(SYS_LOG_DEBUG, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_WHITE"[SAMPLE][AX_VDEC][tid:%ld][DEBUG][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_DBG_LOG(str, arg...)  do { \
        AX_SYS_LogPrint_Ex(SYS_LOG_DEBUG, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_WHITE"[SAMPLE][AX_VDEC][tid:%ld][DEBUG][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_LOG(str, arg...)  do { \
        AX_SYS_LogPrint_Ex(SYS_LOG_INFO, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_WHITE"[SAMPLE][AX_VDEC][tid:%ld][INFO][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_NOTICE_LOG(str, arg...)  do { \
        AX_SYS_LogPrint_Ex(SYS_LOG_NOTICE, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_BLUE"[SAMPLE][AX_VDEC][tid:%ld][NOTICE][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_WARN_LOG(str, arg...)  do { \
        AX_SYS_LogPrint_Ex(SYS_LOG_WARN, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_PURPLE"[SAMPLE][AX_VDEC][tid:%ld][WARN][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_ERR_LOG(str, arg...)  do { \
        printf("\n"MACRO_YELLOW"[SAMPLE][AX_VDEC][tid:%ld][ERROR][%s][line:%d]" str "\n", \
                gettid(), __func__, __LINE__, ##arg); \
        AX_SYS_LogPrint_Ex(SYS_LOG_ERROR, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_YELLOW"[SAMPLE][AX_VDEC][tid:%ld][ERROR][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_CRIT_LOG(str, arg...)        do{ \
        printf("\n"MACRO_RED"[SAMPLE][AX_VDEC][tid:%ld][CR_ERROR][%s][line:%d]" str "\n", \
                gettid(), __func__, __LINE__, ##arg); \
        AX_SYS_LogPrint_Ex(SYS_LOG_CRITICAL, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_RED"[SAMPLE][AX_VDEC][tid:%ld][CR_ERROR][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_LOG_T(str, arg...)  do { \
        printf("\n" "[SAMPLE][AX_VDEC][tid:%ld][T][%s][line:%d]: " str "\n", \
               gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_LOG_HIT(str, arg...)  do { \
        AX_SYS_LogPrint_Ex(SYS_LOG_ERROR, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_PURPLE"[SAMPLE][AX_VDEC][tid:%ld][T][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_LOG_ST(str, arg...)  do { \
        printf("\n" "[SAMPLE][AX_VDEC][tid:%ld][T][%s][line:%d]: " str "\n", \
               gettid(), __func__, __LINE__, ##arg); \
        AX_SYS_LogPrint_Ex(SYS_LOG_ERROR, AX_SAMPLE_VDEC_LOG_TAG, AX_ID_USER, \
                           MACRO_PURPLE"[SAMPLE][AX_VDEC][tid:%ld][T][%s][line:%d]: " str "\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_LOG_N(str, arg...)           do{}while(0)
#define SAMPLE_ERR_LOG_N(str, arg...)       do{}while(0)
#define SAMPLE_CRIT_LOG_N(str, arg...)      do{}while(0)


#define AX_VIDEO_FRAME_INFO_T_Print_all(VdGrp, VdChn, pstFrameInfo) \
do { \
    const AX_VIDEO_FRAME_T *pstVFrame = &(((AX_VIDEO_FRAME_INFO_T *)pstFrameInfo)->stVFrame); \
    if (pstVFrame->u64PhyAddr[0] != 0) { \
        SAMPLE_LOG("VdGrp=%d, VdChn=%d, ->enModId:%d, ->bEndOfStream:%d", \
                    VdGrp, VdChn, \
                    ((AX_VIDEO_FRAME_INFO_T *)pstFrameInfo)->enModId, ((AX_VIDEO_FRAME_INFO_T *)pstFrameInfo)->bEndOfStream); \
        SAMPLE_LOG("pstVFrame->u32Width:0x%x", pstVFrame->u32Width); \
        SAMPLE_LOG("pstVFrame->u32Height:0x%x", pstVFrame->u32Height); \
        SAMPLE_LOG("pstVFrame->enImgFormat:0x%x", pstVFrame->enImgFormat); \
        SAMPLE_LOG("pstVFrame->enVscanFormat:0x%x", pstVFrame->enVscanFormat); \
        SAMPLE_LOG("pstVFrame->stCompressInfo.enCompressMode:0x%x", pstVFrame->stCompressInfo.enCompressMode); \
        SAMPLE_LOG("pstVFrame->stCompressInfo.u32CompressLevel:0x%x", pstVFrame->stCompressInfo.u32CompressLevel); \
        SAMPLE_LOG("pstVFrame->stDynamicRange:0x%x", pstVFrame->stDynamicRange); \
        SAMPLE_LOG("pstVFrame->stColorGamut:0x%x", pstVFrame->stColorGamut); \
        for (int i = 0; i < AX_MAX_COLOR_COMPONENT; i++) { \
            SAMPLE_LOG("pstVFrame->u32PicStride[%d]:0x%x", i, pstVFrame->u32PicStride[i]); \
            SAMPLE_LOG("pstVFrame->u32ExtStride[%d]:0x%x", i, pstVFrame->u32ExtStride[i]); \
            SAMPLE_LOG("pstVFrame->u64PhyAddr[%d]:0x%llx", i, pstVFrame->u64PhyAddr[i]);     \
            SAMPLE_LOG("pstVFrame->u64VirAddr[%d]:0x%llx", i, pstVFrame->u64VirAddr[i]);     \
            SAMPLE_LOG("pstVFrame->u64ExtPhyAddr[%d]:0x%llx", i, pstVFrame->u64ExtPhyAddr[i]); \
            SAMPLE_LOG("pstVFrame->u64ExtVirAddr[%d]:0x%llx", i, pstVFrame->u64ExtVirAddr[i]); \
            SAMPLE_LOG("pstVFrame->u32BlkId[%d]:0x%x", i, pstVFrame->u32BlkId[i]); \
        } \
        SAMPLE_LOG("pstVFrame->s16CropX:0x%x", pstVFrame->s16CropX); \
        SAMPLE_LOG("pstVFrame->s16CropY:0x%x", pstVFrame->s16CropY); \
        SAMPLE_LOG("pstVFrame->s16CropWidth:0x%x", pstVFrame->s16CropWidth); \
        SAMPLE_LOG("pstVFrame->s16CropHeight:0x%x", pstVFrame->s16CropHeight); \
        SAMPLE_LOG("pstVFrame->u32TimeRef:0x%x", pstVFrame->u32TimeRef); \
        SAMPLE_LOG("pstVFrame->u64PTS:%lld", pstVFrame->u64PTS);           \
        SAMPLE_LOG("pstVFrame->u64SeqNum:0x%llx", pstVFrame->u64SeqNum);     \
        SAMPLE_LOG("pstVFrame->u64UserData:0x%llx", pstVFrame->u64UserData); \
        SAMPLE_LOG("pstVFrame->u64PrivateData:0x%llx", pstVFrame->u64PrivateData);  \
        SAMPLE_LOG("pstVFrame->u32FrameFlag:0x%x", pstVFrame->u32FrameFlag);      \
        SAMPLE_LOG("pstVFrame->u32FrameSize:0x%x", pstVFrame->u32FrameSize);      \
    } else { \
        /* SAMPLE_LOG("pstVFrame->u64PhyAddr[%d]:0x%lx", 0, pstVFrame->u64PhyAddr[0]);  */  \
    } \
} while(0)


#define AX_VIDEO_FRAME_INFO_T_Print_brief(VdGrp, VdChn, pstFrameInfo) \
do { \
    const AX_VIDEO_FRAME_T *pstVFrame = &(((AX_VIDEO_FRAME_INFO_T *)pstFrameInfo)->stVFrame); \
    if (pstVFrame->u64PhyAddr[0] != 0) { \
        SAMPLE_LOG("VdGrp=%d, VdChn=%d, ->enModId:%d, ->bEndOfStream:%d", \
                    VdGrp, VdChn, \
                    ((AX_VIDEO_FRAME_INFO_T *)pstFrameInfo)->enModId, ((AX_VIDEO_FRAME_INFO_T *)pstFrameInfo)->bEndOfStream); \
        SAMPLE_LOG("pstVFrame->u32Width:0x%x", pstVFrame->u32Width); \
        SAMPLE_LOG("pstVFrame->u32Height:0x%x", pstVFrame->u32Height); \
        SAMPLE_LOG("pstVFrame->enImgFormat:0x%x", pstVFrame->enImgFormat); \
        for (int i = 0; i < AX_MAX_COLOR_COMPONENT; i++) { \
            SAMPLE_LOG("pstVFrame->u32PicStride[%d]:0x%x", i, pstVFrame->u32PicStride[i]); \
            SAMPLE_LOG("pstVFrame->u32ExtStride[%d]:0x%x", i, pstVFrame->u32ExtStride[i]); \
            SAMPLE_LOG("pstVFrame->u64PhyAddr[%d]:0x%llx", i, pstVFrame->u64PhyAddr[i]);     \
            SAMPLE_LOG("pstVFrame->u64VirAddr[%d]:0x%llx", i, pstVFrame->u64VirAddr[i]);     \
            SAMPLE_LOG("pstVFrame->u64ExtPhyAddr[%d]:0x%llx", i, pstVFrame->u64ExtPhyAddr[i]); \
            SAMPLE_LOG("pstVFrame->u64ExtVirAddr[%d]:0x%llx", i, pstVFrame->u64ExtVirAddr[i]); \
            SAMPLE_LOG("pstVFrame->u32BlkId[%d]:0x%x", i, pstVFrame->u32BlkId[i]); \
        } \
        SAMPLE_LOG("pstVFrame->u64PTS:%lld", pstVFrame->u64PTS);           \
    } else { \
        /* SAMPLE_LOG("pstVFrame->u64PhyAddr[%d]:0x%lx", 0, pstVFrame->u64PhyAddr[0]);  */  \
    } \
} while(0)




#ifdef __cplusplus
}
#endif
#endif
