/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __COMMON_VDEC_UTILS_H__
#define __COMMON_VDEC_UTILS_H__


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
#include "common_vdec_log.h"

#include "libavcodec/codec_id.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"


#ifdef __cplusplus
extern "C" {
#endif

#define SAMPLE_VDEC_MAX_ARGS            (128)
#define SAMPLE_VDEC_OPTION_MAX_COUNT    (300)


#define AX_VDEC_FILE_DIR_LEN            (256)
#define AX_VDEC_FILE_NAME_LEN           (64)
#define AX_VDEC_FILE_PATH_LEN           (AX_VDEC_FILE_DIR_LEN + AX_VDEC_FILE_NAME_LEN)


#define USR_PIC_WIDTH                   (1920)
#define USR_PIC_HEIGHT                  (1080)

#define SEEK_NALU_MAX_LEN               (1024)
#define NAL_CODED_SLICE_CRA             (21)
#define NAL_CODED_SLICE_IDR             (5)
#define STREAM_BUFFER_MAX_SIZE           (3 * 1024 * 1024)
#define STREAM_BUFFER_MIN_SIZE           (1 * 1024 * 1024)
#define STREAM_BUFFER_MAX_SIZE_HIGH_RES  (100 * 1024 * 1024)


#define VDEC_BS_PARSER_BUF_SIZE         0x80000


#define SIZE_ALIGN(x,align) ((((x)+(align)-1)/(align))*(align))
#ifndef ALIGN_UP
#define ALIGN_UP(x, align)      (((x) + ((align) - 1)) & ~((align) - 1))
#endif

#define AX_SHIFT_LEFT_ALIGN(a) (1 << (a))



#define AX_VDEC_WIDTH_ALIGN     AX_SHIFT_LEFT_ALIGN(8)
#define AX_JDEC_WIDTH_ALIGN     AX_SHIFT_LEFT_ALIGN(6)

#define AX_VDEC_HEIGHT_ALIGN     AX_SHIFT_LEFT_ALIGN(6)

#define SAMPLE_VDEC_OUTPUT_FRAMEBUF_CNT     (8)
#define SAMPLE_VDEC_INPUT_FIFO_DEPTH        (32)
#define SAMPLE_VDEC_REF_BLK_CNT             (AX_VDEC_MAX_OUTPUT_FIFO_DEPTH)
#define SAMPLE_VDEC_FRAME_CNT               (1)
#define SAMPLE_VDEC_MAX_STREAM_CNT          AX_VDEC_MAX_GRP_NUM

#define AX_VDEC_LOSSY_YUV_LITE_WIDTH        128
#define AX_VDEC_LOSSY_YUV_LITE_HEIGHT       2
#define AX_VDEC_LOSSY_COMP_LEVEL_FACTOR     32

typedef struct {
    int num;
    int den;
} MPEG_RATIONAL_T;

typedef struct axSAMPLE_H264_SPS_DATA_T {
    uint16_t        width;
    uint16_t        height;
    MPEG_RATIONAL_T pixel_aspect;
    uint8_t         profile;
    uint8_t         level;
} SAMPLE_H264_SPS_DATA_T;

typedef enum axSAMPLE_BSBOUNDARY_TYPE_E {
    BSPARSER_NO_BOUNDARY = 0,
    BSPARSER_BOUNDARY = 1,
    BSPARSER_BOUNDARY_NON_SLICE_NAL = 2
} SAMPLE_BSBOUNDARY_TYPE_E;

typedef struct axSAMPLE_VDEC_USERPIC_T {
    AX_VDEC_USRPIC_T stUserPic;
    AX_BOOL recvStmAfUsrPic;
    AX_BOOL bAllChnGetUsrPic;
    AX_S32 s32RecvPicNumBak;
    AX_S32 usrPicChnEnaCnt;
    AX_BOOL usrPicGet[AX_DEC_MAX_CHN_NUM];
    AX_CHAR *pUsrPicFilePath[AX_DEC_MAX_CHN_NUM];
    FILE *fpUsrPic[AX_DEC_MAX_CHN_NUM];
    AX_POOL PoolId[AX_DEC_MAX_CHN_NUM];
    AX_POOL BlkId[AX_DEC_MAX_CHN_NUM];
} SAMPLE_VDEC_USERPIC_T;

typedef struct axSAMPLE_INPUT_FILE_INFO_T {
    FILE *fInput;
    size_t curPos;
    size_t sFileSize;
    AX_PAYLOAD_TYPE_E enDecType;
} SAMPLE_INPUT_FILE_INFO_T;

typedef struct axSAMPLE_BITSTREAM_INFO_T {
    AX_PAYLOAD_TYPE_E eVideoType;
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_U32 nFps;
    AX_VDEC_GRP VdGrp;
    SAMPLE_INPUT_FILE_INFO_T stBsInfo;
} SAMPLE_BITSTREAM_INFO_T;

typedef struct axSAMPLE_STREAM_BUF_T {
    AX_MEMORY_ADDR_T tBufAddr;
    AX_U32 uBufSize;
    AX_U8 *pBufBeforeFill;
    AX_U8 *pBufAfterFill;
    AX_BOOL bRingbuf;
} SAMPLE_STREAM_BUF_T;

typedef struct axSAMPLE_FFMPEG_T {
    AX_S32 s32VideoIndex;
    AVFormatContext *pstAvFmtCtx;
    AVBSFContext *pstAvBSFCtx;
    AVPacket *pstAvPkt;
} SAMPLE_FFMPEG_T;

typedef struct axSAMPLE_VDEC_CHN_CFG_T {
    AX_VDEC_CHN VdChn;
    AX_BOOL bChnEnable;
    AX_U32 u32PicWidth;                 /* Width of scaler or crop target image */
    AX_U32 u32PicHeight;                /* Height of scaler or crop target image */
    AX_U32 u32FrameStride;
    AX_U32 u32FramePadding;
    AX_U32 u32CropX;
    AX_U32 u32CropY;
    AX_U32 u32ScaleRatioX;
    AX_U32 u32ScaleRatioY;
    AX_U32 uCompressLevel;
    AX_U32 u32OutputFifoDepth;
    AX_U32 u32FrameBufCnt;              /* frame buffer number, valid if bSdkAutoFramePool is TRUE */
    AX_VDEC_OUTPUT_MODE_E enOutputMode;
    AX_IMG_FORMAT_E enImgFormat;        /* Pixel format of target image */
    AX_CHAR *pOutputFilePath;
    AX_CHAR *pUsrPicFilePath;
    AX_BOOL bUserPicEnable;
    AX_F32  f32DstFrmRate;
    AX_BOOL bFrmRateCtrl;
} SAMPLE_VDEC_CHN_CFG_T;

typedef enum {
    AX_VDEC_MULTI_MODE_DISABLE = 0,
    AX_VDEC_MULTI_MODE_THREAD,
    AX_VDEC_MULTI_MODE_PROCESS,
    AX_VDEC_MULTI_MODE_BUTT
} AX_VDEC_MULTI_MODE_E;

typedef enum {
    AX_VDEC_SELECT_MODE_DISABLE = 0,
    AX_VDEC_SELECT_MODE_PRIVATE,
    AX_VDEC_SELECT_MODE_POSIX,
    AX_VDEC_SELECT_MODE_BUTT
} AX_VDEC_SELECT_MODE_E;


typedef enum {
    AX_VDEC_GRP_UNEXIST = 0,
    AX_VDEC_GRP_CREATED,
    AX_VDEC_GRP_START_RECV,
    AX_VDEC_GRP_STOP_RECV,
    AX_VDEC_GRP_RESET,
    AX_VDEC_GRP_DESTROYED,
    AX_VDEC_GRP_BUTT
} AX_VDEC_GRP_STATUS_E;

typedef struct axSAMPLE_VDEC_CMD_PARAM_T {
    AX_U32 uGrpCount;
    AX_U32 uStreamCount;
    AX_S32 sLoopDecNum;
    AX_S32 sRecvPicNum;
    AX_S32 sMilliSec;
    AX_U32 u32MaxPicWidth;
    AX_U32 u32MaxPicHeight;
    AX_S32 sWriteFrames;
    AX_S32 DestMD5;
    AX_U32 uMaxGrpCnt;
    AX_U32 uStartGrpId;
    AX_S32 sStreamSize;
    AX_BOOL bJpegDecOneFrm;
    AX_F32 f32SrcFrmRate;
    AX_VDEC_DISPLAY_MODE_E enDisplayMode;
    AX_VDEC_MULTI_MODE_E enMultimode;
    AX_VDEC_SELECT_MODE_E enSelectMode;
    AX_POOL_SOURCE_E enFrameBufSrc;
    AX_PAYLOAD_TYPE_E enDecType;
    AX_VDEC_INPUT_MODE_E enInputMode;
    AX_VDEC_ENABLE_MOD_E enDecModule;
    AX_CHAR *pInputFilePath;
    SAMPLE_VDEC_CHN_CFG_T tChnCfg[AX_DEC_MAX_CHN_NUM];
    AX_CHAR *pTbCfgFilePath;
    AX_CHAR *pGrpCmdlFile[AX_VDEC_MAX_GRP_NUM];
    AX_BOOL bQuitWait;
    AX_BOOL highRes;
    AX_BOOL bPerfTest;
    AX_BOOL bGetUserData;
    AX_BOOL bGetRbInfo;
    AX_BOOL bQueryStatus;
    AX_BOOL bGetVuiParam;
    AX_BOOL bUsrInstant;
    AX_U32 usrPicIdx;
    AX_BOOL recvStmAfUsrPic;
    AX_BOOL bFfmpegEnable;
    AX_BOOL bDynRes;
    AX_CHAR *pNewInputFilePath;

    /* For sample_vdec_ivps_vo. */
    AX_BOOL pollingEna;
    AX_S32 pollingCnt;
    AX_U32 pollingTime;
    AX_U32 waitTime;
    AX_VDEC_MODE_E enVideoMode;
    AX_VDEC_OUTPUT_ORDER_E enOutputOrder;
} SAMPLE_VDEC_CMD_PARAM_T;

typedef struct axSAMPLE_VDEC_TBCFG_PARAM_T {
    AX_U32 uStreamCount;
    AX_U32 sLoopDecNum;
    AX_S32 sRecvPicNum;
    AX_S32 sMilliSec;
    AX_U32 u32MaxPicWidth;
    AX_U32 u32MaxPicHeight;
    AX_S32 sWriteFrames;
    AX_S32 DestMD5;
    AX_U32 uMaxGrpCnt;
    AX_U32 uStartGrpId;
    AX_BOOL bJpegDecOneFrm;
    AX_F32 f32SrcFrmRate;
    AX_VDEC_DISPLAY_MODE_E enDisplayMode;
    AX_VDEC_MULTI_MODE_E enMultimode;
    AX_VDEC_SELECT_MODE_E enSelectMode;
    AX_POOL_SOURCE_E enFrameBufSrc;
    AX_BOOL bUserPicEnable;
    AX_PAYLOAD_TYPE_E enDecType;
    AX_VDEC_INPUT_MODE_E enInputMode;
    AX_VDEC_ENABLE_MOD_E enDecModule;
    AX_CHAR *pInputFilePath;
    SAMPLE_VDEC_CHN_CFG_T tChnCfg[AX_DEC_MAX_CHN_NUM];
} SAMPLE_VDEC_TBCFG_PARAM_T;

typedef struct axSAMPLE_VDEC_OUTPUT_INFO_T {
    AX_VDEC_GRP         VdGrp;
    AX_VDEC_CHN         VdChn;
    AX_U32              u32Width;
    AX_U32              u32Height;
    AX_U32              u32PicStride;
    AX_IMG_FORMAT_E     enImgFormat;
    AX_U32              u32CompressLevel;
    AX_BOOL             bOneShot;
} SAMPLE_VDEC_OUTPUT_INFO_T;


typedef struct axSAMPLE_VDEC_CONTEXT_T {
    SAMPLE_VDEC_CMD_PARAM_T tCmdParam;
    SAMPLE_VDEC_TBCFG_PARAM_T tTbCfg;
    AX_BOOL bTbCfgEnable;
    struct timeval Timebegin;
    struct timeval Timeend;
    AX_U64 u64SelectFrameCnt;
    int argc;
    char **argv;
    AX_BOOL bArgvAlloc;

    SAMPLE_FFMPEG_T stFfmpeg;
    SAMPLE_BITSTREAM_INFO_T stBitStreamInfo;

    FILE *pInputFd[AX_VDEC_MAX_GRP_NUM];
    FILE *pNewInputFd[AX_VDEC_MAX_GRP_NUM];
    off_t oInputFileSize[AX_VDEC_MAX_GRP_NUM];
    off_t oNewInputFileSize[AX_VDEC_MAX_GRP_NUM];

    AX_BOOL bGrpQuitWait[AX_VDEC_MAX_GRP_NUM];
    AX_BOOL bRecvFlowEnd;
    FILE *pOutputFd[AX_DEC_MAX_CHN_NUM];
    AX_CHAR *pOutputFilePath[AX_DEC_MAX_CHN_NUM];
    AX_CHAR *pOutputFilePath1[AX_DEC_MAX_CHN_NUM];
    SAMPLE_VDEC_OUTPUT_INFO_T outInfo[AX_DEC_MAX_CHN_NUM];

    AX_VDEC_GRP_STATUS_E GrpStatus[AX_VDEC_MAX_GRP_NUM];
    pthread_mutex_t GrpTidMutex[AX_VDEC_MAX_GRP_NUM];
    pthread_mutex_t RecvTidMutex[AX_VDEC_MAX_GRP_NUM][AX_DEC_MAX_CHN_NUM];
    AX_S32 GrpPID[AX_VDEC_MAX_GRP_NUM];
    pthread_t GrpTid[AX_VDEC_MAX_GRP_NUM];
    pthread_t GrpChnRecvTid[AX_VDEC_MAX_GRP_NUM][AX_DEC_MAX_CHN_NUM];
    pthread_t RecvTid;
    AX_BOOL RecvThdWait;
    SAMPLE_VDEC_USERPIC_T stVdecUserPic;

#ifdef AX_VDEC_POOL_REFCNT_TEST
    AX_BLK blkRef[AX_VDEC_MAX_GRP_NUM][SAMPLE_VDEC_REF_BLK_CNT];
#endif
} SAMPLE_VDEC_CONTEXT_T;

typedef struct axSAMPLE_VDEC_USRPIC_CHN_PARAM_T {
    AX_BOOL bChnEnable;
    AX_U32 u32PicWidth;                 /* Width of scaler or crop target image */
    AX_U32 u32PicHeight;                /* Height of scaler or crop target image */
    AX_IMG_FORMAT_E enImgFormat;        /* Pixel format of target image */
    AX_CHAR *pUsrPicFilePath;
    AX_BOOL bUserPicEnable;
} SAMPLE_VDEC_USRPIC_CHN_PARAM_T;
typedef struct axSAMPLE_VDEC_USRPIC_ARGS_T {
    AX_VDEC_GRP VdGrp;
    AX_BOOL bUsrInstant;
    AX_PAYLOAD_TYPE_E enDecType;
    SAMPLE_VDEC_USERPIC_T *pstVdecUserPic;
    SAMPLE_VDEC_USRPIC_CHN_PARAM_T tChnParam[AX_DEC_MAX_CHN_NUM];
} SAMPLE_VDEC_USRPIC_ARGS_T;

typedef struct axSAMPLE_VDEC_ARGS_T {
    AX_VDEC_GRP VdGrp;
    AX_VDEC_GRP_ATTR_T tVdGrpAttr;
    AX_POOL_CONFIG_T tPoolConfig[AX_DEC_MAX_CHN_NUM];
    AX_POOL PoolId[AX_DEC_MAX_CHN_NUM];
    AX_VDEC_CHN_ATTR_T *pstVdChnAttr[AX_DEC_MAX_CHN_NUM];
    AX_BOOL bChnEnable[AX_DEC_MAX_CHN_NUM];
    SAMPLE_VDEC_CONTEXT_T *pstCtx;
    SAMPLE_VDEC_USRPIC_ARGS_T tUsrPicArgs;
} SAMPLE_VDEC_ARGS_T;

typedef struct {
    AX_VDEC_GRP VdGrp;
    AX_VDEC_CHN VdChn;
    SAMPLE_VDEC_CONTEXT_T *pstCtx;
} SAMPLE_VDEC_GRP_CHN_RECV_ARGS_T;

typedef struct {
    SAMPLE_VDEC_ARGS_T stVdecGrpArgs[AX_VDEC_MAX_GRP_NUM];
} SAMPLE_VDEC_GRP_SET_ARGS_T;

const char *AX_VdecRetStr(AX_S32 rv);





#ifdef __cplusplus
}
#endif
#endif
