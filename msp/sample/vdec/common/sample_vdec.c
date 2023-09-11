/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/prctl.h>

#include "common_vdec_api.h"

extern AX_U64 g_u64GetFrmTag;



static volatile AX_S32 s_ThreadExit = 0;
static volatile SAMPLE_VDEC_CONTEXT_T *s_pstVdecCtx = NULL;
static AX_BOOL s_groupSuccess = AX_FALSE;

static AX_U64 s_u64GetFrameNum[AX_VDEC_MAX_GRP_NUM][AX_DEC_MAX_CHN_NUM];

static AX_S32 __VdecRecvFrame(AX_VDEC_GRP VdGrp, AX_VDEC_CHN VdChn, SAMPLE_VDEC_CONTEXT_T *pstVdecCtx)
{
    int res;
    AX_S32 ret;
    AX_S32 sRet = AX_SUCCESS;
    AX_VIDEO_FRAME_INFO_T stFrameInfo[SAMPLE_VDEC_FRAME_CNT];
    AX_VIDEO_FRAME_INFO_T *pstFrameInfo;
    AX_S32 sMilliSec = AX_ERR_VDEC_UNKNOWN;
    FILE *fp_out = NULL;
    int ci = 0;
    int ii = 0;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    AX_U32 uStartGrpId = 0;
    SAMPLE_VDEC_USERPIC_T *pstVdecUserPic = NULL;
    AX_U32 u32UsrPicBlkId = 0;
    SAMPLE_VDEC_OUTPUT_INFO_T *pOutInfo = NULL;
    AX_CHAR *pOutputFilePath = NULL;

    if (pstVdecCtx == NULL) {
        SAMPLE_LOG("VdGrp=%d, pstVdecCtx == NULL", VdGrp);
        return -1;
    }

    pstCmd = &pstVdecCtx->tCmdParam;
    pstVdecUserPic = &pstVdecCtx->stVdecUserPic;
    u32UsrPicBlkId = pstVdecUserPic->stUserPic.stFrmInfo[VdChn].stVFrame.u32BlkId[0];
    sMilliSec = pstCmd->sMilliSec;
    uStartGrpId = pstCmd->uStartGrpId;

    for (ii = 0; ii < SAMPLE_VDEC_FRAME_CNT; ii++) {
        pstFrameInfo = &stFrameInfo[ii];

        memset(pstFrameInfo, 0, sizeof(AX_VIDEO_FRAME_INFO_T));
        SAMPLE_LOG_N("VdGrp=%d, VdChn=%d", VdGrp, VdChn);

        ret = AX_VDEC_GetChnFrame(VdGrp, VdChn, pstFrameInfo, sMilliSec);
        if (ret == AX_SUCCESS) {
            s_u64GetFrameNum[VdGrp][VdChn] += 1;
            SAMPLE_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame AX_SUCCESS, %lld\n",
                           VdGrp, VdChn, s_u64GetFrameNum[VdGrp][VdChn]);
        }
        else if (ret == AX_ERR_VDEC_QUEUE_EMPTY) {
            /* no data in unblock mode or timeout mode */
            SAMPLE_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame AX_ERR_VDEC_QUEUE_EMPTY\n",
                        VdGrp, VdChn);
            usleep(20 * 1000);
            sRet = ret;
            goto ERR_RET;
        }
        else if (ret == AX_ERR_VDEC_UNEXIST) {
            SAMPLE_ERR_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame AX_ERR_VDEC_UNEXIST \n",
                           VdGrp, VdChn);
            usleep(20 * 1000);
            sRet = ret;
            goto ERR_RET;
        }
        else if (ret == AX_ERR_VDEC_FLOW_END) {
            s_u64GetFrameNum[VdGrp][VdChn] += 1;

            SAMPLE_NOTICE_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame AX_ERR_VDEC_FLOW_END %lld\n",
                              VdGrp, VdChn, s_u64GetFrameNum[VdGrp][VdChn]);
            sRet = ret;
            goto ERR_RET;
        }
        else if (ret == AX_ERR_VDEC_STRM_ERROR) {
            s_u64GetFrameNum[VdGrp][VdChn] += 1;
            SAMPLE_WARN_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame AX_ERR_VDEC_STRM_ERROR, %lld\n",
                            VdGrp, VdChn, s_u64GetFrameNum[VdGrp][VdChn]);
            sRet = ret;
            goto ERR_RET;
        }
        else if (AX_ERR_VDEC_NOT_PERM == ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame AX_ERR_VDEC_NOT_PERM\n",
                            VdGrp, VdChn);
            sRet = ret;
            goto ERR_RET;
        }
        else {
            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame FAILED! ret=0x%x %s\n",
                            VdGrp, VdChn, ret, AX_VdecRetStr(ret));
            sRet = ret;
            goto ERR_RET;
        }

        SAMPLE_LOG("VdGrp=%d, VdChn:%d, ii:%d, before AX_VDEC_ReleaseChnFrame "
                    ".u32Width:%d, .bEndOfStream:%d, .u64PhyAddr[0]:0x%llx, "
                    ".u64VirAddr[0]:0x%llx, BlkId[0]:0x%x, u64PTS:%lld, .u64PrivateData:0x%llx\n",
                    VdGrp, VdChn, ii,
                    pstFrameInfo->stVFrame.u32Width, pstFrameInfo->bEndOfStream,
                    pstFrameInfo->stVFrame.u64PhyAddr[0], pstFrameInfo->stVFrame.u64VirAddr[0],
                    pstFrameInfo->stVFrame.u32BlkId[0], pstFrameInfo->stVFrame.u64PTS,
                    pstFrameInfo->stVFrame.u64PrivateData);
    }

    for (ii = 0; ii < SAMPLE_VDEC_FRAME_CNT; ii++) {
        pstFrameInfo = &stFrameInfo[ii];
        if (pstFrameInfo->bEndOfStream == AX_FALSE) {
            if (pstFrameInfo->stVFrame.u64PhyAddr[0] == 0) {
                if (pstVdecUserPic->usrPicChnEnaCnt && !pstCmd->tChnCfg[VdChn].bUserPicEnable) {
                    SAMPLE_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame FAILED! "
                                    "pstFrameInfo->stVFrame.u64PhyAddr[0] == 0\n",
                                    VdGrp, VdChn);
                } else {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_GetChnFrame FAILED! "
                                    "pstFrameInfo->stVFrame.u64PhyAddr[0] == 0\n",
                                    VdGrp, VdChn);
                    sRet = AX_ERR_VDEC_UNKNOWN;
                    goto ERR_RET;
                }
            }
        }
    }

    SampleVdecFeatureTest(VdGrp, pstCmd);

    fp_out = pstVdecCtx->pOutputFd[VdChn];
    SAMPLE_LOG("VdGrp=%d, VdChn:%d, fp_out:%p, after AX_VDEC_GetChnFrame",
               VdGrp, VdChn, fp_out);

    for (ii = 0; ii < SAMPLE_VDEC_FRAME_CNT; ii++) {
        pstFrameInfo = &stFrameInfo[ii];

        if (!pstFrameInfo->stVFrame.u64PhyAddr[0])
            continue;

        if (pstCmd->sWriteFrames || pstCmd->DestMD5) {
            pOutInfo = &pstVdecCtx->outInfo[VdChn];

            pOutInfo->VdGrp = VdGrp;
            pOutInfo->VdChn = VdChn;
            pOutInfo->bOneShot = AX_FALSE;
            pOutputFilePath = pstVdecCtx->pOutputFilePath[VdChn];
            if (pstCmd->bDynRes && pstFrameInfo->stVFrame.u64SeqNum) {
                if ((pOutInfo->enImgFormat != pstFrameInfo->stVFrame.enImgFormat) ||
                        (pOutInfo->u32CompressLevel != pstFrameInfo->stVFrame.stCompressInfo.u32CompressLevel) ||
                        (pOutInfo->u32Width != pstFrameInfo->stVFrame.u32Width) ||
                        (pOutInfo->u32Height != pstFrameInfo->stVFrame.u32Height) ||
                        (pOutInfo->u32PicStride != pstFrameInfo->stVFrame.u32PicStride[0])) {
                    if (fp_out) {
                        res = fclose(fp_out);
                        if (res) {
                            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn:%d, fclose FAILED! res:%d fp_out:%p\n",
                                            VdGrp, VdChn, res, fp_out);
                        }
                        fp_out = NULL;
                        pstVdecCtx->pOutputFd[VdChn] = NULL;
                    }
                    pOutputFilePath = pstVdecCtx->pOutputFilePath1[VdChn];
                }
            }

            pOutInfo->enImgFormat = pstFrameInfo->stVFrame.enImgFormat;
            pOutInfo->u32CompressLevel = pstFrameInfo->stVFrame.stCompressInfo.u32CompressLevel;
            pOutInfo->u32Width = pstFrameInfo->stVFrame.u32Width;
            pOutInfo->u32Height = pstFrameInfo->stVFrame.u32Height;
            pOutInfo->u32PicStride = pstFrameInfo->stVFrame.u32PicStride[0];

            if (fp_out == NULL) {
                SAMPLE_LOG("VdGrp=%d, VdChn:%d, pOutputFilePath:%s, out_info.VdGrp=%d, out_info.VdChn:%d",
                           VdGrp, VdChn, pOutputFilePath, pOutInfo->VdGrp, pOutInfo->VdChn);

                fp_out = OutputFileOpen(&pOutputFilePath, pOutInfo);
                if (fp_out == NULL) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, OutputFileOpen FAILED!",
                                    VdGrp, VdChn);
                    if (pstVdecCtx->pOutputFilePath[VdChn] != NULL) {
                        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn:%d, fopen pstVdecCtx->pOutputFilePath[VdChn]:%s FAILED!",
                                        VdGrp, VdChn, pstVdecCtx->pOutputFilePath[VdChn]);
                        free(pstVdecCtx->pOutputFilePath[VdChn]);
                        pstVdecCtx->pOutputFilePath[VdChn] = NULL;
                    }
                    sRet = AX_ERR_VDEC_UNKNOWN;
                    goto ERR_RET;
                }

                pstVdecCtx->pOutputFd[VdChn] = fp_out;

                SAMPLE_LOG("VdGrp=%d, VdChn:%d, pstVdecCtx->pOutputFd[VdChn]:%p, fp_out:%p",
                            VdGrp, VdChn, pstVdecCtx->pOutputFd[VdChn], fp_out);
            }
        }

        if (pstCmd->DestMD5) {
            AX_CHAR md5_str[33];

            memset(md5_str, 0, sizeof(md5_str));
            OutputFileCheckMD5(VdGrp, VdChn, pstFrameInfo, md5_str);

            fprintf(fp_out, "%s", md5_str);
            fprintf(fp_out, "\n");
            fflush(fp_out);
        } else {
            if (pstCmd->sWriteFrames) {
                ret = OutputFileSaveYUV(VdGrp, VdChn, pstFrameInfo, fp_out, pOutputFilePath);
                if (ret) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, OutputFileSaveYUV FAILED! ret:0x%x",
                                    VdGrp, VdChn, ret);
                    sRet = AX_ERR_VDEC_UNKNOWN;
                }
            }
        }
    }

    if (VdGrp == uStartGrpId) {
        if (pstFrameInfo->stVFrame.u64PhyAddr[0]) {
            printf("%-8lld", pstFrameInfo->stVFrame.u64SeqNum);
            if ((pstFrameInfo->stVFrame.u64SeqNum % 16) == 0) {
                printf("\n");
            }
            fflush(stdout);
        }
    }

    SAMPLE_LOG_N("VdGrp=%d, VdChn:%d ", VdGrp, VdChn);

    for (ii = 0; ii < SAMPLE_VDEC_FRAME_CNT; ii++) {
        pstFrameInfo = &stFrameInfo[ii];

        SAMPLE_LOG_N("VdGrp=%d, VdChn:%d, ii:%d", VdGrp, VdChn, ii);

        if (!pstFrameInfo->stVFrame.u64PhyAddr[0])
            continue;

        SAMPLE_LOG("VdGrp=%d, VdChn:%d, ii:%d, before AX_VDEC_ReleaseChnFrame "
                    ".u64PhyAddr[0]:0x%llx, BlkId[0]:0x%x, u64SeqNum:%lld\n",
                    VdGrp, VdChn, ii, pstFrameInfo->stVFrame.u64PhyAddr[0],
                    pstFrameInfo->stVFrame.u32BlkId[0], pstFrameInfo->stVFrame.u64SeqNum);

        ret = AX_VDEC_ReleaseChnFrame(VdGrp, VdChn, pstFrameInfo);
        if (ret) {
            if (AX_ERR_VDEC_FLOW_END != ret) {
                sRet = ret;
                SAMPLE_CRIT_LOG("VdGrp=%d, VdChn:%d, AX_VDEC_ReleaseChnFrame FAILED! res:0x%x %s \n"
                                "u64PhyAddr[0]:0x%llX, BlkId[0]:0x%x, BlkId[1]:0x%x\n",
                                VdGrp, VdChn, ret, AX_VdecRetStr(ret), pstFrameInfo->stVFrame.u64PhyAddr[0],
                                pstFrameInfo->stVFrame.u32BlkId[0], pstFrameInfo->stVFrame.u32BlkId[1]);
                goto ERR_RET;
            }
        }

        /* This is only valid when using the user image Insert feature.
         * AX_VDEC_SetUserPic. */
        if (pstVdecUserPic->usrPicChnEnaCnt) {
            SAMPLE_LOG("usrPicChnEnaCnt BlkId[0]:0x%x, u32UsrPicBlkId:0x%x",
                        pstFrameInfo->stVFrame.u32BlkId[0], u32UsrPicBlkId);

            if (pstFrameInfo->stVFrame.u32BlkId[0] == u32UsrPicBlkId) {
                pstVdecUserPic->usrPicGet[VdChn] = AX_TRUE;
            }

            for (ci = 0; ci < AX_DEC_MAX_CHN_NUM; ci++) {
                if (pstCmd->tChnCfg[ci].bUserPicEnable) {
                    pstVdecUserPic->bAllChnGetUsrPic = pstVdecUserPic->usrPicGet[ci] ? AX_TRUE : AX_FALSE;
                }
            }
        }

        if (!pstCmd->recvStmAfUsrPic) {
            if (pstVdecUserPic->bAllChnGetUsrPic || pstVdecUserPic->usrPicGet[VdChn]) {
                if (fp_out) {
                    res = fclose(fp_out);
                    if (res) {
                        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn:%d, fclose FAILED! res:%d fp_out:%p\n",
                                        VdGrp, VdChn, res, fp_out);
                    }
                    fp_out = NULL;
                    pstVdecCtx->pOutputFd[VdChn] = NULL;
                }
            }

            if (pstVdecUserPic->bAllChnGetUsrPic) {
                SAMPLE_LOG("VdGrp=%d, bAllChnGetUsrPic == AX_TRUE, ret AX_ERR_VDEC_FLOW_END", VdGrp);
                sRet = AX_ERR_VDEC_FLOW_END;
                goto ERR_RET;
            }
        }

        if (pstFrameInfo->bEndOfStream == AX_TRUE) {
            SAMPLE_LOG("VdGrp=%d, bEndOfStream == AX_TRUE, ret AX_ERR_VDEC_FLOW_END", VdGrp);
            sRet = AX_ERR_VDEC_FLOW_END;
            goto ERR_RET;
        }
    }

