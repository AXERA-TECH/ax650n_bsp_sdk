/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __COMMON_VENC_H__
#define __COMMON_VENC_H__

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "ax_global_type.h"
#include "ax_venc_comm.h"
#include "common_venc_log.h"

#define MAX_TEST_CASE_NAME_SIZE (30)
#define MAX_DIE_FILE_SIZE       (50)
#define MAX_STREAM_SUFFIX       (20)

#define VENC_FBC_8BIT     (8)
#define VENC_FBC_10BIT    (10)
#define VENC_STREAM_8BIT  (8)
#define VENC_STREAM_10BIT (10)
#define VENC_FBC_LOSSLESS (1)
#define VENC_FBC_LOSSY    (2)

#define TILE_ALIGN(var, align) ((var + align - 1) & (~(align - 1)))

typedef AX_S32 (*TestFunction)(AX_S32 VeChn, AX_VOID *handle);
typedef enum
{
    SAMPLE_CODEC_H264 = 0,
    SAMPLE_CODEC_H265 = 1,
    SAMPLE_CODEC_MJPEG = 2,
    SAMPLE_CODEC_JPEG = 3
} SAMPLE_VENC_CODEC_TYPE_E;

typedef enum
{
    SAMPLE_RC_CBR = 0,
    SAMPLE_RC_VBR = 1,
    SAMPLE_RC_AVBR = 2,
    SAMPLE_RC_QPMAP = 3,
    SAMPLE_RC_FIXQP = 4,
    SAMPLE_RC_CVBR = 5,
} SAMPLE_VENC_RC_E;

typedef struct axSAMPLE_VENC_FBC_INFO_T
{
    /* fbc format */
    AX_U32 fbcType; /* 0: not fbc 1: lossless 2: lossy */
    AX_U32 compLevel;
    AX_U32 bitDepth;
    AX_U32 yHdrSize;
    AX_U32 yPadSize;
    AX_U32 uvHdrSize;
    AX_U32 uvPadSize;
} SAMPLE_VENC_FBC_INFO_T;