ERR_RET:
    for (ii = 0; ii < SAMPLE_VDEC_FRAME_CNT; ii++) {
        pstFrameInfo = &stFrameInfo[ii];
        SAMPLE_LOG_N("VdGrp=%d, VdChn:%d, .u64PTS:%lld, sRet0x%x %s\n",
                    VdGrp, VdChn, pstFrameInfo->stVFrame.u64PTS,
                    sRet, AX_VdecRetStr(sRet));
    }

    return sRet;
}



static void *_VdecRecvThread(void *arg)
{
    AX_S32 sRet = 0;
    AX_S32 ret = 0;
    SAMPLE_VDEC_GRP_SET_ARGS_T *pstGrpSetArgs = (SAMPLE_VDEC_GRP_SET_ARGS_T *)arg;
    SAMPLE_VDEC_ARGS_T *pstVdecGrpArgs = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstVdecCtx = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstCtx_Grp0 = NULL;
    AX_VDEC_GRP_SET_INFO_T stGrpSet;
    AX_VDEC_GRP_SET_INFO_T *pstGrpSet = &stGrpSet;
    int gi, ci;
    AX_VDEC_GRP VdGrp = 0;
    AX_VDEC_CHN VdChn = 0;
    int flow_end_total_cnt = 0;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    AX_U32 uGrpCount = 0;
    AX_BOOL bRecvFlowEnd[AX_VDEC_MAX_GRP_NUM] = {0};
    AX_VDEC_GRP uStartGrpId = -1;
    AX_VDEC_GRP maxGrpId = 0;
    AX_U32 noWorkingGrpCnt = 0;
    AX_U32 quitingGrpCnt = 0;

    if (arg == NULL) {
        SAMPLE_CRIT_LOG("arg == NULL");
        return NULL;
    }

    AX_CHAR cPthreadName[16];
    snprintf(cPthreadName, 16, "SampleVdecRecv");
    prctl(PR_SET_NAME, cPthreadName);

    pstVdecGrpArgs = &pstGrpSetArgs->stVdecGrpArgs[0];
    if (pstVdecGrpArgs == NULL) {
        SAMPLE_CRIT_LOG("pstVdecGrpArgs[0] == NULL");
        return NULL;
    }

    pstVdecCtx = pstVdecGrpArgs->pstCtx;
    if (pstVdecCtx == NULL) {
        SAMPLE_CRIT_LOG("pstVdecCtx == NULL");
        return NULL;
    }

    pstCtx_Grp0 = pstVdecCtx;
    pstCmd = &pstVdecCtx->tCmdParam;
    uGrpCount = pstCmd->uGrpCount;
    uStartGrpId = pstCmd->uStartGrpId;
    maxGrpId = pstCmd->uGrpCount + uStartGrpId;

    memset(pstGrpSet, 0, sizeof(AX_VDEC_GRP_SET_INFO_T));
    SAMPLE_LOG("uGrpCount:%d Enter while(1) SelectGrp \n", uGrpCount);

    while (1) {
        if (s_ThreadExit > 1) {
            SAMPLE_LOG("s_ThreadExit:%d, so break\n", s_ThreadExit);
            break;
        }

        noWorkingGrpCnt = 0;
        quitingGrpCnt = 0;
        for (VdGrp = uStartGrpId; VdGrp < maxGrpId; VdGrp++) {
            switch (pstVdecCtx->GrpStatus[VdGrp]) {
                case AX_VDEC_GRP_UNEXIST:
                case AX_VDEC_GRP_CREATED:
                    noWorkingGrpCnt++;
                    break;
                case AX_VDEC_GRP_START_RECV:
                case AX_VDEC_GRP_STOP_RECV:
                    break;
                default:
                    quitingGrpCnt++;
                    break;
            }
        }

        if (noWorkingGrpCnt == pstCmd->uGrpCount) {
            if (!s_groupSuccess && pstVdecCtx->RecvThdWait) {
                goto ERR_RET;
            } else {
                continue;
            }
        }

        if (quitingGrpCnt == pstCmd->uGrpCount) {
            SAMPLE_NOTICE_LOG("All groups are in the RESET or DESTROYED state. quitingGrpCnt:%d, pstCmd->uGrpCount:%d",
                              quitingGrpCnt, pstCmd->uGrpCount);
            goto ERR_RET;
        }

        SAMPLE_LOG("before SelectGrp \n");
        sRet = AX_VDEC_SelectGrp(pstGrpSet, 1000);
        if (sRet == AX_SUCCESS) {
            if (pstGrpSet->u32GrpCount == 0) {
                SAMPLE_ERR_LOG("AX_VDEC_SelectGrp AX_SUCCESS but u32GrpCount:0");
            } else {
                SAMPLE_DBG_LOG("AX_VDEC_SelectGrp u32GrpCount:%d", pstGrpSet->u32GrpCount);
            }
        }
        else if (AX_ERR_VDEC_TIMED_OUT == sRet) {
            SAMPLE_NOTICE_LOG("AX_VDEC_SelectGrp AX_ERR_VDEC_TIMED_OUT u32GrpCount:%d",
                              pstGrpSet->u32GrpCount);

            if (pstGrpSet->u32GrpCount == 0) {
                if (!s_groupSuccess && pstVdecCtx->RecvThdWait) {
                    goto ERR_RET;
                } else {
                    continue;
                }
            }
        }
        else if (AX_ERR_VDEC_NOT_INIT == sRet) {
            SAMPLE_NOTICE_LOG("AX_ERR_VDEC_NOT_INIT, Need Call AX_VDEC_Init first");
            goto ERR_RET;
        }
        else if (AX_ERR_VDEC_FLOW_END == sRet) {
            SAMPLE_NOTICE_LOG("AX_VDEC_SelectGrp AX_ERR_VDEC_FLOW_END");
            goto ERR_RET;
        }
        else {
            SAMPLE_CRIT_LOG("AX_VDEC_SelectGrp FAILED! ret:0x%x %s",
                            sRet, AX_VdecRetStr(sRet));
            goto ERR_RET;
        }

        SAMPLE_LOG("after SelectGrp, u32GrpCount:%d", pstGrpSet->u32GrpCount);
        for (gi = 0; gi < pstGrpSet->u32GrpCount; gi++) {
            VdGrp = pstGrpSet->stChnSet[gi].VdGrp;
            if (VdGrp < uStartGrpId) {
                SAMPLE_CRIT_LOG("VdGrp:%d < uStartGrpId:%d", VdGrp, uStartGrpId);
                goto ERR_RET;
            }


            pstVdecGrpArgs = &pstGrpSetArgs->stVdecGrpArgs[VdGrp - uStartGrpId];
            if (pstVdecGrpArgs == NULL) {
                SAMPLE_LOG("VdGrp=%d, pstVdecGrpArgs == NULL", VdGrp);
                continue;
            }

            pstVdecCtx = pstVdecGrpArgs->pstCtx;
            if (pstVdecCtx == NULL) {
                SAMPLE_LOG("VdGrp=%d, pstVdecCtx == NULL", VdGrp);
                continue;
            }

            switch (pstVdecCtx->GrpStatus[VdGrp]) {
                case AX_VDEC_GRP_START_RECV:
                case AX_VDEC_GRP_STOP_RECV:
                    break;
                default:
                    continue;
            }

            for (ci = 0; ci < pstGrpSet->stChnSet[gi].u32ChnCount; ci++) {
                VdChn = pstGrpSet->stChnSet[gi].VdChn[ci];
                pstVdecCtx->pOutputFilePath[VdChn] = pstCmd->tChnCfg[VdChn].pOutputFilePath;
                SAMPLE_LOG_N("u32GrpCount:%d, u32ChnCount:%d, VdGrp=%d, VdChn:%d, gi:%d, ci:%d",
                            pstGrpSet->u32GrpCount, pstGrpSet->stChnSet[gi].u32ChnCount,
                            VdGrp, VdChn, gi, ci);

                ret = __VdecRecvFrame(VdGrp, VdChn, pstVdecCtx);
                if (ret == AX_SUCCESS) {
                    pstCtx_Grp0->u64SelectFrameCnt++;
                }
                else if (ret == AX_ERR_VDEC_STRM_ERROR) {
                    SAMPLE_WARN_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s\n",
                                    VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                    continue;
                }
                else if (ret == AX_ERR_VDEC_QUEUE_EMPTY) {
                    SAMPLE_ERR_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s\n",
                                   VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                    continue;
                }
                else if (ret == AX_ERR_VDEC_UNEXIST) {
                    SAMPLE_ERR_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s \n",
                                   VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                    continue;
                }
                else if (AX_ERR_VDEC_NOT_PERM == ret) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s \n",
                                    VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                    continue;
                }
                else if (ret == AX_ERR_VDEC_FLOW_END) {
                    if (pstVdecCtx->stVdecUserPic.bAllChnGetUsrPic) {
                        pstCtx_Grp0->u64SelectFrameCnt++;
                    }

                    SAMPLE_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s\n",
                               VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                    continue;
                }
                else {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame FAILED! ret=0x%x %s\n",
                                    VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                    goto ERR_RET;
                }
            }

            if (ret == AX_ERR_VDEC_FLOW_END) {
                SAMPLE_LOG("VdGrp=%d, gi:%d, flow_end_total_cnt:%d, ->bRecvFlowEnd[VdGrp]:%d\n",
                           VdGrp, gi, flow_end_total_cnt, bRecvFlowEnd[VdGrp]);

                if (bRecvFlowEnd[VdGrp] == AX_FALSE) {
                    flow_end_total_cnt++;
                    bRecvFlowEnd[VdGrp] = AX_TRUE;
                    SAMPLE_LOG("VdGrp=%d, gi:%d, flow_end_total_cnt:%d, ->bRecvFlowEnd[VdGrp]:%d\n",
                               VdGrp, gi, flow_end_total_cnt, bRecvFlowEnd[VdGrp]);
                }
            }
        }

        if (flow_end_total_cnt == uGrpCount) {
            SAMPLE_LOG("flow_end_total_cnt:%d flow end!\n", flow_end_total_cnt);
            break;
        }
    }

ERR_RET:
    for (gi = 0; gi < uGrpCount; gi++) {
            VdGrp = gi;
            pstVdecGrpArgs = &pstGrpSetArgs->stVdecGrpArgs[VdGrp];
            if (pstVdecGrpArgs == NULL) {
                SAMPLE_LOG("VdGrp=%d, pstVdecGrpArgs == NULL", VdGrp);
                continue;
            }

            pstVdecCtx = pstVdecGrpArgs->pstCtx;
            if (pstVdecCtx == NULL) {
                SAMPLE_LOG("VdGrp=%d, pstVdecCtx == NULL", VdGrp);
                continue;
            }
            pstVdecCtx->bRecvFlowEnd = AX_TRUE;
    }

    if (pstCmd->bQuitWait == AX_TRUE) {
        SAMPLE_LOG_T("VdecRecvFrame Finished! Now waiting forever until press q");

        while(('q' != getchar())) {
            sleep(1);
        }

        for (gi = 0; gi < uGrpCount; gi++) {
            VdGrp = gi;
            pstVdecGrpArgs = &pstGrpSetArgs->stVdecGrpArgs[VdGrp];
            if (pstVdecGrpArgs == NULL) {
                SAMPLE_LOG("VdGrp=%d, pstVdecGrpArgs == NULL", VdGrp);
                continue;
            }

            pstVdecCtx = pstVdecGrpArgs->pstCtx;
            if (pstVdecCtx == NULL) {
                SAMPLE_LOG("VdGrp=%d, pstVdecCtx == NULL", VdGrp);
                continue;
            }
            pstVdecCtx->bGrpQuitWait[VdGrp] = AX_FALSE;
            SAMPLE_LOG("VdGrp=%d, bGrpQuitWait:%d", VdGrp, pstVdecCtx->bGrpQuitWait[VdGrp]);
        }
    }

    if (pstCtx_Grp0->u64SelectFrameCnt) {
        g_u64GetFrmTag += 1;
    } else {
        SAMPLE_CRIT_LOG("s_ThreadExit:%d, pstGrpSet->u32GrpCount:%d",
                        s_ThreadExit, pstGrpSet->u32GrpCount);
    }

    if (s_ThreadExit == 0) {
        s_ThreadExit += 2;
    }

    gettimeofday(&pstVdecCtx->Timeend, NULL);
    AX_U32 total_usec = 1000000 * (pstVdecCtx->Timeend.tv_sec - pstVdecCtx->Timebegin.tv_sec)
                        + pstVdecCtx->Timeend.tv_usec - pstVdecCtx->Timebegin.tv_usec;
    float total_msec = (float)total_usec / 1000.f;
    float msec_per_frame = total_msec / (float)pstCtx_Grp0->u64SelectFrameCnt;

    SAMPLE_LOG_T("uGrpCount=%d, msec per frame: %.1f, AVG FPS: %.1f. total msec:%.1f, total frame count:%lld \n",
                 uGrpCount, msec_per_frame, 1000.f / msec_per_frame, total_msec, pstCtx_Grp0->u64SelectFrameCnt);

    SAMPLE_LOG(" RecvThread exit, last VdGrp=%d, VdChn:%d\n", VdGrp, VdChn);
    return NULL;
}

static void *__VdecGrpChnRecvThread(void *arg)
{
    SAMPLE_VDEC_GRP_CHN_RECV_ARGS_T *pstFuncArgs = (SAMPLE_VDEC_GRP_CHN_RECV_ARGS_T *)arg;
    AX_VDEC_GRP VdGrp = 0;
    AX_S32 ret = 0;
    AX_VDEC_CHN VdChn = 0;
    SAMPLE_VDEC_CONTEXT_T *pstCtx = NULL;
    AX_U32 recvFrmCnt = 0;

    if (arg == NULL) {
        SAMPLE_CRIT_LOG("arg == NULL");
        return NULL;
    }

    VdChn = pstFuncArgs->VdChn;
    VdGrp = pstFuncArgs->VdGrp;

    pstCtx = pstFuncArgs->pstCtx;
    if (NULL == pstCtx) {
        SAMPLE_CRIT_LOG("pstFuncArgs->pstCtx == NULL");
        return NULL;
    }

    SAMPLE_LOG_N("VdGrp=%d, VdChn=%d, pstCtx:%p, pstFuncArgs->pstCtx:%p\n",
                VdGrp, VdChn, pstCtx, pstFuncArgs->pstCtx);

    while (1) {
        if (pstCtx->GrpStatus[VdGrp] == AX_VDEC_GRP_START_RECV) {
            break;
        }

        usleep(1000);
    }

    while (1) {
        if (s_ThreadExit > 1) {
            SAMPLE_LOG("VdGrp=%d, VdChn:%d, s_ThreadExit:%d\n", VdGrp, VdChn, s_ThreadExit);
            break;
        }

        SAMPLE_LOG_N("VdGrp=%d, VdChn=%d", VdGrp, VdChn);

        ret = __VdecRecvFrame(VdGrp, VdChn, pstCtx);
        if (ret != AX_SUCCESS) {
            if (ret == AX_ERR_VDEC_QUEUE_EMPTY) {
                SAMPLE_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s\n",
                           VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                continue;
            }
            else if (ret == AX_ERR_VDEC_UNEXIST) {
                SAMPLE_ERR_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s \n",
                               VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                continue;
            }
            else if (AX_ERR_VDEC_NOT_PERM == ret) {
                SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s \n",
                               VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                continue;
            }
            else if (ret == AX_ERR_VDEC_STRM_ERROR) {
                SAMPLE_WARN_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s\n",
                            VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                continue;
            }
            else if (ret == AX_ERR_VDEC_FLOW_END) {
                if (pstCtx->bRecvFlowEnd == AX_FALSE)
                    pstCtx->bRecvFlowEnd = AX_TRUE;
                SAMPLE_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame ret=0x%x %s\n",
                           VdGrp, VdChn, ret, AX_VdecRetStr(ret));
                break;
            }

            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, __VdecRecvFrame FAILED! ret=0x%x %s\n",
                            VdGrp, VdChn, ret, AX_VdecRetStr(ret));
            break;
        } else {
            recvFrmCnt++;
        }
    }


    if (recvFrmCnt) {
        g_u64GetFrmTag += 1;
    } else {
        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, s_ThreadExit:%d",
                        VdGrp, VdChn, s_ThreadExit);
    }

    if (s_ThreadExit == 0) {
        s_ThreadExit += 1;
        SAMPLE_NOTICE_LOG("++s_ThreadExit:%d\n", s_ThreadExit);
    }

    SAMPLE_LOG("VdGrp=%d, VdChn:%d, RecvThread exit\n",
               VdGrp, VdChn);
    return NULL;
}