typedef struct axSAMPLE_VENC_CMD_PARA_T
{
    AX_BOOL bNormal;
    /* common control params */
    AX_CHAR *input;
    AX_CHAR *output;
    AX_BOOL bLoopEncode; /* rewind the input file when get EOF */
    AX_U32 encFrameNum;  /* how many frames want to encode */
    AX_U32 picFormat;    /* 1: I420; 3: NV12; 4: NV21; 13: YUYV422; 14: UYVY422 */
    AX_BOOL logLevel;
    AX_BOOL bSaveStrm;
    AX_U32 strideY;
    AX_U32 strideU;
    AX_U32 strideV;
    AX_S32 syncType; /* -1: block; 0: non-block; x(>0): wait for x milliSec */
    AX_S32 startQp;  /* start qp for first IDR */

    /* common channel params */
    AX_U32 chnNum; /* encode channel number */
    /* total encode thread number */
    AX_U32 encThdNum;

    /* only bChnCustom is true, below params can be available */
    AX_BOOL bChnCustom; /* force all channel use the same codec type; 0: disable 1: enable */
    AX_U16 codecType;   /* 0: h.264; 1: h.265; 2: jpeg; 3: mjpeg.*/
    /* End of only bChnCustom is true, below params can be available */

    AX_U32 picW;    /* input frame width */
    AX_U32 picH;    /* input frame height */
    AX_U32 maxPicW; /* max frame width */
    AX_U32 maxPicH; /* max frame height */

    AX_BOOL bCrop; /* whether enable crop, 0: disable; 1: enable.*/
    AX_U32 cropX;  /* horizontal cropping offset */
    AX_U32 cropY;  /* vertical cropping offset */
    AX_U32 cropW;  /* width of encoded image */
    AX_U32 cropH;  /* height of encoded image */

    AX_BOOL bLinkMode;  /* link mode, 0: UnLink; 1: Link with ISP and so on */
    AX_U32 strmBufSize; /* stream buffer size */
    /* End of common channel params */

    /* rate control params */
    AX_U32 rcMode;    /*0: CBR; 1: VBR; 2: AVBR 3: QPMAP 4: FIXQP 5: CVBR. */
    AX_U32 rcModeNew; /*0: CBR; 1: VBR; 2: AVBR 3: QPMAP 4: FIXQP 5: CVBR. For change rcMode dynamically. */

    AX_U32 gopLen;
    AX_U32 virILen; /* virtual I frame interval */
    AX_F32 srcFrameRate;
    AX_F32 dstFrameRate;
    AX_U32 bitRate; /* in kbps */
    AX_U32 qpMin;
    AX_U32 qpMax;
    AX_U32 qpMinI;
    AX_U32 qpMaxI;
    AX_U32 maxIprop; /* Range:[1, 100]; the max I P size ratio */
    AX_U32 minIprop; /* Range:[1, u32MaxIprop]; the min I P size ratio */
    AX_U32 IQp;      /* RW; Range:[0, 51]; qp of the i frame */
    AX_U32 PQp;      /* RW; Range:[0, 51]; qp of the p frame */
    AX_U32 BQp;      /* RW; Range:[0, 51]; qp of the b frame */
    /* cvbr params */
    AX_U32 ltMaxBt;    /* Range:[2, 614400];the long-term target max bitrate, can not be larger than
                                            u32MaxBitRate,the unit is kbps */
    AX_U32 ltMinBt;    /* Range:[0, 614400];the long-term target min bitrate,  can not be larger than
                                            ltMaxBt,the unit is kbps */
    AX_U32 ltStaTime;  /* Range:[1, 1440]; the long-term rate statistic time, the unit is
                                          u32LongTermStatTimeUnit*/
    AX_U32 shtStaTime; /* Range:[1, 120]; the short-term rate statistic time, the unit is second (s)*/
    AX_U32 minQpDelta; /* Difference between FrameLevelMinQp and MinQp */
    AX_U32 maxQpDelta; /* Difference between FrameLevelMaxQp and MaxQp */
    /* end cvbr params */

    /*
     * Range:[-1, 51]; Fixed qp for every frame.
     * -1: disable fixed qp mode.
     * [0, 51]: value of fixed qp.
     */
    AX_S32 fixedQp;
    AX_U32 IQpDelta; /* Range:[-51, 51]; QP difference between target QP and intra frame QP */

    AX_U16 ctbRcMode; /* 0: diable ctbRc; 1: quality first ctbRc; 2: bitrate first ctbRc 3: quality and bitrate balance
                         ctbRc */
    AX_U16 qpMapQpType;  /* 0: disable qpmap; 1: deltaQp; 2: absQp */
    AX_U16 qpMapBlkType; /* 0: disable; 1: skip mode; 2: Ipcm mode */
    AX_U16 qpMapBlkUnit; /* 0: 64x64; 1: 32x32; 2: 16x16.*/
    /* End of rate control params */

    /* gop params */
    AX_U16 gopMode; /*0: normalP; 1: oneLTR; 2: svc-t */

    /* filter the bit streams at different layer by temporalID in svc-t mode.
     * 0 : save the 0 layer bit stream.
     * 1 : save the 0 and 1 layer bit stream.
     * 2 : save the 0, 1 and 2 layer bit stream.
     */
    AX_U16 temporalID;

    AX_U32 qFactor;
    AX_BOOL enableEncodeOnce;
    AX_BOOL roiEnable;
    AX_CHAR *vencRoiMap;
    AX_CHAR *jencRoiMap;
    AX_U32 qRoiFactor;
    AX_BOOL bDynProfileLevel;
    /* frame index to do dynamic params setting */
    AX_U32 dynAttrIdx;
    /* ut case id */
    AX_U32 ut;
    AX_S32 defaultUt;
    TestFunction function;
    AX_U32 grpId;

    /* pool related variable */
    AX_S32 poolId;
    AX_S32 poolIdDynRes; /* for dynamic resolution */
    AX_S32 frameSize;
    AX_S32 BlkSize;
    /* qpmap pool */
    AX_S32 qpMapSize;
    AX_S32 maxCuSize; /* h264: 16; h265: 64 */
    /* fbc format */
    AX_U32 fbcType; /* 0: not fbc 1: lossless 2: lossy */
    AX_U32 compLevel;
    AX_U32 bitDepth;
    AX_U32 yHdrSize;
    AX_U32 yPadSize;
    AX_U32 uvHdrSize;
    AX_U32 uvPadSize;

    AX_U32 lumaSize;
    AX_U32 chromaSize;
    AX_U32 refreshNum; /* how many p frame to do intra refresh in one gop,
                            only NormalP support gdr */
    AX_BOOL bGetStrmBufInfo;
    AX_BOOL bQueryStatus;

    /* for dynamic resolution test case */
    AX_BOOL bDynRes; /* enable change resolution */
    AX_CHAR *newInput;
    AX_U32 newPicW; /* input frame width */
    AX_U32 newPicH; /* input frame height */
    /* end for dynamic resolution test case */
    AX_BOOL bInsertIDR;
    /* rate jam */
    AX_U32 drpFrmMode; /* 0: normal; 1: p-skip */
    AX_U32 encFrmGap;  /* 0: continue to do rate jam strategy; n(n>0): do rate jam strategy for n times */
    AX_U32 frmThrBps;  /* instant bit rate, bps */
    /* super frame */
    AX_U32 pri;  /* 0: framebits first; 1: bitrate first */
    AX_U32 thrI; /* threshold of the super I frame */
    AX_U32 thrP; /* threshold of the super P frame */
    /* slice split */
    AX_U32 sliceNum;
    /* output stream suffix */
    AX_CHAR strmSuffix[MAX_STREAM_SUFFIX];
    /* user data size */
    AX_U32 uDataSize;
    /* for perf */
    AX_U32 vbCnt;
    AX_BOOL bPerf; /* test encode performance */
    AX_U64 vencLoopExit;
    /* fifo depth */
    AX_U8 inFifoDep;
    AX_U8 outFifoDep;
    AX_BOOL bCoreCoWork;
    AX_U8 strmBitDep; /* output bit depth of stream */
    /* VUI */
    AX_BOOL bSignalPresent;
    AX_U8 videoFormat;
    AX_BOOL bFullRange;
    AX_BOOL bColorPresent;
    AX_U8 colorPrimaries;
    AX_U8 transferCharacter;
    AX_U8 matrixCoeffs;
} SAMPLE_VENC_CMD_PARA_T;

typedef struct axSAMPLE_VENC_SENDFRAME_PARA_T
{
    AX_BOOL bSendFrmStart;
    VENC_CHN VeChn;
    AX_BOOL bLoopEncode;
    AX_U32 encFrmNum;
    AX_S32 syncType;

    const AX_CHAR *fileInput;
    AX_U32 width;
    AX_U32 height;
    AX_U32 strideY;
    AX_U32 strideU;
    AX_U32 strideV;
    AX_IMG_FORMAT_E eFmt;
    AX_S32 poolId;
    AX_S32 frameSize;
    AX_S32 blkSize;
    AX_U32 lumaSize;
    AX_U32 chromaSize;
    FILE *fFileIn;
    AX_VOID *ptrPrivate;

    pthread_t sendFrmPid;

    AX_U32 dynAttrIdx;
    AX_S32 ut;
    TestFunction function;

    SAMPLE_VENC_FBC_INFO_T stFbcInfo;

    AX_U16 qpMapQpType;  /* 0: disable qpmap; 1: deltaQp; 2: absQp */
    AX_U16 qpMapBlkType; /* 0: disable block mode; 1: skip mode; 2: Ipcm mode */
    AX_U16 qpMapBlkUnit; /* 0: 64x64; 1: 32x32; 2: 16x16; 3: 8x8(h.264 not support 8x8).*/
    AX_S32 qpMapSize;
    AX_S32 maxCuSize; /* h264: 16; h265: 64 */

    AX_S32 rcMode;
    AX_U32 rcModeNew; /*0: CBR; 1: VBR; 2: AVBR 3: QPMAP 4: FIXQP. For change rcMode dynamically. */

    AX_U32 bitRate;
    AX_F32 srcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc chnnel */
    AX_F32 dstFrameRate; /* RW; Range:[1, 240]; the output frame rate of the venc chnnel */
    AX_U32 maxIprop;     /* Range:[1, 100]; the max I P size ratio */
    AX_U32 minIprop;     /* Range:[1, u32MaxIprop]; the min I P size ratio */
    AX_U32 qpMin;
    AX_U32 qpMax;
    AX_U32 qpMinI;
    AX_U32 qpMaxI;
    AX_U32 gopLen;
    AX_U32 IQp; /* RW; Range:[0, 51]; qp of the i frame */
    AX_U32 PQp; /* RW; Range:[0, 51]; qp of the p frame */
    AX_U32 BQp; /* RW; Range:[0, 51]; qp of the b frame */
    /* cvbr params */
    AX_U32 ltMaxBt;    /* Range:[2, 614400];the long-term target max bitrate, can not be larger than
                                            u32MaxBitRate,the unit is kbps */
    AX_U32 ltMinBt;    /* Range:[0, 614400];the long-term target min bitrate,  can not be larger than
                                            ltMaxBt,the unit is kbps */
    AX_U32 ltStaTime;  /* Range:[1, 1440]; the long-term rate statistic time, the unit is
                                          u32LongTermStatTimeUnit*/
    AX_U32 shtStaTime; /* Range:[1, 120]; the short-term rate statistic time, the unit is second (s)*/
    AX_U32 minQpDelta; /* Difference between FrameLevelMinQp and MinQp */
    AX_U32 maxQpDelta; /* Difference between FrameLevelMaxQp and MaxQp */
    /* end cvbr params */

    /*
     * Range:[-1, 51]; Fixed qp for every frame.
     * -1: disable fixed qp mode.
     * [0, 51]: value of fixed qp.
     */
    AX_S32 fixedQp;
    AX_PAYLOAD_TYPE_E enType;
    AX_BOOL roiEnable;
    AX_U32 qFactor;
    AX_U32 qRoiFactor;
    AX_CHAR *vencRoiMap;
    AX_CHAR *jencRoiMap;

    AX_U32 u32RefreshNum; /* how many p frame to do intra refresh in one gop */
    /* for dynamic resolution test case */
    AX_BOOL bDynRes; /* enable change resolution */
    AX_CHAR *newInput;
    AX_U32 newPicW; /* input frame width */
    AX_U32 newPicH; /* input frame height */
    /* end for dynamic resolution test case */
    AX_BOOL bInsertIDR;
    /* rate jam */
    AX_U32 drpFrmMode;
    AX_U32 encFrmGap;
    AX_U32 frmThrBps; /* instant bit rate, bps */
    /* super frame */
    AX_U32 pri; /* 0: framebits first; 1: bitrate first */
    AX_U32 thrI;
    AX_U32 thrP;
    /* slice split */
    AX_U32 sliceNum;
    /* user data size */
    AX_U32 uDataSize;
    /* for perf */
    AX_U32 vbCnt;
    AX_BOOL bPerf; /* test encode performance */
    AX_U64 totalSendFrame;
} SAMPLE_VENC_SENDFRAME_PARA_T;