static AX_S32 __VdecSendEndOfStream(AX_VDEC_GRP VdGrp)
{
    AX_S32 sRet = 0;
    AX_VDEC_STREAM_T tStrInfo = {0};
    tStrInfo.bEndOfStream = AX_TRUE;
    sRet = AX_VDEC_SendStream(VdGrp, &tStrInfo, -1);
    if (sRet != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_SendStream FAILED! ret:0x%x %s\n",
                        VdGrp, sRet, AX_VdecRetStr(sRet));
        goto ERR_RET;
    }

ERR_RET:
    return sRet;
}

static AX_S32 __VdecInputModeFrame(SAMPLE_VDEC_ARGS_T *pstFuncArgs,
                                   SAMPLE_INPUT_FILE_INFO_T *pstStreamInfo,
                                   SAMPLE_STREAM_BUF_T *pstStreamBuf)
{
    AX_S32 ret = 0;
    AX_S32 sRet = 0;
    AX_VDEC_STREAM_T tStrInfo = {0};
    AX_VDEC_GRP VdGrp = 0;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstCtx = NULL;
    AX_U32 uBufSize = 0;
    size_t sReadLen = 0;
    AX_U32 uSendPicNum = 0;
    AX_BOOL bContSendStm = AX_TRUE;
    AX_S32 sMilliSec = AX_ERR_VDEC_UNKNOWN;
    AX_BOOL bPerfTest = AX_FALSE;
    AX_BOOL bReadFrm = AX_TRUE;

    if (NULL == pstFuncArgs) {
        SAMPLE_CRIT_LOG("NULL == pstFuncArgs\n");
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    VdGrp = pstFuncArgs->VdGrp;
    if (NULL == pstStreamInfo) {
        SAMPLE_CRIT_LOG("VdGrp=%d, NULL == pstStreamInfo\n", VdGrp);
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    if (NULL == pstStreamBuf) {
        SAMPLE_CRIT_LOG("VdGrp=%d, NULL == pstStreamBuf\n", VdGrp);
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    if (pstStreamBuf->tBufAddr.pVirAddr == NULL) {
        SAMPLE_CRIT_LOG("pstStreamBuf->tBufAddr.pVirAddr == NULL\n");
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    pstCtx = pstFuncArgs->pstCtx;
    if (NULL == pstCtx) {
        SAMPLE_CRIT_LOG("pstFuncArgs->pstCtx == NULL\n");
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    pstCmd = &pstCtx->tCmdParam;
    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("VdGrp=%d, pstCmd == NULL", VdGrp);
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    sMilliSec = pstCmd->sMilliSec;
    bPerfTest = pstCmd->bPerfTest;
    pstStreamInfo->enDecType = pstCmd->enDecType;
    uBufSize = pstStreamBuf->uBufSize;
    SAMPLE_LOG("begin to decoder. uBufSize=%d\n", uBufSize);

    while (1) {
        if (pstCmd->sLoopDecNum <= 0) {
            sRet = __VdecSendEndOfStream(VdGrp);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, __VdecSendEndOfStream FAILED! ret:0x%x %s",
                                VdGrp, sRet, AX_VdecRetStr(sRet));
                ret = AX_ERR_VDEC_UNKNOWN;
                goto ERR_RET;
            }
            SAMPLE_LOG("VdGrp=%d, s_ThreadExit:%d, __VdecSendEndOfStream Done! break sendstream while(1)",
                       VdGrp, s_ThreadExit);
            break;
        }

        switch (pstCtx->GrpStatus[VdGrp]) {
            case AX_VDEC_GRP_UNEXIST:
            case AX_VDEC_GRP_CREATED:
                continue;
            case AX_VDEC_GRP_START_RECV:
                break;
            default:
                ret = AX_ERR_VDEC_FLOW_END;
                goto ERR_RET;
        }

        if (bReadFrm) {
            if ((pstCmd->enInputMode !=AX_VDEC_INPUT_MODE_STREAM)
                && ((pstCmd->enDecType == PT_H264) || (pstCmd->enDecType == PT_H265))) {
                if (pstCmd->bFfmpegEnable) {
                    sRet = SampleVdecFfmpegExtractOnePic(&pstCtx->stFfmpeg, &pstCtx->stBitStreamInfo,
                                                         pstStreamBuf, &sReadLen);
                    if (sRet) {
                        SAMPLE_CRIT_LOG("VdGrp=%d, SampleVdecFfmpegExtractOnePic FAILED! ret:0x%x\n", VdGrp, sRet);
                        ret = AX_ERR_VDEC_UNKNOWN;
                        goto ERR_RET;
                    }
                } else {
                    sRet = StreamFileParserReadFrame(pstStreamInfo, pstStreamBuf, &sReadLen);
                    if (sRet) {
                        SAMPLE_CRIT_LOG("VdGrp=%d, StreamParserReadFrameH264 FAILED! ret:0x%x\n", VdGrp, sRet);
                        ret = AX_ERR_VDEC_UNKNOWN;
                        goto ERR_RET;
                    }
                }
            } else if (pstCmd->enDecType == PT_JPEG) {
                if (!(bPerfTest && uSendPicNum)) {
                    sRet = StreamParserReadFrameJpeg(pstStreamInfo, pstStreamBuf, &sReadLen);
                    if (sRet) {
                        SAMPLE_CRIT_LOG("VdGrp=%d, StreamParserReadFrameJpeg FAILED! ret:0x%x\n", VdGrp, sRet);
                        ret = AX_ERR_VDEC_UNKNOWN;
                        goto ERR_RET;
                    }
                }
            }
        }

        if (sReadLen > 0) {
            tStrInfo.pu8Addr = pstStreamBuf->tBufAddr.pVirAddr;
            tStrInfo.u64PhyAddr = 0;
            tStrInfo.u32StreamPackLen = (AX_U32)sReadLen;  /*stream len*/
            tStrInfo.bEndOfFrame = AX_TRUE;
            tStrInfo.bEndOfStream = AX_FALSE;

            sRet = AX_SYS_GetCurPTS(&tStrInfo.u64PTS);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, AX_SYS_GetCurPTS FAILED! ret:0x%x\n", VdGrp, sRet);
                ret = sRet;
                goto ERR_RET;
            }
            tStrInfo.u64PrivateData = 0xAFAF5A5A;
        } else {
            if (pstCmd->enDecType == PT_JPEG) {
                if (!pstCmd->bDynRes) {
                    pstCmd->sLoopDecNum--;
                } else if (pstCmd->bDynRes && (pstStreamInfo->fInput == pstCtx->pNewInputFd[VdGrp])) {
                    pstCmd->sLoopDecNum--;
                    fseek(pstStreamInfo->fInput, 0, SEEK_SET);
                    pstStreamInfo->fInput = pstCtx->pInputFd[VdGrp];
                    pstStreamInfo->curPos = 0;
                    pstStreamInfo->sFileSize = pstCtx->oInputFileSize[VdGrp];
               } else if (pstCmd->bDynRes && (pstStreamInfo->fInput == pstCtx->pInputFd[VdGrp])) {
                    fseek(pstStreamInfo->fInput, 0, SEEK_SET);
                    pstStreamInfo->fInput = pstCtx->pNewInputFd[VdGrp];
                    pstStreamInfo->curPos = 0;
                    pstStreamInfo->sFileSize = pstCtx->oNewInputFileSize[VdGrp];
                }
            } else {
                pstCmd->sLoopDecNum--;
            }

            if (pstCmd->sLoopDecNum > 0) {
                if (pstCmd->bFfmpegEnable && pstCmd->enDecType != PT_JPEG) {
                    if ((pstCmd->enInputMode !=AX_VDEC_INPUT_MODE_STREAM)
                            && ((pstCmd->enDecType == PT_H264) || (pstCmd->enDecType == PT_H265))) {
                        sRet = SampleVdecFfmpegDeinit(&pstCtx->stFfmpeg, pstCtx->stBitStreamInfo.VdGrp);
                        if (sRet != AX_SUCCESS) {
                            SAMPLE_CRIT_LOG("VdGrp=%d, SampleVdecFfmpegDeinit FAILED! ret:0x%x\n",
                                        VdGrp, sRet);
                        }
                        pstCtx->stBitStreamInfo.VdGrp = VdGrp;
                        sRet = SampleVdecFfmpegInit(&pstCtx->stFfmpeg, pstCmd->pInputFilePath, &pstCtx->stBitStreamInfo);
                        if (sRet != AX_SUCCESS) {
                            SAMPLE_CRIT_LOG("VdGrp=%d, SampleVdecFfmpegInit FAILED! ret:0x%x\n",
                                        VdGrp, sRet);
                        }
                        continue;
                    }
                } else {
                    fseek(pstStreamInfo->fInput, 0, SEEK_SET);
                   if (pstCmd->enDecType == PT_JPEG)
                       pstStreamInfo->curPos = 0;
                    continue;
                }
            } else {
                SAMPLE_LOG("VdGrp=%d, Notice! pstCmd->sLoopDecNum: %d\n", VdGrp, pstCmd->sLoopDecNum);
                continue;
            }
        }

        SAMPLE_LOG("VdGrp=%d, before AX_VDEC_SendStream, uSendPicNum:%d, sReadLen:0x%lx",
                   VdGrp, uSendPicNum, sReadLen);

        SAMPLE_LOG("VdGrp=%d, tStrInfo.pu8Addr:%p, tStrInfo.u64PhyAddr:0x%llx, sRecvPicNum:%d ",
                   VdGrp, tStrInfo.pu8Addr, tStrInfo.u64PhyAddr, pstCmd->sRecvPicNum);

        SAMPLE_LOG("VdGrp=%d, s_ThreadExit:%d, .u64PTS:%lld .bEndOfStream:%d, .bEndOfFrame:%d, .bSkipDisplay:%d",
                   VdGrp, s_ThreadExit, tStrInfo.u64PTS,
                   tStrInfo.bEndOfStream, tStrInfo.bEndOfFrame, tStrInfo.bSkipDisplay);

        if ((pstCtx->Timebegin.tv_sec == 0) && (pstCtx->Timebegin.tv_usec == 0)) {
            gettimeofday(&pstCtx->Timebegin, NULL);
        }

        sRet = AX_VDEC_SendStream(VdGrp, &tStrInfo, sMilliSec);
        if (sRet == AX_SUCCESS) {
            SAMPLE_LOG("VdGrp=%d, AX_VDEC_SendStream AX_SUCCESS, uSendPicNum:%d", VdGrp, uSendPicNum);
        }
        else if (sRet == AX_ERR_VDEC_FLOW_END) {
            SAMPLE_LOG("VdGrp=%d, AX_VDEC_SendStream ret AX_ERR_VDEC_FLOW_END, uSendPicNum:%d",
                        VdGrp, uSendPicNum);
            break;
        }
        else if (sRet == AX_ERR_VDEC_QUEUE_FULL) {
            bReadFrm = AX_FALSE;
            usleep(1000);
            continue;
        }
        else if (sRet == AX_ERR_VDEC_NOT_PERM) {
            bReadFrm = AX_FALSE;
            usleep(1000);
            continue;
        }
        else if ((sRet == AX_ERR_VDEC_NOT_SUPPORT)
                || (sRet == AX_ERR_VDEC_NOMEM)
                || (sRet == AX_ERR_VDEC_NOBUF)) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_SendStream FAILED! ret:0x%x %s",
                            VdGrp, sRet, AX_VdecRetStr(sRet));

            ret = __VdecSendEndOfStream(VdGrp);
            if (ret) {
                SAMPLE_CRIT_LOG("VdGrp=%d, __VdecSendEndOfStream FAILED! ret:0x%x %s",
                                VdGrp, ret, AX_VdecRetStr(ret));
            }
            goto ERR_RET;
        }
        else {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_SendStream FAILED! ret:0x%x %s\n",
                            VdGrp, sRet, AX_VdecRetStr(sRet));
            ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET;
        }

        bReadFrm = AX_TRUE;
        uSendPicNum ++;

        if (pstFuncArgs->tUsrPicArgs.pstVdecUserPic->usrPicChnEnaCnt
                &&  (uSendPicNum == pstCmd->usrPicIdx)) {
            sRet = VdecUserPicEnable(VdGrp, pstFuncArgs->tUsrPicArgs.pstVdecUserPic,
                                     &bContSendStm, pstCtx);
            if (sRet != AX_SUCCESS){
                SAMPLE_CRIT_LOG("VdGrp=%d, VdecUserPicEnable FAILED! ret:0x%x %s",
                                VdGrp, sRet, AX_VdecRetStr(sRet));
                goto ERR_RET;
            }

            if (!bContSendStm) {
                sReadLen = 0;
                pstCmd->sLoopDecNum--;
                break;
            }
        }

        if (uSendPicNum == pstCmd->sRecvPicNum)
            pstCmd->sLoopDecNum--;
    }


    return 0;

ERR_RET:
    return ret;
}


static AX_S32 __VdecChnAttrEnable(SAMPLE_VDEC_ARGS_T *pstFuncArgs, AX_VDEC_CHN_ATTR_T *pstChnSet)
{
    AX_VDEC_GRP VdGrp = AX_INVALID_ID;
    AX_VDEC_CHN VdChn = 0;

    AX_S32 sRet = AX_SUCCESS;
    AX_VDEC_CHN_ATTR_T *pstVdChnAttr[AX_DEC_MAX_CHN_NUM];
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstCtx = NULL;

    VdGrp = pstFuncArgs->VdGrp;
    pstCtx = pstFuncArgs->pstCtx;
    pstCmd = &pstCtx->tCmdParam;

    for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
        if (pstCmd->tChnCfg[VdChn].bChnEnable == AX_FALSE) {
            SAMPLE_LOG("VdGrp=%d, VdChn=%d, bChnEnable=%d",
                    VdGrp, VdChn, pstCmd->tChnCfg[VdChn].bChnEnable);
            continue;
        }

        pstVdChnAttr[VdChn] = pstChnSet + VdChn;
        pstFuncArgs->pstVdChnAttr[VdChn] = pstVdChnAttr[VdChn];

        sRet = SampleVdecChnAttrSet(VdGrp, VdChn, pstCmd, pstVdChnAttr[VdChn]);

        if (sRet != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_SetChnAttr FAILED! ret:0x%x %s\n",
                            VdGrp, VdChn, sRet, AX_VdecRetStr(sRet));
            goto ERR_RET;
        }

        sRet = AX_VDEC_EnableChn(VdGrp, VdChn);
        if (sRet != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_EnableChn FAILED! ret:0x%x %s\n",
                            VdGrp, VdChn, sRet, AX_VdecRetStr(sRet));
            goto ERR_RET;
        }
    }

ERR_RET:
    return sRet;
}


static AX_S32 __VdecCreateUserPool(SAMPLE_VDEC_ARGS_T *pstFuncArgs, AX_VDEC_CHN_ATTR_T *pstChnSet)
{
    AX_VDEC_CHN_ATTR_T *pstVdChnAttr[AX_DEC_MAX_CHN_NUM];
    AX_FRAME_COMPRESS_INFO_T tCompressInfo;
    AX_U32 FrameSize = 0;
    AX_VDEC_GRP VdGrp = AX_INVALID_ID;
    AX_VDEC_CHN VdChn = 0;
    AX_VDEC_GRP_ATTR_T *pstVdGrpAttr = NULL;
    AX_POOL_CONFIG_T *pstPoolConfig = NULL;
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 sRet = AX_SUCCESS;
    int tmp_ci = 0;

    if (pstFuncArgs == NULL) {
        SAMPLE_CRIT_LOG("pstFuncArgs == NULL\n");
        s32Ret = AX_ERR_VDEC_NULL_PTR;
        goto ERR_RET;
    }

    VdGrp = pstFuncArgs->VdGrp;

    if (pstChnSet == NULL) {
        SAMPLE_CRIT_LOG("pstChnSet == NULL\n");
        s32Ret = AX_ERR_VDEC_NULL_PTR;
        goto ERR_RET;
    }

    tCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
    tCompressInfo.u32CompressLevel = 0;

    pstVdGrpAttr = &pstFuncArgs->tVdGrpAttr;

    for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
        if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

        pstVdChnAttr[VdChn] = pstChnSet + VdChn;
        if (pstVdChnAttr[VdChn] == NULL) {
            SAMPLE_CRIT_LOG("pstVdChnAttr[VdChn] == NULL\n");
            s32Ret = AX_ERR_VDEC_NULL_PTR;
            goto ERR_RET;
        }

        pstPoolConfig = &pstFuncArgs->tPoolConfig[VdChn];

        FrameSize = AX_VDEC_GetPicBufferSize(pstVdChnAttr[VdChn]->u32FrameStride,
                                            pstVdChnAttr[VdChn]->u32PicHeight,
                                            pstVdChnAttr[VdChn]->enImgFormat,
                                            &tCompressInfo, pstVdGrpAttr->enCodecType);

        pstPoolConfig->MetaSize = 512;
        pstPoolConfig->BlkCnt = pstVdChnAttr[VdChn]->u32OutputFifoDepth;

        SAMPLE_LOG("Get FrameSize is 0x%x %d, BlkCnt:%d\n",
                    FrameSize, FrameSize, pstPoolConfig->BlkCnt);

        pstPoolConfig->BlkSize = FrameSize;
        pstPoolConfig->CacheMode = POOL_CACHE_MODE_NONCACHE;
        // memset(pstPoolConfig->PartitionName, 0, sizeof(pstPoolConfig->PartitionName));
        snprintf((AX_CHAR *)pstPoolConfig->PartitionName, AX_MAX_PARTITION_NAME_LEN, "anonymous");
    }

    for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
        if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

        pstPoolConfig = &pstFuncArgs->tPoolConfig[VdChn];
        pstFuncArgs->PoolId[VdChn] = AX_POOL_CreatePool(pstPoolConfig);
        if (AX_INVALID_POOLID == pstFuncArgs->PoolId[VdChn]) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_POOL_CreatePool FAILED! BlkCnt:%d, BlkSize:0x%llx\n",
                            VdGrp, pstPoolConfig->BlkCnt,
                            pstPoolConfig->BlkSize);
            goto ERR_RET_DESTROY_POOL;
        }
    }

    for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
        if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

        s32Ret = AX_VDEC_AttachPool(VdGrp, VdChn, pstFuncArgs->PoolId[VdChn]);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_AttachPool FAILED! PoolId[%d]:%d ret:0x%x %s\n",
                            VdGrp, VdChn, pstFuncArgs->PoolId[VdChn],
                            s32Ret, AX_VdecRetStr(s32Ret));
            goto ERR_RET_DETACH_POOL;
        }
    }

    return s32Ret;

ERR_RET_DETACH_POOL:
    tmp_ci = VdChn;
    for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
        if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

        sRet = AX_VDEC_DetachPool(VdGrp, VdChn);
        if (sRet) {
            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_DetachPool FAILED! ret:0x%x %s",
                            VdGrp, VdChn, sRet, AX_VdecRetStr(sRet));
        }
    }

ERR_RET_DESTROY_POOL:
    tmp_ci = VdChn;
    for (VdChn = 0; VdChn < tmp_ci; VdChn++) {
        if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

        if (pstFuncArgs->PoolId[VdChn] == AX_INVALID_POOLID) continue;

        sRet = AX_POOL_DestroyPool(pstFuncArgs->PoolId[VdChn]);
        if (sRet) {
            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, PoolId[VdChn]:%d, AX_POOL_DestroyPool FAILED! ret:0x%x %s",
                            VdGrp, VdChn, pstFuncArgs->PoolId[VdChn], sRet, AX_VdecRetStr(sRet));
        }
    }

ERR_RET:
    return s32Ret;
}


static AX_S32 __VdecGrpCreate(AX_VDEC_GRP VdGrp, AX_VDEC_GRP_ATTR_T *pstVdGrpAttr,
                              SAMPLE_VDEC_CMD_PARAM_T *pstCmd, AX_U32 uStreamBufSize)
{
    AX_S32 s32Ret = AX_SUCCESS;

    pstVdGrpAttr->enCodecType = pstCmd->enDecType;
    pstVdGrpAttr->u32MaxPicWidth = pstCmd->u32MaxPicWidth;  /*Max pic width*/
    pstVdGrpAttr->u32MaxPicHeight = pstCmd->u32MaxPicHeight;  /*Max pic height*/
    pstVdGrpAttr->u32StreamBufSize = uStreamBufSize;
    pstVdGrpAttr->enInputMode = pstCmd->enInputMode;

    if (pstCmd->enFrameBufSrc == POOL_SOURCE_USER) {
        pstVdGrpAttr->bSdkAutoFramePool = AX_FALSE;
    } else if (pstCmd->enFrameBufSrc == POOL_SOURCE_PRIVATE) {
        pstVdGrpAttr->bSdkAutoFramePool = AX_TRUE;
    } else {
        SAMPLE_CRIT_LOG("Unsupport enFrameBufSrc:%d\n",
                        pstCmd->enFrameBufSrc);
        goto ERR_RET;
    }

    pstVdGrpAttr->bSkipSdkStreamPool = AX_FALSE;

    s32Ret = AX_VDEC_CreateGrp(VdGrp, pstVdGrpAttr);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_CreateGrp FAILED! ret:0x%x %s\n",
                        VdGrp, s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET;
    }

ERR_RET:
    return s32Ret;
}

static AX_S32 __VdecInputModeStream(SAMPLE_VDEC_ARGS_T *pstFuncArgs,
                                   SAMPLE_INPUT_FILE_INFO_T *pstStreamInfo,
                                   SAMPLE_STREAM_BUF_T *pstStreamBuf)
{
    AX_S32 ret = 0;
    AX_S32 sRet = 0;
    AX_VDEC_STREAM_T tStrInfo = {0};
    AX_VDEC_GRP VdGrp = 0;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstCtx = NULL;
    FILE *fInput = NULL;
    off_t inputFileSize = 0;
    size_t fread_sz = 0;

    if (NULL == pstFuncArgs) {
        SAMPLE_CRIT_LOG("NULL == pstFuncArgs\n");
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    VdGrp = pstFuncArgs->VdGrp;
    if (NULL == pstStreamInfo) {
        SAMPLE_CRIT_LOG("VdGrp=%d, NULL == pstStreamInfo\n", VdGrp);
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    if (NULL == pstStreamBuf) {
        SAMPLE_CRIT_LOG("VdGrp=%d, NULL == pstStreamBuf\n", VdGrp);
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    if (pstStreamBuf->tBufAddr.pVirAddr == NULL) {
        SAMPLE_CRIT_LOG("pstStreamBuf->tBufAddr.pVirAddr == NULL\n");
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    pstCtx = pstFuncArgs->pstCtx;
    if (NULL == pstCtx) {
        SAMPLE_CRIT_LOG("pstFuncArgs->pstCtx == NULL\n");
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    pstCmd = &pstCtx->tCmdParam;
    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("VdGrp=%d, pstCmd == NULL", VdGrp);
        ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    fInput = pstStreamInfo->fInput;
    rewind(fInput);

    inputFileSize = pstStreamInfo->sFileSize;
    AX_U32 uLeftSize = inputFileSize;
    AX_U32 uReadPackSize = pstCmd->sStreamSize;

    while (1) {
        if ((s_ThreadExit > 1) || (pstCmd->sLoopDecNum <= 0)) {
            sRet = __VdecSendEndOfStream(VdGrp);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, __VdecSendEndOfStream FAILED! ret:0x%x %s",
                                VdGrp, sRet, AX_VdecRetStr(sRet));
                ret = AX_ERR_VDEC_UNKNOWN;
                goto ERR_RET;
            }
            SAMPLE_LOG("VdGrp=%d, s_ThreadExit:%d, __VdecSendEndOfStream Done! break sendstream while(1)",
                       VdGrp, s_ThreadExit);
            break;
        }

        uReadPackSize = ((uLeftSize < pstCmd->sStreamSize) ? uLeftSize : pstCmd->sStreamSize);
        fread_sz = fread(pstStreamBuf->tBufAddr.pVirAddr, 1, uReadPackSize, fInput);
        uLeftSize = inputFileSize - uReadPackSize;
        tStrInfo.pu8Addr = pstStreamBuf->tBufAddr.pVirAddr;
        tStrInfo.u64PhyAddr = 0;
        tStrInfo.u32StreamPackLen = uReadPackSize;
        tStrInfo.bEndOfStream = AX_FALSE;

        if ((pstCtx->Timebegin.tv_sec == 0) && (pstCtx->Timebegin.tv_usec == 0)) {
            gettimeofday(&pstCtx->Timebegin, NULL);
        }

        SAMPLE_LOG("VdGrp=%d, pts:%lld inputFileSize:0x%lx fread_sz:0x%lx\n",
                    VdGrp, tStrInfo.u64PTS, inputFileSize, fread_sz);
        sRet = AX_VDEC_SendStream(VdGrp, &tStrInfo, -1);
        if (sRet != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_SendStream FAILED! ret:0x%x %s\n",
                            VdGrp, sRet, AX_VdecRetStr(sRet));
            ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET;
        }

        inputFileSize -= fread_sz;
        if (inputFileSize == 0) {
            pstCmd->sLoopDecNum--;
            fseek(pstStreamInfo->fInput, 0, SEEK_SET);
            inputFileSize = pstStreamInfo->sFileSize;
            uLeftSize = inputFileSize;
            uReadPackSize = pstCmd->sStreamSize;
            continue;
        }
    }


    SAMPLE_LOG("VdGrp=%d, pts:%lld AX_VDEC_SendStream done! loop \n",
               VdGrp, tStrInfo.u64PTS);

    return AX_ERR_VDEC_FLOW_END;

ERR_RET:
    return ret;
}

static AX_S32 __VdecGrpSendStream(SAMPLE_VDEC_ARGS_T *pstFuncArgs, SAMPLE_STREAM_BUF_T *pstStreamBuf)
{
    AX_S32 sRet = AX_SUCCESS;
    AX_VDEC_RECV_PIC_PARAM_T tRecvParam;
    AX_VDEC_GRP VdGrp = pstFuncArgs->VdGrp;
    SAMPLE_INPUT_FILE_INFO_T tStreamInfo = {0};

    SAMPLE_VDEC_CONTEXT_T *pstVdecCtx;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd;

    pstVdecCtx = pstFuncArgs->pstCtx;
    pstCmd = &pstVdecCtx->tCmdParam;


    sRet = AX_VDEC_SetDisplayMode(VdGrp, pstCmd->enDisplayMode);
    if (sRet != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_VDEC_SetDisplayMode failed! ret:0x%x %s\n", sRet, AX_VdecRetStr(sRet));
        goto ERR_RET;
    }

    if (s_ThreadExit) {
        SAMPLE_NOTICE_LOG("s_ThreadExit:%d\n", s_ThreadExit);
    }

    while (!s_ThreadExit && (pstCmd->sLoopDecNum > 0)) {
        memset(&tRecvParam, 0, sizeof(tRecvParam));
        tRecvParam.s32RecvPicNum = pstCmd->sRecvPicNum;
        sRet = AX_VDEC_StartRecvStream(VdGrp, &tRecvParam);
        if (sRet != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_StartRecvStream FAILED! ret:0x%x %s\n",
                            VdGrp, sRet, AX_VdecRetStr(sRet));
            goto ERR_RET;
        } else {
            pstVdecCtx->GrpStatus[VdGrp] = AX_VDEC_GRP_START_RECV;
        }

        SAMPLE_LOG_N("VdGrp=%d, pstVdecCtx:%p, pstCtx->GrpStatus[VdGrp]:%d\n",
                   VdGrp, pstVdecCtx, pstVdecCtx->GrpStatus[VdGrp]);

        SAMPLE_LOG_N("VdGrp=%d, pstCmd->enInputMode:%d \n", VdGrp, pstCmd->enInputMode);
        if (s_ThreadExit > 1) {
            SAMPLE_LOG("VdGrp=%d, s_ThreadExit:%d, so goto exit", VdGrp, s_ThreadExit);
            sRet = __VdecSendEndOfStream(VdGrp);
            if (sRet != AX_SUCCESS) {
                SAMPLE_CRIT_LOG("VdGrp=%d, __VdecSendEndOfStream FAILED! ret:0x%x %s\n",
                                VdGrp, sRet, AX_VdecRetStr(sRet));
                goto ERR_RET_STOP_RECV;
            }
        } else {
            if ((AX_VDEC_INPUT_MODE_FRAME == pstCmd->enInputMode)
                    || (AX_VDEC_INPUT_MODE_NAL == pstCmd->enInputMode)) {
                tStreamInfo.fInput = pstVdecCtx->pInputFd[VdGrp];
                tStreamInfo.sFileSize = pstVdecCtx->oInputFileSize[VdGrp];

                sRet = __VdecInputModeFrame(pstFuncArgs, &tStreamInfo, pstStreamBuf);
                if (sRet) {
                    if (sRet != AX_ERR_VDEC_FLOW_END) {
                        SAMPLE_CRIT_LOG("VdGrp=%d, VdecInputModeFrame FAILED! ret:0x%x\n", VdGrp, sRet);
                        goto ERR_RET_STOP_RECV;
                    }
                }
            }
            else if ((AX_VDEC_INPUT_MODE_COMPAT == pstCmd->enInputMode)
                    || (AX_VDEC_INPUT_MODE_STREAM == pstCmd->enInputMode)) {
                tStreamInfo.fInput = pstVdecCtx->pInputFd[VdGrp];
                tStreamInfo.sFileSize = pstVdecCtx->oInputFileSize[VdGrp];
                fseek(tStreamInfo.fInput, 0, SEEK_SET);

                sRet = __VdecInputModeStream(pstFuncArgs, &tStreamInfo, pstStreamBuf);
                if (sRet) {
                    if (sRet != AX_ERR_VDEC_FLOW_END) {
                        SAMPLE_CRIT_LOG("VdGrp=%d, __VdecInputModeStream FAILED! ret:0x%x\n", VdGrp, sRet);
                        goto ERR_RET_STOP_RECV;
                    }
                }
            }
        }

        sRet = AX_VDEC_StopRecvStream(VdGrp);
        if (sRet) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_StopRecvStream FAILED! ret:0x%x %s",
                            VdGrp, sRet, AX_VdecRetStr(sRet));
        } else {
            pstVdecCtx->GrpStatus[VdGrp] = AX_VDEC_GRP_STOP_RECV;
        }

        SAMPLE_LOG("VdGrp=%d, AX_VDEC_StopRecvStream Done! sLoopDecNum:%d\n",
                    VdGrp, pstCmd->sLoopDecNum);
        if (s_ThreadExit) {
            break;
        }
    }

    return sRet;