typedef struct axSAMPLE_VENC_GETSTREAM_PARA_T
{
    AX_BOOL bGetStrmStart;
    VENC_CHN VeChn;
    AX_S32 syncType;
    pthread_t getStrmPid;
    AX_PAYLOAD_TYPE_E enType;
    AX_S32 testId;
    TestFunction function;
    AX_U16 gopMode; /*0: normalP; 1: oneLTR; 2: svc-t */
    /* filter the bit streams at different layer by temporalID in svc-t mode.
     * 0 : save the 0 layer bit stream.
     * 1 : save the 0 and 1 layer bit stream.
     * 2 : save the 0, 1 and 2 layer bit stream.
     */
    AX_U16 temporalID;
    AX_BOOL bGetStrmBufInfo;
    AX_BOOL bQueryStatus;
    AX_BOOL bSaveStrm;
    AX_U32 chnNum;
    AX_U32 grpId;
    AX_U32 startChn;
    /* output stream suffix */
    AX_CHAR strmSuffix[MAX_STREAM_SUFFIX];
    AX_CHAR *output;
    AX_U64 totalGetStream;
    AX_VOID *ptrPrivate;
} SAMPLE_VENC_GETSTREAM_PARA_T;

AX_S32 COMMON_VENC_Start(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_RC_E rcMode,
                         SAMPLE_VENC_CMD_PARA_T *pstArg);
AX_S32 COMMON_VENC_Stop(VENC_CHN VeChn);

AX_S32 COMMON_VENC_Create(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_RC_E rcMode,
                          SAMPLE_VENC_CMD_PARA_T *pstArg);

AX_VOID *COMMON_VENC_GetStreamProc(AX_VOID *arg);
AX_S32 COMMON_VENC_StartGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg);
AX_S32 COMMON_VENC_StopGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg);

AX_VOID *COMMON_VENC_SendFrameProc(AX_VOID *arg);
AX_S32 COMMON_VENC_StartSendFrame(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg);
AX_S32 COMMON_VENC_StopSendFrame(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg);

AX_VOID *COMMON_VENC_SendQpmapFrameProc(AX_VOID *arg);
AX_S32 COMMON_VENC_SendQpmapFrame();
AX_S32 COMMON_VENC_StopSendQpmapFrame();

AX_S32 COMMON_VENC_ReadFile(FILE *pFileIn, AX_S32 widthSrc, AX_S32 strideSrc, AX_S32 heightSrc, AX_IMG_FORMAT_E eFmt,
                            AX_VOID *pVaddr);
AX_S32 COMMON_VENC_ReadFbcFile(FILE *pFileIn, AX_S32 width, AX_S32 height, SAMPLE_VENC_FBC_INFO_T *pFbc,
                               AX_VIDEO_FRAME_T *pstFrame);

AX_VOID COMMON_VENC_AdjustLoopExit(AX_U64 *pVencLoopExit, AX_U32 chn);

AX_S32 COMMON_VENC_SaveJpegFile(VENC_CHN VeChn, AX_S32 testId, AX_U64 totalStream, AX_U8 *pu8Addr, AX_S32 u32Len,
                                AX_BOOL bSaveFile);
AX_S32 COMMON_VENC_WriteStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg, AX_BOOL bCommon, AX_U64 totalStream, FILE *pFile,
                               AX_VENC_STREAM_T *pstStream);

AX_VOID COMMON_VENC_SendGetDelay(VENC_CHN VeChn, AX_BOOL bExit, SAMPLE_VENC_SENDFRAME_PARA_T *pSend,
                                 SAMPLE_VENC_GETSTREAM_PARA_T *pGet);
#endif