ERR_RET_STOP_RECV:
    sRet = AX_VDEC_StopRecvStream(VdGrp);
    if (sRet) {
        SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_StopRecvStream FAILED! ret:0x%x", VdGrp, sRet);
    } else {
        pstVdecCtx->GrpStatus[VdGrp] = AX_VDEC_GRP_STOP_RECV;
    }
ERR_RET:
    s_ThreadExit += 1;
    SAMPLE_NOTICE_LOG("++s_ThreadExit:%d\n", s_ThreadExit);
    return sRet;
}

static AX_S32 __VdecInitUsrPicArgs(SAMPLE_VDEC_ARGS_T *pstFuncArgs, SAMPLE_VDEC_USERPIC_T *pstVdecUserPic)
{
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstVdecCtx = NULL;
    SAMPLE_VDEC_USRPIC_ARGS_T *pstUsrPicArgs = NULL;
    AX_VDEC_GRP VdGrp = 0;
    AX_VDEC_CHN VdChn = 0;
    AX_S32 sRet = AX_SUCCESS;

    if (pstFuncArgs == NULL) {
        sRet = AX_ERR_VDEC_UNKNOWN;
        SAMPLE_CRIT_LOG("pstFuncArgs == NULL\n");
        goto ERR_RET;
    }
    VdGrp = pstFuncArgs->VdGrp;
    if (pstVdecUserPic == NULL) {
        sRet = AX_ERR_VDEC_UNKNOWN;
        SAMPLE_CRIT_LOG("pstVdecUserPic == NULL\n");
        goto ERR_RET;
    }

    pstVdecCtx = pstFuncArgs->pstCtx;
    if (NULL == pstVdecCtx) {
        sRet = AX_ERR_VDEC_UNKNOWN;
        SAMPLE_CRIT_LOG("VdGrp=%d, pstFuncArgs->pstCtx == NULL\n", VdGrp);
        goto ERR_RET;
    }

    pstCmd = &pstVdecCtx->tCmdParam;
    pstVdecUserPic->recvStmAfUsrPic = pstCmd->recvStmAfUsrPic;
    pstVdecUserPic->s32RecvPicNumBak = pstCmd->sRecvPicNum;
    pstUsrPicArgs = &pstFuncArgs->tUsrPicArgs;
    pstUsrPicArgs->VdGrp = VdGrp;
    pstUsrPicArgs->bUsrInstant = pstCmd->bUsrInstant;
    pstUsrPicArgs->enDecType = pstCmd->enDecType;
    pstUsrPicArgs->pstVdecUserPic = pstVdecUserPic;
    for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
        if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;
        pstUsrPicArgs->tChnParam[VdChn].bChnEnable = AX_TRUE;
        pstUsrPicArgs->tChnParam[VdChn].u32PicWidth = pstCmd->tChnCfg[VdChn].u32PicWidth;
        pstUsrPicArgs->tChnParam[VdChn].u32PicHeight = pstCmd->tChnCfg[VdChn].u32PicHeight;
        pstUsrPicArgs->tChnParam[VdChn].enImgFormat = pstCmd->tChnCfg[VdChn].enImgFormat;
        pstUsrPicArgs->tChnParam[VdChn].pUsrPicFilePath = pstCmd->tChnCfg[VdChn].pUsrPicFilePath;
        pstUsrPicArgs->tChnParam[VdChn].bUserPicEnable = pstCmd->tChnCfg[VdChn].bUserPicEnable;
    }

ERR_RET:
    return sRet;
}

static void *_VdecGroupThreadMain(void *arg)
{
    SAMPLE_VDEC_ARGS_T *pstFuncArgs = (SAMPLE_VDEC_ARGS_T *)arg;
    AX_S32 sRet = 0;
    AX_S32 s32Ret = 0;
    int res = 0;

    AX_VDEC_GRP VdGrp = 0;
    AX_VDEC_CHN VdChn = 0;
    AX_CHAR *sFile = NULL;
    FILE *fInput = NULL;

    off_t inputFileSize = 0;
    size_t read_size = 0;

    AX_U8 *pstStreamMem = NULL;
    AX_VDEC_GRP_ATTR_T *pstVdGrpAttr = NULL;
    SAMPLE_STREAM_BUF_T tStreamBuf = {0};

    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstVdecCtx = NULL;
    AX_U32 tmp_size = 0;
    AX_S32 ErrorCode = 0;
    AX_BOOL bCreateUserPool = AX_FALSE;
    AX_S32 ci = 0;
    AX_U32 uBufSize = 0;

    if (pstFuncArgs == NULL) {
        SAMPLE_CRIT_LOG("pstFuncArgs == NULL\n");
        return NULL;
    }

    VdGrp = pstFuncArgs->VdGrp;
    SAMPLE_LOG("VdGrp=%d begin\n", VdGrp);

    AX_CHAR cPthreadName[16];
    snprintf(cPthreadName, 16, "SampleVdec%d", VdGrp);
    prctl(PR_SET_NAME, cPthreadName);

    pstVdecCtx = pstFuncArgs->pstCtx;
    if (NULL == pstVdecCtx) {
        SAMPLE_CRIT_LOG("VdGrp=%d, pstFuncArgs->pstCtx == NULL\n", VdGrp);
        return NULL;
    }

    pstVdecCtx->GrpPID[VdGrp] = gettid();

    pstCmd = &pstVdecCtx->tCmdParam;

    sFile = pstCmd->pInputFilePath;
    if (sFile == NULL) {
        SAMPLE_CRIT_LOG("VdGrp=%d, Please input pInputFilePath\n", VdGrp);
        return NULL;
    }

    fInput = fopen(sFile, "rb");
    if (fInput == NULL) {
        SAMPLE_CRIT_LOG("VdGrp=%d, Unable to open input file:%s\n", VdGrp, sFile);
        return NULL;
    }

    res = fseek(fInput, 0L, SEEK_END);
    if (res) {
        SAMPLE_CRIT_LOG("VdGrp=%d, fseek FAILED! ret:%d\n", VdGrp, res);
        goto ERR_RET_FCLOSE;
    }

    inputFileSize = ftello(fInput);
    rewind(fInput);
    pstVdecCtx->pInputFd[VdGrp] = fInput;
    pstVdecCtx->oInputFileSize[VdGrp] = inputFileSize;

    if (pstCmd->bDynRes) {
        sFile = pstCmd->pNewInputFilePath;
        if (sFile == NULL) {
            SAMPLE_CRIT_LOG("VdGrp=%d, Please input pInputFilePath\n", VdGrp);
            return NULL;
        }

        fInput = fopen(sFile, "rb");
        if (fInput == NULL) {
            SAMPLE_CRIT_LOG("VdGrp=%d, Unable to open input file:%s\n", VdGrp, sFile);
            return NULL;
        }

        res = fseek(fInput, 0L, SEEK_END);
        if (res) {
            SAMPLE_CRIT_LOG("VdGrp=%d, fseek FAILED! ret:%d\n", VdGrp, res);
            goto ERR_RET_FCLOSE;
        }

        inputFileSize = ftello(fInput);
        rewind(fInput);
        pstVdecCtx->pNewInputFd[VdGrp] = fInput;
        pstVdecCtx->oNewInputFileSize[VdGrp] = inputFileSize;
    }


    SAMPLE_BITSTREAM_INFO_T *pstBitStreamInfo = &pstVdecCtx->stBitStreamInfo;
    inputFileSize = pstVdecCtx->oInputFileSize[VdGrp] > pstVdecCtx->oNewInputFileSize[VdGrp] ?
                        pstVdecCtx->oInputFileSize[VdGrp] : pstVdecCtx->oNewInputFileSize[VdGrp];
    SAMPLE_LOG("bDynRes:%d file:%s - %s, pInputFd:%p - %p, FileSize: %ld :%ld - %ld",
               pstCmd->bDynRes, pstCmd->pInputFilePath, pstCmd->pNewInputFilePath,
               pstVdecCtx->pInputFd[VdGrp], pstVdecCtx->pNewInputFd[VdGrp],inputFileSize,
               pstVdecCtx->oInputFileSize[VdGrp], pstVdecCtx->oNewInputFileSize[VdGrp]);

    if (pstCmd->bFfmpegEnable) {
        if ((pstCmd->enInputMode !=AX_VDEC_INPUT_MODE_STREAM)
                && ((pstCmd->enDecType == PT_H264) || (pstCmd->enDecType == PT_H265))) {
            pstBitStreamInfo->VdGrp = VdGrp;
            s32Ret = SampleVdecFfmpegInit(&pstVdecCtx->stFfmpeg, pstCmd->pInputFilePath, pstBitStreamInfo);
            if (s32Ret != AX_SUCCESS) {
                SAMPLE_CRIT_LOG("VdGrp=%d, SampleVdecFfmpegInit FAILED! ret:0x%x\n",
                            VdGrp, s32Ret);
                goto ERR_RET_FCLOSE;
            }
        }
    }

    if (pstCmd->highRes) {
        tStreamBuf.uBufSize = STREAM_BUFFER_MAX_SIZE_HIGH_RES;
    }
    else {
        if (pstCmd->enDecType == PT_JPEG) {
            uBufSize = inputFileSize > STREAM_BUFFER_MIN_SIZE ? inputFileSize : STREAM_BUFFER_MIN_SIZE;
            tStreamBuf.uBufSize = uBufSize > STREAM_BUFFER_MAX_SIZE ? STREAM_BUFFER_MAX_SIZE : uBufSize;
        } else {
            tStreamBuf.uBufSize = inputFileSize > STREAM_BUFFER_MAX_SIZE ? STREAM_BUFFER_MAX_SIZE : inputFileSize;
            if (pstCmd->enInputMode ==AX_VDEC_INPUT_MODE_STREAM) {
                tStreamBuf.uBufSize = (pstCmd->sStreamSize * 2) > STREAM_BUFFER_MAX_SIZE ?
                                            (pstCmd->sStreamSize * 2) : STREAM_BUFFER_MAX_SIZE;
            }
        }
    }

    s32Ret = AX_SYS_MemAlloc(&tStreamBuf.tBufAddr.u64PhyAddr, (AX_VOID **)&tStreamBuf.tBufAddr.pVirAddr,
                             tStreamBuf.uBufSize, 0x100, (AX_S8 *)"vdec_input_stream");
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, AX_SYS_MemAlloc FAILED! uBufSize:0x%x ret:0x%x\n",
                         VdGrp, tStreamBuf.uBufSize, s32Ret);
        goto ERR_RET_FFMPEG_DEINIT;
    } else {
        if ((tStreamBuf.tBufAddr.pVirAddr == NULL) || (tStreamBuf.tBufAddr.u64PhyAddr == 0)) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_SYS_MemAlloc FAILED! uBufSize:%d",
                            VdGrp, tStreamBuf.uBufSize);
            goto ERR_RET_FFMPEG_DEINIT;
        }
    }


    SAMPLE_LOG("VdGrp=%d, AX_SYS_MemAlloc uBufSize:0x%x tBufAddr.pVirAddr:%p, .tBufAddr.u64PhyAddr:0x%llx\n",
               VdGrp, tStreamBuf.uBufSize, tStreamBuf.tBufAddr.pVirAddr, tStreamBuf.tBufAddr.u64PhyAddr);

    tStreamBuf.pBufBeforeFill = tStreamBuf.tBufAddr.pVirAddr;
    tStreamBuf.pBufAfterFill = tStreamBuf.tBufAddr.pVirAddr;
    tStreamBuf.bRingbuf = AX_FALSE;

    pstStreamMem = (AX_U8 *)tStreamBuf.tBufAddr.pVirAddr;

    if (pstCmd->enDecType == PT_H264) {
        if ((pstCmd->tChnCfg[ci].u32PicWidth == 0) || (pstCmd->tChnCfg[ci].u32PicHeight == 0)) {
            SAMPLE_H264_SPS_DATA_T sps_data;
            AX_U32 parse_len = pstVdecCtx->oInputFileSize[VdGrp] >= SEEK_NALU_MAX_LEN ?
                                    SEEK_NALU_MAX_LEN : pstVdecCtx->oInputFileSize[VdGrp];
            memset(&sps_data, 0, sizeof(SAMPLE_H264_SPS_DATA_T));

            read_size = fread(pstStreamMem, sizeof(AX_U8), (size_t)parse_len, fInput);
            if (read_size != parse_len) {
                SAMPLE_CRIT_LOG("fread FAILED! read_size:0x%lx != parse_len:0x%x\n",
                                read_size, parse_len);
                goto ERR_RET_MEMFREE;
            }
            rewind(fInput);

            sRet = h264_parse_sps(pstStreamMem, parse_len, &sps_data);
            SAMPLE_LOG_T("h264_parse_sps sRet:0x%x sps_data.height:%d, sps_data.width:%d parse_len:%d",
                    sRet, sps_data.height, sps_data.width, parse_len);
            if (sRet == AX_SUCCESS) {
                for (ci = 0; ci < AX_DEC_MAX_CHN_NUM; ci++) {
                    if (pstCmd->tChnCfg[ci].enOutputMode == AX_VDEC_OUTPUT_ORIGINAL) {
                        pstCmd->tChnCfg[ci].u32PicWidth = sps_data.width;
                        pstCmd->tChnCfg[ci].u32PicHeight = sps_data.height;
                    } else {
                        if (pstCmd->tChnCfg[ci].u32PicWidth == 0) {
                            pstCmd->tChnCfg[ci].u32PicWidth = pstCmd->u32MaxPicWidth;
                        }

                        if (pstCmd->tChnCfg[ci].u32PicHeight == 0) {
                            pstCmd->tChnCfg[ci].u32PicHeight = pstCmd->u32MaxPicHeight;
                        }
                    }
                }

                if (pstCmd->u32MaxPicWidth < sps_data.width) {
                    pstCmd->u32MaxPicWidth = sps_data.width;
                }

                if (pstCmd->u32MaxPicHeight < sps_data.height) {
                    pstCmd->u32MaxPicHeight = sps_data.height;
                }
            } else {
                for (ci = 0; ci < AX_DEC_MAX_CHN_NUM; ci++) {
                    if (pstCmd->tChnCfg[ci].u32PicWidth == 0) {
                        pstCmd->tChnCfg[ci].u32PicWidth = pstCmd->u32MaxPicWidth;
                    }

                    if (pstCmd->tChnCfg[ci].u32PicHeight == 0) {
                        pstCmd->tChnCfg[ci].u32PicHeight = pstCmd->u32MaxPicHeight;
                    }
                }
            }
        }
    } else {
        for (ci = 0; ci < AX_DEC_MAX_CHN_NUM; ci++) {
            if (pstCmd->tChnCfg[ci].u32PicWidth == 0) {
                pstCmd->tChnCfg[ci].u32PicWidth = pstCmd->u32MaxPicWidth;
            }

            if (pstCmd->tChnCfg[ci].u32PicHeight == 0) {
                pstCmd->tChnCfg[ci].u32PicHeight = pstCmd->u32MaxPicHeight;
            }
        }
    }

    for (ci = 0; ci < AX_DEC_MAX_CHN_NUM; ci++) {
        if (!pstCmd->tChnCfg[ci].bChnEnable) continue;
        if (pstCmd->tChnCfg[ci].u32PicWidth == 0) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pstCmd->tChnCfg[%d].u32PicWidth == 0\n",
                            VdGrp, ci);
        }

        if (pstCmd->tChnCfg[ci].u32PicHeight == 0) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pstCmd->tChnCfg[%d].u32PicHeight == 0\n",
                            VdGrp, ci);
        }
    }

    pstVdGrpAttr = &pstFuncArgs->tVdGrpAttr;

    if (pstCmd->bQuitWait == AX_TRUE) {
        pstVdecCtx->bGrpQuitWait[VdGrp] = AX_TRUE;
    }

    sRet = __VdecGrpCreate(VdGrp, pstVdGrpAttr, pstCmd, tStreamBuf.uBufSize);
    if (sRet != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, __VdecGrpCreate FAILED! ret:0x%x\n",
                        VdGrp, VdChn, sRet);
        goto ERR_RET_MEMFREE;
    }

    pstVdecCtx->GrpStatus[VdGrp] = AX_VDEC_GRP_CREATED;
    SAMPLE_LOG("VdGrp=%d, AX_VDEC_CreateGrp done!", VdGrp);

    tmp_size = sizeof(AX_VDEC_CHN_ATTR_T) * AX_DEC_MAX_CHN_NUM;

    AX_VDEC_CHN_ATTR_T *pstChnSet = calloc(1, tmp_size);
    if (pstChnSet == NULL) {
        SAMPLE_CRIT_LOG("calloc FAILED! size:0x%x\n", tmp_size);
        goto ERR_RET_DESTROY_GRP;
    }

    sRet = __VdecChnAttrEnable(pstFuncArgs, pstChnSet);
    if (sRet != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, __VdecChnAttrEnable FAILED! ret:0x%x\n",
                        VdGrp, VdChn, sRet);
        goto ERR_RET_CHNATTR_FREE;
    }

    if (pstVdGrpAttr->bSdkAutoFramePool == AX_FALSE) {
        sRet = __VdecCreateUserPool(pstFuncArgs, pstChnSet);
        if (sRet != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, __VdecCreateUserPool FAILED! ret:0x%x\n",
                            VdGrp, VdChn, sRet);
            goto ERR_RET_CHNATTR_FREE;
        }
        bCreateUserPool = AX_TRUE;
    }

    AX_VDEC_GRP_PARAM_T stGrpParam;
    memset(&stGrpParam, 0, sizeof(stGrpParam));
    stGrpParam.stVdecVideoParam.enOutputOrder = pstCmd->enOutputOrder;
    stGrpParam.stVdecVideoParam.enVdecMode = pstCmd->enVideoMode;
    stGrpParam.f32SrcFrmRate = pstCmd->f32SrcFrmRate;
    sRet = AX_VDEC_SetGrpParam(VdGrp, &stGrpParam);
    if (0 != sRet) {
        SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_SetGrpParam fail, ret = 0x%x", VdGrp, sRet);
        goto ERR_RET_DETACH_POOL;
    }

    sRet = __VdecInitUsrPicArgs(pstFuncArgs, &pstVdecCtx->stVdecUserPic);
    if (sRet != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, __VdecInitUsrPicArgs failed. ret:0x%x %s\n",
                         VdGrp, sRet, AX_VdecRetStr(sRet));
        goto ERR_RET_DETACH_POOL;
    }

    sRet = __VdecUsrPicCreat(&pstFuncArgs->tUsrPicArgs, &pstVdecCtx->stVdecUserPic);
    if (sRet != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, __VdecUsrPicCreat failed. ret:0x%x %s\n",
                         VdGrp, sRet, AX_VdecRetStr(sRet));
        goto ERR_RET_DETACH_POOL;
    }

    s_groupSuccess = AX_TRUE;

    sRet = __VdecGrpSendStream(pstFuncArgs, &tStreamBuf);
    if (sRet != AX_SUCCESS) {
        goto ERR_RET_DISABLE_USER_PIC;
    }

    SAMPLE_LOG("VdGrp=%d, bQuitWait:%d, s_ThreadExit:%d, bRecvFlowEnd:%d bGrpQuitWait:%d\n",
               VdGrp, pstCmd->bQuitWait, s_ThreadExit,
               pstVdecCtx->bRecvFlowEnd, pstVdecCtx->bGrpQuitWait[VdGrp]);

    if (pstCmd->bQuitWait == AX_TRUE) {
        SAMPLE_LOG_T("VdecGrpSendStream Finished! Now waiting RecvFlowEnd");
    }


    while (1) {
        if (s_ThreadExit > 0) {
            SAMPLE_LOG_T("VdGrp=%d, s_ThreadExit:%d, break while(1)!", VdGrp, s_ThreadExit);
            break;
        }

        sleep(2);

        if ((AX_TRUE == pstVdecCtx->bRecvFlowEnd) && (AX_FALSE == pstVdecCtx->bGrpQuitWait[VdGrp])) {
            SAMPLE_LOG_T("VdGrp=%d, bRecvFlowEnd break while(1)! \n", VdGrp);
            break;
        }
        SAMPLE_LOG("VdGrp=%d, bRecvFlowEnd:%d bGrpQuitWait:%d, s_ThreadExit:%d\n",
                   VdGrp, pstVdecCtx->bRecvFlowEnd, pstVdecCtx->bGrpQuitWait[VdGrp], s_ThreadExit);
    }

    if (pstVdGrpAttr->bSdkAutoFramePool == AX_FALSE) {
        for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
            if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

            sRet = AX_VDEC_DetachPool(VdGrp, VdChn);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, PoolId[VdChn]:%d, AX_VDEC_DetachPool FAILED! ret:0x%x %s",
                                VdGrp, VdChn, pstFuncArgs->PoolId[VdChn], sRet, AX_VdecRetStr(sRet));
            }
        }
    }

    if (s_ThreadExit == 0) {
        s_ThreadExit += 1;
        SAMPLE_NOTICE_LOG("++s_ThreadExit:%d\n", s_ThreadExit);
    }

    SAMPLE_LOG("VdGrp=%d, s_ThreadExit:%d AX_VDEC_DestroyGrp enter ++++", VdGrp, s_ThreadExit);
    while (1) {
        sRet = AX_VDEC_DestroyGrp(VdGrp);
        if (sRet == AX_ERR_VDEC_BUSY) {
            SAMPLE_WARN_LOG("VdGrp=%d, AX_VDEC_DestroyGrp FAILED! ret:0x%x %s",
                           VdGrp, sRet, AX_VdecRetStr(sRet));
            usleep(10000);
            if (pstCmd->sWriteFrames) {
                sleep(1);
            }

            continue;
        }

        if (sRet == AX_SUCCESS) {
            pstVdecCtx->GrpStatus[VdGrp] = AX_VDEC_GRP_DESTROYED;
        } else {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_DestroyGrp FAILED! ret:0x%x %s",
                            VdGrp, sRet, AX_VdecRetStr(sRet));
            ErrorCode |= sRet;
        }

        break;
    }

    SAMPLE_LOG("VdGrp=%d, AX_VDEC_DestroyGrp exit ---- ret:0x%x", VdGrp, sRet);

    if (pstVdGrpAttr->bSdkAutoFramePool == AX_FALSE) {
        for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
            if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

            if (pstFuncArgs->PoolId[VdChn] == AX_INVALID_POOLID) continue;

            sRet = AX_POOL_DestroyPool(pstFuncArgs->PoolId[VdChn]);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, PoolId[VdChn]:%d, AX_POOL_DestroyPool FAILED! ret:0x%x %s",
                                VdGrp, VdChn, pstFuncArgs->PoolId[VdChn], sRet, AX_VdecRetStr(sRet));
            }
        }
    }

    if (pstCmd->bFfmpegEnable) {
        if ((pstCmd->enInputMode != AX_VDEC_INPUT_MODE_STREAM)
                && ((pstCmd->enDecType == PT_H264) || (pstCmd->enDecType == PT_H265))) {
            s32Ret = SampleVdecFfmpegDeinit(&pstVdecCtx->stFfmpeg, pstBitStreamInfo->VdGrp);
            if (s32Ret != AX_SUCCESS) {
                SAMPLE_CRIT_LOG("VdGrp=%d, SampleVdecFfmpegDeinit FAILED! ret:0x%x\n",
                            VdGrp, s32Ret);
                goto ERR_RET_FCLOSE;
            }
        }
    }


    if (pstChnSet) {
        free(pstChnSet);
        pstChnSet = NULL;
    }

    for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
        pstFuncArgs->pstVdChnAttr[VdChn] = NULL;
    }

    if (pstStreamMem) {
        if (tStreamBuf.tBufAddr.u64PhyAddr != 0) {
            sRet = AX_SYS_MemFree(tStreamBuf.tBufAddr.u64PhyAddr, tStreamBuf.tBufAddr.pVirAddr);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, AX_SYS_MemFree FAILED! ret:0x%x", VdGrp, sRet);
                ErrorCode |= sRet;
            }
        } else {
            free(tStreamBuf.tBufAddr.pVirAddr);
        }
    }

    if (fInput) {
        fclose(fInput);
        fInput = NULL;
    }

    return NULL;


ERR_RET_DISABLE_USER_PIC:
    VdecUserPicDestroy(VdGrp, pstFuncArgs->tUsrPicArgs.pstVdecUserPic);
ERR_RET_DETACH_POOL:
    if (pstVdGrpAttr->bSdkAutoFramePool == AX_FALSE) {
        for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
            if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

            sRet = AX_VDEC_DetachPool(VdGrp, VdChn);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, PoolId[VdChn]:%d, AX_VDEC_DetachPool FAILED! ret:0x%x %s",
                                VdGrp, VdChn, pstFuncArgs->PoolId[VdChn], sRet, AX_VdecRetStr(sRet));
            }
        }
    }
ERR_RET_CHNATTR_FREE:
    if (pstChnSet) {
        free(pstChnSet);
        pstChnSet = NULL;
    }

    for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
        pstFuncArgs->pstVdChnAttr[VdChn] = NULL;
    }
ERR_RET_DESTROY_GRP:
    while (1) {
        sRet = AX_VDEC_DestroyGrp(VdGrp);
        if (sRet == AX_ERR_VDEC_BUSY) {
            SAMPLE_WARN_LOG("VdGrp=%d, AX_VDEC_DestroyGrp FAILED! ret:0x%x %s",
                           VdGrp, sRet, AX_VdecRetStr(sRet));
            usleep(10000);
            if (pstCmd->sWriteFrames) {
                sleep(1);
            }
            continue;
        }

        if (sRet == AX_SUCCESS) {
            pstVdecCtx->GrpStatus[VdGrp] = AX_VDEC_GRP_DESTROYED;
        } else {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_DestroyGrp FAILED! ret:0x%x %s",
                            VdGrp, sRet, AX_VdecRetStr(sRet));
            ErrorCode |= sRet;
        }

        break;
    }

    if ((bCreateUserPool) && pstVdGrpAttr->bSdkAutoFramePool == AX_FALSE) {
        for (VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
            if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

            if (pstFuncArgs->PoolId[VdChn] == AX_INVALID_POOLID) continue;

            sRet = AX_POOL_DestroyPool(pstFuncArgs->PoolId[VdChn]);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, PoolId[VdChn]:%d, AX_POOL_DestroyPool FAILED! ret:0x%x %s",
                                VdGrp, VdChn, pstFuncArgs->PoolId[VdChn], sRet, AX_VdecRetStr(sRet));
            }
        }
    }
ERR_RET_MEMFREE:
    if (pstStreamMem) {
        if (tStreamBuf.tBufAddr.u64PhyAddr != 0) {
            sRet = AX_SYS_MemFree(tStreamBuf.tBufAddr.u64PhyAddr, tStreamBuf.tBufAddr.pVirAddr);
            if (sRet) {
                SAMPLE_CRIT_LOG("VdGrp=%d, AX_SYS_MemFree FAILED! ret:0x%x", VdGrp, sRet);
            }
        } else {
            free(tStreamBuf.tBufAddr.pVirAddr);
            tStreamBuf.tBufAddr.pVirAddr = NULL;
        }
    }
ERR_RET_FFMPEG_DEINIT:
    if (pstCmd->bFfmpegEnable) {
        if ((pstCmd->enInputMode !=AX_VDEC_INPUT_MODE_STREAM)
                && ((pstCmd->enDecType == PT_H264) || (pstCmd->enDecType == PT_H265))) {
            s32Ret = SampleVdecFfmpegDeinit(&pstVdecCtx->stFfmpeg, pstBitStreamInfo->VdGrp);
            if (s32Ret != AX_SUCCESS) {
                SAMPLE_CRIT_LOG("VdGrp=%d, SampleVdecFfmpegDeinit FAILED! ret:0x%x\n",
                            VdGrp, s32Ret);
            }
        }
    }

ERR_RET_FCLOSE:
    if (fInput) {
        fclose(fInput);
        fInput = NULL;
    }

    return NULL;
}

static int _VdecRecvProcess(SAMPLE_VDEC_CONTEXT_T *pstVdecCtx, SAMPLE_VDEC_GRP_SET_ARGS_T *pstGrpSetArgs)
{
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    AX_S32 s32Ret = 0;
    int gi;
    AX_VDEC_GRP VdGrp = 0;

    SAMPLE_VDEC_GRP_CHN_RECV_ARGS_T stGrpRecvArgs;
    SAMPLE_VDEC_ARGS_T *pstFuncArgs = NULL;

    if (pstVdecCtx == NULL) {
        SAMPLE_CRIT_LOG("pstSampleCtx == NULL\n");
        return -1;
    }

    pstCmd = &pstVdecCtx->tCmdParam;

    if (pstCmd->enSelectMode == AX_VDEC_SELECT_MODE_DISABLE) {
        SAMPLE_LOG_N("tmp_size:%d, pstCmd->uGrpCount:%d", tmp_size, pstCmd->uGrpCount);

        for (gi = 0; gi < pstCmd->uGrpCount; gi++) {

            VdGrp = gi;
            pstFuncArgs = &pstGrpSetArgs->stVdecGrpArgs[gi];

            for (int VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
                if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

                stGrpRecvArgs.VdGrp = VdGrp;
                stGrpRecvArgs.VdChn = VdChn;
                stGrpRecvArgs.pstCtx = pstFuncArgs->pstCtx;

                SAMPLE_LOG("VdGrp=%d, VdChn=%d, pstCtx:%p", VdGrp, VdChn, stGrpRecvArgs.pstCtx);
                /* create thread for get chn frame */
                if (pthread_create(&pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn],
                                   NULL, __VdecGrpChnRecvThread, &stGrpRecvArgs) != 0) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, pthread_create __VdecGrpChnRecvThread FAILED!\n",
                                    VdGrp, VdChn);
                    goto ERR_RET;
                }

                SAMPLE_LOG_N("VdGrp=%d, VdChn=%d, pthread_create __VdecGrpChnRecvThread done! pthread_id:0x%lx!\n",
                            VdGrp, VdChn, pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn]);
            }
        }
    }
    else if (pstCmd->enSelectMode == AX_VDEC_SELECT_MODE_PRIVATE) {
        if (pthread_create(&pstVdecCtx->RecvTid, NULL, _VdecRecvThread, pstGrpSetArgs) != 0) {
            SAMPLE_CRIT_LOG("pthread_create _VdecRecvThread FAILED!\n");
            s32Ret = AX_ERR_VDEC_RUN_ERROR;
            goto ERR_RET;
        }
    }
    else {
        SAMPLE_CRIT_LOG("VdGrp=%d, Unsupport enSelectMode:%d!\n",
                        VdGrp, pstCmd->enSelectMode);
        s32Ret = AX_ERR_VDEC_NOT_SUPPORT;
    }

ERR_RET:
    return s32Ret;
}


static int _VdecTestMain(SAMPLE_VDEC_CONTEXT_T *pstVdecCtx, SAMPLE_VDEC_GRP_SET_ARGS_T *pstGrpSetArgs)
{
    int i;
    int ret = 0;
    AX_U32 GrpNum = 0;
    AX_S32 s32Ret = 0;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_ARGS_T *pstFuncArgs = NULL;
    AX_VDEC_GRP VdGrp = 0;

    if (pstVdecCtx == NULL) {
        SAMPLE_CRIT_LOG("pstSampleCtx == NULL\n");
        return -1;
    }

    pstCmd = &pstVdecCtx->tCmdParam;

    s32Ret = _VdecRecvProcess(pstVdecCtx, pstGrpSetArgs);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_SUCCESS FAILED! ret:0x%x\n", s32Ret);
        goto ERR_RET;
    }

    for (i = 0; i < pstCmd->uGrpCount; i++) {
        GrpNum = i;
        pstFuncArgs = &pstGrpSetArgs->stVdecGrpArgs[i];
        ret = pthread_create(&pstVdecCtx->GrpTid[i], NULL, _VdecGroupThreadMain,
                             (void *)pstFuncArgs);
        if (ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pthread_create _VdecGroupThreadMain FAILED! ret:%d, %s, \n",
                            i, ret, strerror(ret));
            s32Ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET_CANCEL_PTHREAD_RECV;
        }
        SAMPLE_LOG("VdGrp=%d, pthread_create _VdecGroupThread done! pthread_id:0x%lx \n",
                   i, pstVdecCtx->GrpTid[i]);
    }

    for (i = 0; i < pstCmd->uGrpCount; i++) {
        SAMPLE_LOG("VdGrp=%d, before pthread_join _VdecGroupThread pthread_id:0x%lx, PID:%d\n",
                   i, pstVdecCtx->GrpTid[i], pstVdecCtx->GrpPID[i]);

        ret = pthread_join(pstVdecCtx->GrpTid[i], NULL);
        if (ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pthread_join FAILED! ret:%d, %s, \n",
                            i, ret, strerror(ret));
            s32Ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET_CANCEL_PTHREAD_RECV;
        }
        SAMPLE_LOG("VdGrp=%d, pthread_join _VdecGroupThread done! pthread_id:0x%lx, PID:%d \n",
                   i, pstVdecCtx->GrpTid[i], pstVdecCtx->GrpPID[i]);
        pstVdecCtx->GrpTid[i] = 0;
    }


    if (pstFuncArgs->pstCtx->tCmdParam.enSelectMode == AX_VDEC_SELECT_MODE_DISABLE) {
        for (int gi = 0; gi < pstCmd->uGrpCount; gi++) {
            for (int VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
                if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

                SAMPLE_LOG("VdGrp=%d, VdChn=%d, before pthread_join pthread_id:0x%lx",
                            VdGrp, VdChn, pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn]);

                ret = pthread_join(pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn], NULL);
                if (ret) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, pthread_join __VdecGrpChnRecvThread FAILED! ret:%d, %s, \n",
                                    VdGrp, VdChn, ret, strerror(ret));
                    s32Ret = AX_ERR_VDEC_UNKNOWN;
                    goto ERR_RET_CANCEL_PTHREAD_DEC;
                } else {
                    SAMPLE_LOG("VdGrp=%d, VdChn=%d, pthread_join __VdecGrpChnRecvThread done! pthread_id:0x%lx",
                                VdGrp, VdChn, pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn]);
                    pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn] = 0;
                }
            }
        }
    }
    else if (pstCmd->enSelectMode == AX_VDEC_SELECT_MODE_PRIVATE) {
        pstVdecCtx->RecvThdWait = AX_TRUE;
        ret = pthread_join(pstVdecCtx->RecvTid, NULL);
        if (ret) {
            SAMPLE_CRIT_LOG("pthread_join FAILED! ret:%d, %s, \n",
                            ret, strerror(ret));
            s32Ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET;
        }
    }

    return s32Ret;

ERR_RET_CANCEL_PTHREAD_RECV:
    if (pstVdecCtx->tCmdParam.enSelectMode == AX_VDEC_SELECT_MODE_DISABLE) {
        for (int gi = 0; gi < pstCmd->uGrpCount; gi++) {
            for (int VdChn = 0; VdChn < AX_DEC_MAX_CHN_NUM; VdChn++) {
                if (pstFuncArgs->bChnEnable[VdChn] == AX_FALSE) continue;

                VdGrp = gi;
                ret = pthread_cancel(pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn]);
                if (ret) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, pthread_cancel __VdecGrpChnRecvThread FAILED! ret:%d, %s, \n",
                                    VdGrp, VdChn, ret, strerror(ret));
                    s32Ret = AX_ERR_VDEC_UNKNOWN;
                    goto ERR_RET;
                }

                ret = pthread_join(pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn], NULL);
                if (ret) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, pthread_join __VdecGrpChnRecvThread FAILED! ret:%d, %s, \n",
                                    VdGrp, VdChn, ret, strerror(ret));
                    s32Ret = AX_ERR_VDEC_UNKNOWN;
                    goto ERR_RET;
                }
                SAMPLE_LOG("VdGrp=%d, VdChn=%d, pthread_join __VdecGrpChnRecvThread done! pthread_id:0x%lx",
                            VdGrp, VdChn, pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn]);
                pstVdecCtx->GrpChnRecvTid[VdGrp][VdChn] = 0;
            }
        }
    }
    else if (pstVdecCtx->tCmdParam.enSelectMode == AX_VDEC_SELECT_MODE_PRIVATE) {
        ret = pthread_cancel(pstVdecCtx->RecvTid);
        if (ret) {
            SAMPLE_CRIT_LOG("pthread_cancel FAILED! ret:%d, %s, \n",
                            ret, strerror(ret));
            s32Ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET;
        }

        ret = pthread_join(pstVdecCtx->RecvTid, NULL);
        if (ret) {
            SAMPLE_CRIT_LOG("pthread_join FAILED! ret:%d, %s, \n",
                            ret, strerror(ret));
            s32Ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET;
        }
    }

ERR_RET_CANCEL_PTHREAD_DEC:
    for (i = 0; i < GrpNum; i++) {
        ret = pthread_cancel(pstVdecCtx->GrpTid[i]);
        if (ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pthread_cancel FAILED! ret:%d, %s, \n",
                            i, ret, strerror(ret));
            goto ERR_RET;
        }

        ret = pthread_join(pstVdecCtx->GrpTid[i], NULL);
        if (ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pthread_join FAILED! ret:%d, %s, \n",
                            i, ret, strerror(ret));
            goto ERR_RET;
        }
    }
ERR_RET:
    return s32Ret;
}

int Sample_VdecTestBenchMain(SAMPLE_VDEC_CONTEXT_T **ppstSampleCtx)
{
    int i;
    AX_S32 s32Ret = 0;
    SAMPLE_VDEC_GRP_SET_ARGS_T stGrpSetArgs;
    SAMPLE_VDEC_ARGS_T *pstVdecGrpArgs = NULL;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    AX_U32 tmp_size = 0;
    AX_U32 uStreamCount = 0;
    SAMPLE_VDEC_CONTEXT_T *pstVdecCtx = NULL;
    AX_VDEC_GRP uStartGrpId = 0;

    if (ppstSampleCtx == NULL) {
        SAMPLE_CRIT_LOG("ppstSampleCtx == NULL\n");
        return -1;
    }

    pstVdecCtx = *ppstSampleCtx;
    if (pstVdecCtx == NULL) {
        SAMPLE_CRIT_LOG("pstSampleCtx == NULL\n");
        return -1;
    }

    pstCmd = &pstVdecCtx->tCmdParam;

    uStreamCount = pstCmd->uStreamCount;
    uStartGrpId = pstCmd->uStartGrpId;
    memset(&stGrpSetArgs, 0x0, sizeof(SAMPLE_VDEC_GRP_SET_ARGS_T));
    for (i = 0; i < pstCmd->uGrpCount; i++) {
        pstVdecGrpArgs = &stGrpSetArgs.stVdecGrpArgs[i];
        pstVdecGrpArgs->VdGrp = i + uStartGrpId;

        if (i == 0) {
            pstVdecGrpArgs->pstCtx = pstVdecCtx;
        } else {
            tmp_size = sizeof(SAMPLE_VDEC_CONTEXT_T);
            pstVdecGrpArgs->pstCtx = (SAMPLE_VDEC_CONTEXT_T *)calloc(1, tmp_size);
            if (NULL == pstVdecGrpArgs->pstCtx) {
                SAMPLE_CRIT_LOG("calloc FAILED! size:0x%x\n", tmp_size);
                s32Ret = AX_ERR_VDEC_NOMEM;
                goto ERR_RET_ARGS;
            }

            if (uStreamCount > 0) {
                s32Ret = VdecDefaultParamsSet(&pstVdecGrpArgs->pstCtx->tCmdParam);
                if (s32Ret) {
                    SAMPLE_CRIT_LOG("VdecDefaultParamsSet FAILED! ret:0x%x\n", s32Ret);
                    goto ERR_RET_ARGS;
                }

                uStreamCount--;
            } else {
                memcpy(pstVdecGrpArgs->pstCtx, pstVdecCtx, sizeof(SAMPLE_VDEC_CONTEXT_T));
                pstVdecGrpArgs->pstCtx->tCmdParam.tChnCfg[0].pOutputFilePath = NULL;
                pstVdecGrpArgs->pstCtx->tCmdParam.pTbCfgFilePath = NULL;
            }
        }

        if (pstCmd->pGrpCmdlFile[i] == NULL) {
            pstCmd->pGrpCmdlFile[i] = pstCmd->pGrpCmdlFile[0];
        }

        s32Ret = VdecParseStreamCfg(pstCmd->pGrpCmdlFile[i], pstVdecGrpArgs->pstCtx, i);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, VdecParseStreamCfg FAILED! ret:%d, \n", i, s32Ret);
            goto ERR_RET_ARGS;
        }

        s32Ret = OpenTestHooks(pstVdecGrpArgs->pstCtx);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, OpenTestHooks FAILED! ret:%d, \n", i, s32Ret);
            goto ERR_RET_ARGS;
        }

        pstVdecGrpArgs->tVdGrpAttr.enCodecType = pstVdecGrpArgs->pstCtx->tCmdParam.enDecType;

        for (int ci = 0; ci < AX_VDEC_MAX_CHN_NUM; ci++) {
            pstVdecGrpArgs->bChnEnable[ci] = pstCmd->tChnCfg[ci].bChnEnable;
            // SAMPLE_LOG("VdGrp=%d, VdChn=%d, bChnEnable=%d", i, ci, pstCmd->tChnCfg[ci].bChnEnable);
        }
    }


    SAMPLE_LOG_N("pstVdecCtx:%p, .stVdecGrpArgs[0].pstCtx:%p, "
                "pstVdecGrpArgs->tVdGrpAttr.enCodecType:%d",
                pstVdecCtx, stGrpSetArgs.stVdecGrpArgs[0].pstCtx,
                pstVdecGrpArgs->tVdGrpAttr.enCodecType);

    if (s_ThreadExit) {
        SAMPLE_LOG("s_ThreadExit:%d, so goto exit", s_ThreadExit);
        goto ERR_RET_ARGS;
    }

    s32Ret = _VdecTestMain(pstVdecCtx, &stGrpSetArgs);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("VdGrp=%d, _VdecTestMain FAILED! ret:0x%x, %s\n",
                        i, s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_ARGS;
    }


    return s32Ret;


ERR_RET_ARGS:
    uStreamCount = pstCmd->uStreamCount;
    for (i = 0; i < uStreamCount; i++) {
        pstVdecGrpArgs = &stGrpSetArgs.stVdecGrpArgs[i];

        if (NULL != pstVdecGrpArgs) {
            pstVdecCtx = pstVdecGrpArgs->pstCtx;
            if (pstVdecCtx != NULL) {
                if (pstVdecCtx->bArgvAlloc == AX_TRUE) {
                    if (pstVdecCtx->argv != NULL) {
                        if (pstVdecCtx->argv[0] != NULL) {
                            free(pstVdecCtx->argv[0]);
                            pstVdecCtx->argv[0] = NULL;
                        }

                        free(pstVdecCtx->argv);
                        pstVdecCtx->argv = NULL;
                    }

                    pstVdecCtx->bArgvAlloc = AX_FALSE;
                }

                free(pstVdecCtx);
                pstVdecCtx = NULL;
            }

            pstVdecGrpArgs = NULL;
        }
    }


    *ppstSampleCtx = NULL;
    return s32Ret;
}


static void _VdecSigInt(int sigNo)
{
#if 1
    AX_S32 s32Ret = 0;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstVdecCtx = NULL;
    AX_VDEC_GRP uStartGrpId = 0;
    AX_VDEC_GRP VdGrp = 0;
    AX_VDEC_GRP_STATUS_T stGrpStatus;

    if (s_pstVdecCtx == NULL) {
        s_ThreadExit += 1;
        SAMPLE_NOTICE_LOG("s_pstVdecCtx == NULL\n");
        goto ERR_RET;
    }

    pstVdecCtx = (SAMPLE_VDEC_CONTEXT_T *)s_pstVdecCtx;

    pstCmd = &pstVdecCtx->tCmdParam;

    uStartGrpId = pstCmd->uStartGrpId;
    for (VdGrp = uStartGrpId; VdGrp < (pstCmd->uGrpCount + uStartGrpId); VdGrp++) {
        memset(&stGrpStatus, 0, sizeof(stGrpStatus));
        s32Ret = AX_VDEC_QueryStatus(VdGrp, &stGrpStatus);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_QueryStatus ret:0x%x %s\n",
                            VdGrp, s32Ret, AX_VdecRetStr(s32Ret));
            continue;
        }

        if (stGrpStatus.bStartRecvStream) {
            s32Ret = AX_VDEC_StopRecvStream(VdGrp);
            if (s32Ret) {
                SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_StopRecvStream ret:0x%x %s\n",
                                VdGrp, s32Ret, AX_VdecRetStr(s32Ret));
                continue;
            }
        }

        while (1) {
            s32Ret = AX_VDEC_ResetGrp(VdGrp);
            if (s32Ret != AX_ERR_VDEC_BUSY) {
                break;
            }
        }

        if (s32Ret) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_ResetGrp ret:0x%x %s\n",
                            VdGrp, s32Ret, AX_VdecRetStr(s32Ret));
            continue;
        }

        pstVdecCtx->GrpStatus[VdGrp] = AX_VDEC_GRP_RESET;
    }
ERR_RET:
#endif
    s_ThreadExit += 1;

    SAMPLE_LOG_ST("Catch signal %d, s_ThreadExit:%d ==============\n\n\n", sigNo, s_ThreadExit);

    if (s_ThreadExit > 3) {
        exit(0);
    }

    return;
}


AX_S32 Sample_VdecTestBenchInit(int argc, char *argv[], SAMPLE_VDEC_CONTEXT_T **ppstVdecCtx)
{
    AX_S32 s32Ret = 0;
    pid_t pid, ppid;
    struct timeval current_tv;
    SAMPLE_VDEC_CONTEXT_T *pstVdecCtx = NULL;
    int gi = 0;
    int ci = 0;

    signal(SIGINT, _VdecSigInt); /* ctrl + c */
    signal(SIGQUIT, _VdecSigInt); /* ctrl + \ */
    signal(SIGTSTP, _VdecSigInt); /* ctrl + z */

    gettimeofday(&current_tv, NULL);
    pid = getpid();
    ppid = getppid();

    AX_CHAR cPthreadName[16];
    snprintf(cPthreadName, 16, "%s", argv[0]);
    prctl(PR_SET_NAME, cPthreadName);

    SAMPLE_LOG_ST("Start! pid:%d, ppid:%d, date:%s, time:%s, current_tv.tv_sec:%ld",
                  pid, ppid, __DATE__, __TIME__, current_tv.tv_sec);

    AX_CHAR cTmp[1024] = {0};
    AX_CHAR *pcTmp = cTmp;
    int len = 0;
    for (int ii = 0; ii < argc; ii++) {
        len = strlen(argv[ii]);
        strcat(pcTmp, argv[ii]);
        pcTmp += len;
        strcat(pcTmp, " ");
        pcTmp += 1;
    }

    SAMPLE_LOG_ST("cmd:%s \n", cTmp);

    pstVdecCtx = (SAMPLE_VDEC_CONTEXT_T *)calloc(1, sizeof(SAMPLE_VDEC_CONTEXT_T));
    if (NULL == pstVdecCtx) {
        SAMPLE_CRIT_LOG("calloc FAILED! size:0x%lx\n", sizeof(SAMPLE_VDEC_CONTEXT_T));
        return -1;
    }

    SAMPLE_LOG("pstVdecCtx:%s \n", pstVdecCtx);

    pstVdecCtx->argc = argc;
    pstVdecCtx->argv = argv;

#ifdef AX_VDEC_POOL_REFCNT_TEST

    int bi;
    for (gi = 0; gi < AX_VDEC_MAX_GRP_NUM; gi++) {
        for (bi = 0; bi < SAMPLE_VDEC_REF_BLK_CNT; bi++) {
            pstVdecCtx->blkRef[gi][bi] = AX_INVALID_BLOCKID;
        }
    }
#endif

    for (gi = 0; gi < AX_VDEC_MAX_GRP_NUM; gi++) {
        for (ci = 0; ci < AX_DEC_MAX_CHN_NUM; ci++) {
            s_u64GetFrameNum[gi][ci] = 0;
        }
    }

    *ppstVdecCtx = pstVdecCtx;
    s_pstVdecCtx = pstVdecCtx;

    return s32Ret;
}


int Sample_VdecJpegDecodeOneFrame(SAMPLE_VDEC_CMD_PARAM_T *pstCmd)
{
    int res = 0;
    AX_S32 s32Ret = 0;
    AX_S32 sRet = 0;
    AX_U64 streamPhyAddr = 0;
    AX_VOID *pStreamVirAddr = NULL;
    AX_U64 outPhyAddrDst = 0;
    AX_VOID *outVirAddrDst = NULL;
    AX_S32 heightAlign = 0;
    AX_S32 frmStride = 0;

    FILE *fInput = NULL;
    FILE *fp_out = NULL;
    AX_U32 uBufSize = 0;
    off_t inputFileSize = 0;
    AX_CHAR *streamFile = NULL;
    AX_VDEC_DEC_ONE_FRM_T decOneFrmParam;
    SAMPLE_INPUT_FILE_INFO_T stStreamInfo;
    SAMPLE_STREAM_BUF_T stStreamBuf;
    size_t sReadLen = 0;
    SAMPLE_VDEC_OUTPUT_INFO_T out_info;
    AX_VIDEO_FRAME_INFO_T stFrameInfo;
    AX_U32 uPixBits = 0;

    if (NULL == pstCmd) {
        SAMPLE_CRIT_LOG("NULL == pstCmd\n");
        s32Ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    if (pstCmd->enDecType != PT_JPEG) {
        SAMPLE_CRIT_LOG("only support for jpeg\n");
        s32Ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    streamFile = pstCmd->pInputFilePath;
    if (NULL == streamFile) {
        SAMPLE_CRIT_LOG("NULL == streamFile\n");
        s32Ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    memset(&decOneFrmParam, 0x0, sizeof(AX_VDEC_DEC_ONE_FRM_T));
    memset(&stStreamInfo, 0x0, sizeof(SAMPLE_INPUT_FILE_INFO_T));
    memset(&stStreamBuf, 0x0, sizeof(SAMPLE_STREAM_BUF_T));
    memset(&out_info, 0x0, sizeof(SAMPLE_VDEC_OUTPUT_INFO_T));
    memset(&stFrameInfo, 0x0, sizeof(AX_VIDEO_FRAME_INFO_T));
    /* Reading input file */
    fInput = fopen(streamFile, "rb");
    if (fInput == NULL) {
        SAMPLE_CRIT_LOG("Unable to open input stream file:%s\n", streamFile);
        s32Ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET;
    }

    /* file i/o pointer to full */
    res = fseek(fInput, 0L, SEEK_END);
    if (res) {
        SAMPLE_CRIT_LOG("fseek FAILED! ret:%d\n", res);
        s32Ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET_CLOSE_IN;
    }

    inputFileSize = ftello(fInput);
    rewind(fInput);

    if (pstCmd->highRes)
        stStreamBuf.uBufSize = STREAM_BUFFER_MAX_SIZE_HIGH_RES;
    else
        stStreamBuf.uBufSize = inputFileSize > STREAM_BUFFER_MAX_SIZE ? STREAM_BUFFER_MAX_SIZE : inputFileSize;
    s32Ret = AX_SYS_MemAlloc(&streamPhyAddr, (AX_VOID **)&pStreamVirAddr,
                             stStreamBuf.uBufSize, 0x100, (AX_S8 *)"vdec_input_stream");
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_SYS_MemAlloc FAILED! uBufSize:0x%x ret:0x%x\n",
                        stStreamBuf.uBufSize, s32Ret);
        goto ERR_RET_CLOSE_IN;
    }

    stStreamBuf.tBufAddr.pVirAddr = pStreamVirAddr;
    stStreamBuf.tBufAddr.u64PhyAddr = streamPhyAddr;

    uPixBits = 8;
    frmStride = AX_COMM_ALIGN(pstCmd->tChnCfg[0].u32PicWidth * uPixBits, AX_JDEC_WIDTH_ALIGN * 8) / 8;
    heightAlign = ALIGN_UP(pstCmd->tChnCfg[0].u32PicHeight, 2);
    // uBufSize = widthAlign * heightAlign * 3 / 2;
    uBufSize = AX_VDEC_GetPicBufferSize(heightAlign, frmStride, AX_FORMAT_YUV420_PLANAR, NULL,
                                        pstCmd->enDecType);
    s32Ret = AX_SYS_MemAlloc(&outPhyAddrDst, (AX_VOID **)&outVirAddrDst,
                             uBufSize, 0x1000, (AX_S8 *)"vdec_output_YUV");
    if (s32Ret != 0) {
        SAMPLE_CRIT_LOG("AX_SYS_MemAlloc FAILED! uBufSize:0x%x ret:0x%x\n",
                       uBufSize, s32Ret);
        goto ERR_RET_FREE_STREAM;
    }

    stStreamInfo.fInput = fInput;
    stStreamInfo.sFileSize = inputFileSize;

    sRet = StreamParserReadFrameJpeg(&stStreamInfo, &stStreamBuf, &sReadLen);
    if (sRet) {
        SAMPLE_CRIT_LOG("StreamParserReadFrameJpeg FAILED! ret:0x%x\n", sRet);
        s32Ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET_FREE_OUT;
    }

    if (!sReadLen) {
        SAMPLE_CRIT_LOG("read jpeg frame FAILED!\n");
        s32Ret = AX_ERR_VDEC_UNKNOWN;
        goto ERR_RET_FREE_OUT;
    }

    decOneFrmParam.stStream.pu8Addr = stStreamBuf.tBufAddr.pVirAddr;
    decOneFrmParam.stStream.u64PhyAddr = stStreamBuf.tBufAddr.u64PhyAddr;
    decOneFrmParam.stStream.u32StreamPackLen = (AX_U32)sReadLen;

    decOneFrmParam.stFrame.u64VirAddr[0] = (AX_U64)outVirAddrDst;
    decOneFrmParam.stFrame.u64VirAddr[1] = (AX_U64)outVirAddrDst + frmStride * heightAlign;
    decOneFrmParam.stFrame.u64PhyAddr[0] = outPhyAddrDst;
    decOneFrmParam.stFrame.u64PhyAddr[1] = outPhyAddrDst + frmStride * heightAlign;
    decOneFrmParam.enImgFormat = pstCmd->tChnCfg[0].enImgFormat;
    decOneFrmParam.enOutputMode = pstCmd->tChnCfg[0].enOutputMode;
    if (AX_VDEC_OUTPUT_CROP == decOneFrmParam.enOutputMode) {
        decOneFrmParam.stFrame.s16CropX = pstCmd->tChnCfg[0].u32CropX;
        decOneFrmParam.stFrame.s16CropY = pstCmd->tChnCfg[0].u32CropY;
        decOneFrmParam.stFrame.s16CropWidth = pstCmd->tChnCfg[0].u32PicWidth;
        decOneFrmParam.stFrame.s16CropHeight = pstCmd->tChnCfg[0].u32PicHeight;
    }


    s32Ret = AX_VDEC_JpegDecodeOneFrame(&decOneFrmParam);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_VDEC_JpegDecodeOneFrame FAILED! ret:0x%x %s\n",
                        s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_FREE_OUT;
    }

    if (pstCmd->sWriteFrames || pstCmd->DestMD5) {
        out_info.enImgFormat = decOneFrmParam.stFrame.enImgFormat;
        out_info.u32Width = decOneFrmParam.stFrame.u32Width;
        out_info.u32Height = decOneFrmParam.stFrame.u32Height;
        out_info.u32PicStride = decOneFrmParam.stFrame.u32PicStride[0];
        out_info.bOneShot = AX_TRUE;
        fp_out = OutputFileOpen(&pstCmd->tChnCfg[0].pOutputFilePath, &out_info);
        if (fp_out == NULL) {
            if (pstCmd->tChnCfg[0].pOutputFilePath != NULL) {
                SAMPLE_CRIT_LOG("fopen pstCmd->pOutputFilePath:%s FAILED!",
                                pstCmd->tChnCfg[0].pOutputFilePath);
                free(pstCmd->tChnCfg[0].pOutputFilePath);
                pstCmd->tChnCfg[0].pOutputFilePath = NULL;
            }
            s32Ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET_FREE_OUT;
        }

        stFrameInfo.stVFrame = decOneFrmParam.stFrame;
        s32Ret = OutputFileSaveYUV(0, 0, &stFrameInfo, fp_out, pstCmd->tChnCfg[0].pOutputFilePath);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("OutputFileSaveYUV FAILED! ret:0x%x", s32Ret);
            s32Ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET_FREE_OUT;
        }
    }

    if (fInput != NULL) {
        res = fclose(fInput);
        if (res) {
            SAMPLE_CRIT_LOG("fclose FAILED! ret:%d\n", res);
            sRet = AX_ERR_VDEC_UNKNOWN;
        }
        fInput = NULL;
    }

    if (fp_out != NULL) {
        res = fclose(fp_out);
        if (res) {
            SAMPLE_CRIT_LOG("fclose FAILED! ret:%d\n", res);
            sRet = AX_ERR_VDEC_UNKNOWN;
        }
        fp_out = NULL;
    }

    s32Ret = AX_SYS_MemFree(streamPhyAddr, pStreamVirAddr);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_SYS_MemFree streamPhyAddr FAILED! s32Ret:0x%x\n", s32Ret);
    }

    s32Ret = AX_SYS_MemFree(outPhyAddrDst, (AX_VOID *)outVirAddrDst);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_SYS_MemFree outPhyAddrDst FAILED! s32Ret:0x%x\n", s32Ret);
    }

    if (s32Ret || sRet) {
        goto ERR_RET;
    }

    g_u64GetFrmTag += 1;

    return 0;


ERR_RET_FREE_OUT:
    if (outVirAddrDst != NULL) {
        sRet = AX_SYS_MemFree(outPhyAddrDst, (AX_VOID *)outVirAddrDst);
        if(sRet != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("AX_SYS_MemFree outPhyAddrDst FAILED! sRet:0x%x\n", sRet);
        }
    }
ERR_RET_FREE_STREAM:
    if (pStreamVirAddr != NULL) {
        sRet = AX_SYS_MemFree(streamPhyAddr, pStreamVirAddr);
        if(sRet != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("AX_SYS_MemFree streamPhyAddr FAILED! sRet:0x%x\n", sRet);
        }
    }
ERR_RET_CLOSE_IN:
    if (fInput) {
        res = fclose(fInput);
        if (res) {
            SAMPLE_CRIT_LOG("fclose FAILED! ret:%d\n", res);
        }
        fInput = NULL;
    }
ERR_RET:
    SAMPLE_CRIT_LOG("s32Ret:0x%x, sRet:0x%x", s32Ret, sRet);
    return s32Ret || sRet;
}
