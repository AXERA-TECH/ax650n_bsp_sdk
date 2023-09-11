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
#include <string.h>
#include <ctype.h>

#include "common_vdec_cmd.h"



static enum TBCfgCallbackResult __TBReadParam(char *block, char *key, char *value,
                                              enum TBCfgCallbackParam state, void *cb_param)
{
    if (block == NULL || key == NULL || value == NULL || cb_param == NULL) {
        SAMPLE_CRIT_LOG("block == NULL || key == NULL || value == NULL || cb_param == NULL");
        return TB_CFG_ERROR;
    }

    // SAMPLE_VDEC_TBCFG_PARAM_T *pstTbCfg = (SAMPLE_VDEC_TBCFG_PARAM_T *)cb_param;
    // SAMPLE_LOG("block:%s, key:%s, value:%s", block, key, value);

    switch (state) {
    case TB_CFG_CALLBACK_BLK_START:
        break;
    case TB_CFG_CALLBACK_VALUE:
        break;
    default:
        break;
    }
    return TB_CFG_OK;
}



AX_S32 OpenTestHooks(const SAMPLE_VDEC_CONTEXT_T *pstVdecCtx)
{
    char* file_name = NULL;
    AX_BOOL result = AX_FALSE;

    if (pstVdecCtx == NULL) {
        SAMPLE_CRIT_LOG("pstVdecCtx == NULL");
        return -1;
    }

    if (pstVdecCtx->tCmdParam.pTbCfgFilePath == NULL) {
        file_name = "./tb.cfg";
    } else {
        file_name = pstVdecCtx->tCmdParam.pTbCfgFilePath;
    }

    SAMPLE_LOG("tb_cfg_file: %s\n", file_name);

    FILE* f_tbcfg = fopen(file_name, "r");
    if (f_tbcfg == NULL) {
        SAMPLE_LOG("UNABLE TO OPEN INPUT FILE: %s\n", file_name);
        SAMPLE_LOG("USING DEFAULT CONFIGURATION\n");
    } else {
        SAMPLE_LOG("open pTbCfgFilePath:%s ok!", file_name);

        fclose(f_tbcfg);

        result = TBParseConfig(file_name, __TBReadParam, (void *)&pstVdecCtx->tTbCfg);
        if (result == AX_FALSE) {
            SAMPLE_CRIT_LOG("TBParseConfig failed");
            return -1;
        }

        SAMPLE_LOG("TBParseConfig Done!\n");
    }

    return 0;
}


// Parse cropping parameters from argument string:
// --crop wxh@[x,y]
// --crop=wxh@[x,y]
// Return 1 for illegal string, otherwise return 0.
static int __ParseCropParams(char *optarg, AX_U32 *x, AX_U32 *y, AX_U32 *w, AX_U32 *h) {
    char *p = optarg;
    char *q = p;
    while (*p && isdigit(*p)) p++;
    if (!*p || *p != 'x') return 1;
    //*p++ = '\0';
    p++; *w = atoi(q); q = p;
    while (*p && isdigit(*p)) p++;
    if (!*p || *p != '@') return 1;
    //*p++ = '\0';
    p++; *h = atoi(q); q = p;
    if (!*p || *p != '[') return 1;
    q = ++p;
    while (*p && isdigit(*p)) p++;
    if (!*p || *p != ',') return 1;
    //*p++ = '\0';
    p++; *x = atoi(q); q = p;
    while (*p && isdigit(*p)) p++;
    if (!*p || *p != ']') return 1;
    //*p++ = '\0';
    p++; *y = atoi(q); q = p;
    if (*p) return 1;
    return 0;
}

// Parse scaling parameters from argument string:
//   --scale wxh
//   --scale=wxh
// Return 1 for illegal string, otherwise return 0.
static int __ParseScaleParams(char *optarg, AX_U32 *w, AX_U32 *h, AX_U32 *rx, AX_U32 *ry) {
    char *p = optarg;
    char *q = p;
    if (*p && p[0] == '-' && p[1] == 'd') {
        p += 2; q = p;
        while (*p && isdigit(*p)) p++;
        if (!*p) {
            *rx = *ry = atoi(q);
            return 0;
        } else if (*p != ':') return 1;
        *p++ = '\0'; *rx = atoi(q); q = p;
        while (*p && isdigit(*p)) p++;
        if (*p) return 1;
        *ry = atoi(q);
        *w = *h = 0;
    } else {
        while (*p && isdigit(*p)) p++;
        if (!*p || *p != 'x') return 1;
        *p++ = '\0'; *w = atoi(q); q = p;
        while (*p && isdigit(*p)) p++;
        if (*p) return 1;
        *p++ = '\0'; *h = atoi(q); q = p;
        *rx = *ry = 0;
    }
    return 0;
}

static int __ParseResParams(char *optarg, AX_U32 *w, AX_U32 *h) {
    char *p = optarg;
    char *q = p;

    while (*p && isdigit(*p)) p++;
    if (!*p || *p != 'x') return 1;
    *p++ = '\0'; *w = atoi(q); q = p;
    while (*p && isdigit(*p)) p++;
    if (*p) return 1;
    *p++ = '\0'; *h = atoi(q); q = p;
    return 0;
}

AX_S32 VdecParseStreamCfg(const char *streamcfg, SAMPLE_VDEC_CONTEXT_T *pstVdecCtx, AX_VDEC_GRP VdGrp)
{
    int ret;
    int i;
    char *p;
    FILE *fp = NULL;
    AX_S32 s32Ret = 0;
    size_t read_size = 0;
    AX_CHAR *pRead = NULL;

    if (streamcfg == NULL) {
        SAMPLE_LOG("VdGrp=%d, streamcfg == NULL", VdGrp);
        return 0;
    }

    if (pstVdecCtx == NULL) {
        SAMPLE_CRIT_LOG("pstVdecCtx == NULL");
        return -1;
    }

    fp = fopen(streamcfg, "r");
    if (fp == NULL) {
        SAMPLE_CRIT_LOG("VdGrp=%d, fopen(streamcfg:%s); == NULL",
                        VdGrp, streamcfg);
        return -1;
    }

    if ((ret = fseeko(fp, 0, SEEK_END)) < 0) {
        SAMPLE_CRIT_LOG("fseek ret:%d", ret);
        s32Ret = -1;
        goto ERR_RET;
    }

    off_t fsize = ftello(fp);
    if (fsize < 0) {
        SAMPLE_CRIT_LOG("ftello fsize:%ld", fsize);
        s32Ret = -1;
        goto ERR_RET;
    }

    if ((ret = fseeko(fp, 0, SEEK_SET)) < 0) {
        SAMPLE_CRIT_LOG("ret:%d", ret);
        s32Ret = -1;
        goto ERR_RET;
    }

    pRead = calloc(1, fsize + 2);
    if (pRead == NULL) {
        SAMPLE_CRIT_LOG("calloc fsize:%ld", fsize);
        s32Ret = -1;
        goto ERR_RET;
    }

    if ((ret = fseeko(fp, 0, SEEK_SET)) < 0) {
        SAMPLE_CRIT_LOG("ret:%d", ret);
        s32Ret = -1;
        goto ERR_RET_FREE;
    }

    read_size = fread(pRead, 1, fsize, fp);
    if (read_size != fsize) {
        SAMPLE_CRIT_LOG("fread ret:%ld, fsize:%ld", read_size, fsize);
        s32Ret = -1;
        goto ERR_RET_FREE;
    }

    AX_U32 tmp_size = SAMPLE_VDEC_MAX_ARGS * sizeof(char *);
    pstVdecCtx->argv = (char **)calloc(1, tmp_size+1);
    if (pstVdecCtx->argv == NULL) {
        SAMPLE_CRIT_LOG("calloc tmp_size:%d", tmp_size);
        s32Ret = -1;
        goto ERR_RET_FREE;
    }

    pstVdecCtx->argv[0] = pRead;
    pstVdecCtx->bArgvAlloc = AX_TRUE;

    fclose(fp);
    fp = NULL;

    p = pstVdecCtx->argv[0];
    for (i = 1; i < SAMPLE_VDEC_MAX_ARGS; i++) {
        SAMPLE_LOG_N("i:%d, p:%s", i, p);
        while (*p && *p <= 32)
            ++p;
        if (!*p) break;
        pstVdecCtx->argv[i] = p;
        while (*p > 32)
            ++p;
        *p = 0; ++p;
    }
    pstVdecCtx->argc = i;

    for (i = 0; i < pstVdecCtx->argc; i++) {
        SAMPLE_LOG_N("pstVdecCtx->argc:%d, pstVdecCtx->argv[%d]:%s",
                  pstVdecCtx->argc, i, pstVdecCtx->argv[i]);
    }

    ret = VdecCmdLineParseAndCheck(pstVdecCtx->argc, pstVdecCtx->argv,
                                   &pstVdecCtx->tCmdParam, VdGrp, 0);
    if (ret) {
        SAMPLE_CRIT_LOG("VdGrp=%d, VdecCmdLineParseAndCheck ret:%d", VdGrp, ret);
        s32Ret = -1;
        goto ERR_RET_FREE_ARG;
    }

    if (pRead != NULL) {
        free(pRead);
        pRead = NULL;
    }

    if (pstVdecCtx->argv != NULL) {
        free(pstVdecCtx->argv);
        pstVdecCtx->argv = NULL;
    }

    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;

    pstCmd = &pstVdecCtx->tCmdParam;
    SAMPLE_LOG("VdGrp=%d, pstCmd->pInputFilePath:%s\n",
               VdGrp, pstCmd->pInputFilePath);

    return 0;

ERR_RET_FREE_ARG:
    if (pstVdecCtx->argv != NULL) {
        free(pstVdecCtx->argv);
        pstVdecCtx->argv = NULL;
    }
ERR_RET_FREE:
    if (pRead != NULL) {
        free(pRead);
        pRead = NULL;
    }
ERR_RET:
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    return s32Ret;
}

extern char *__progname;

static void __PrintHelp_Com()
{
    printf("usage: %s -i streamFile <args>\n", __progname);
    printf("args:\n");

    printf("  -c:       group count. (1-uMaxGrpCnt), default: 1\n");
    printf("  -L:       loop decode number. (int), default: 1\n");
    printf("  -N:       receive decode number. (int), default: <= 0, no limit\n");
    printf("  -w:       write YUV frame to file. (0: not write, others: write), default: 0\n");
    printf("  -m:       check MD5 every frame. (0: not output md5, other: md5), default: 0\n");
    printf("  -W:       max output buffer width. (for pool GetPicBufferSize), default: 8192\n");
    printf("  -H:       max outbut buffer height. (for pool GetPicBufferSize), default: 8192\n");
    printf("  -i:       input file. user specified input bitstream file path), \n");
    printf("  -o:       output file. (user specified output yuv file path), default: ./out.yuv\n");
    printf("  -M:       video mode. (3: stream, 1: frame), default: 1\n");
    printf("  -T:       video type. (96: PT_H264, 265: PT_H265, 26: PT_JPEG), default: 96 (PT_H264)\n");
    printf("  -j:       whether test jpeg single frame decoding function. \n"
            "               (0: not test, 1: test), default: 0\n");
    printf("  -q:       whether to wait input 'q' to quit program when finish stream decoding. \n"
            "               (0: not wait, 1: wait), default: 0\n");
    printf("  -s:       if video mode is stream, parameter is valid, and is send stream size, \n"
            "               Byte. it is less than stream buffer size 3M Byte, default: 1048576(1M) Byte\n");
    printf("\n");
    printf("  --sMilliSec:          receive timeout flag.  (-1: block, 0: unblock, > 0: msec), default: -1\n");
    printf("  --select:             select mode. (0: disable, 1: AX select, 2: Posix select\n");

    printf("  --VdChn:              channel id. (default enable VdChn 0. \n"
            "                           Video decode have 3 hardware channel, \n"
            "                           Jpeg decode have 1 hardware channel\n");
    printf("  --res:                stream resolution (--res=WidthxHeight, default: 1920x1080.\n");
    printf("  --scale:              Enable scaling. (--scale=WidthxHeight.\n");
    printf("  --crop:               Enable cropping. (--crop=WidthxHeight@[x,y], from [x,y] with size WidthxHeight.\n");
    printf("  --outFormat:          outFormat. (0: YUV400, 3: NV12(for 8-bit/10-bit stream), \n"
            "                           4: NV21(for 8-bit/10-bit stream). \n"
            "                           42:10bitP010, 40:10bitY/U/V 4 pixels in 5 bytes. default: 3\n");
    printf("  --highRes:            for high resolution test(for only one group of jdec).  \n"
            "                           (0: disable, 1: enable. default: 0\n");
    printf("  --nstream:            stream count. (0-uMaxGrpCnt), default: 0\n");
    printf("  --streamcfg:          stream config file path. (user specified stream config file path),\n");
    printf("  --fbc_complevel:      yuv fbc compress level. (0-10), default: 0\n");
    printf("  --uMaxGrpCnt:         max group count. (0-164), default: 164\n");
    printf("  --uStartGrpId:        start group id. (0-164), default: 0\n");
    printf("  --bDynRes:            dynamic resolution test of jdec. \n"
            "                           1: enable. 0: disable. default: 0\n");
    printf("  --newInput:           another input file for jdec dynamic resolution test.\n");
    printf("  --f32SrcFrmRate:      decoder input frame rate. default: 0\n");
    printf("  --f32DstFrmRate:      decoder output frame rate. default: 0\n");
    printf("  --bFrmRateCtrl:       decoder output frame rate enable. default: 0,FALSE\n");

    printf("  --enDisplayMode:      display mode. (0: preview mode, 1: playback mode), default: 1\n");
    printf("  --enDecModule:        decoder hw module. (0: both vdec and jdec, 1: only vdec, 2: only jdec), default: 0\n");
    printf("  --uOutputFifoDepth:   output fifo depth. (0-34), default: 4\n");
    printf("  --enFrameBufSrc:      output frame buf source  (1: private pool, 2: user pool), default: 1, \n");
    printf("  --u32FrameBufCnt:     frame buffer count. (default: 8, \n");
    printf("  --bPerfTest:          whether do performance test \n"
            "                           (now support for jdec). 1: enable. 0: disable. default: 0\n");
    printf("  --bGetUserData:       whether get user data when decode. 1: enable. 0: disable. default: 0\n");
    printf("  --bGetRbInfo:         whether get input ringbuf info when decode. 1: enable. 0: disable. default: 0\n");
    printf("  --bQueryStatus:       whether query group decode status when decode. 1: enable. 0: disable. default: 0\n");
    printf("  --bGetVuiParam:       whether get vui param when decode. 1: enable. 0: disable. default: 0\n");
    printf("  --usrPicFile:         user picture file. (for inserting user picture.\n");
    printf("  --usrPicIdx:          Specifies which frame to insert the user picture after. (default: 7\n");
    printf("  --bUsrInstant         whether insert user picture instantly. (1: enable. 0: disable. default: 0\n");
    printf("  --recvStmAfUsrPic     whether to start recv stream after inserting user picture. \n"
            "                           (1: enable. 0: disable. default: 0\n");
    printf("  --bFfmpegEnable:      whether used ffmpeg lib parser stream to decode. 1: enable. 0: disable. default: 1\n");
    printf("  --enOutputOrder:      Output order. 0: OUTPUT_ORDER_DISP. 1: OUTPUT_ORDER_DEC.  default: 0\n");
    printf("  --enVideoMode:        Video Mode. 0: VIDEO_DEC_MODE_IPB. 1: VIDEO_DEC_MODE_IP. 2: VIDEO_DEC_MODE_I. default: 0\n");
}

static void __PrintHelp_Link()
{
    printf("  --pollingEna:         whether to enable the polling function. 1: enable. 0: disable. default: 0\n");
    printf("  --pollingCnt:         polling number. default: 0\n");
    printf("  --pollingTime:        polling time. default: 10\n");
}

static AX_S32 __SampleInitOptions(SAMPLE_OPTION_T **ppOptions, SAMPLE_OPTION_NAME_T **ppOptName)
{
    int i = 0;
    int j = 0;
    AX_S32 ret = 0;
    SAMPLE_OPTION_T *pOptions = NULL;
    AX_U32 tmp_size = 0;
    SAMPLE_OPTION_NAME_T *pOptName = NULL;

    if (ppOptions == NULL) {
        SAMPLE_CRIT_LOG("ppOptions == NULL");
        return -1;
    }

    tmp_size = sizeof(SAMPLE_OPTION_T) * SAMPLE_VDEC_OPTION_MAX_COUNT;
    pOptions = calloc(1, tmp_size);
    if (pOptions == NULL) {
        SAMPLE_CRIT_LOG("calloc tmp_size:%d FAILED!", tmp_size);
        return -1;
    }

    ret = SampleOptionsFill(pOptions, i++, "help", 'h', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "uGrpCount", 'c', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "sLoopDecNum", 'L', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "sRecvPicNum", 'N', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "sWriteFrames", 'w', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "DestMD5", 'm', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "u32MaxPicWidth", 'W', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "u32MaxPicHeight", 'H', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "pInputFilePath", 'i', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "pOutputFilePath", 'o', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "enInputMode", 'M', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "sStreamSize", 's', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "enDecType", 'T', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bJpegDecOneFrm", 'j', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bQuitWait", 'q', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "sMilliSec", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "VdChn", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "crop", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "scale", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "res", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bUsrInstant", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "recvStmAfUsrPic", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "usrPicFile", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "usrPicIdx", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "outFormat", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bDynRes", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "newInput", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "highRes", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bPerfTest", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bGetUserData", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bGetRbInfo", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bQueryStatus", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bGetVuiParam", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "outputfile", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "tb_cfg_file", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "f32SrcFrmRate", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "f32DstFrmRate", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bFrmRateCtrl", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "enDisplayMode", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "multimode", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "select", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "streamcfg", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "nstream", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "fbc_complevel", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "uMaxGrpCnt", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "uStartGrpId", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "enDecModule", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "uOutputFifoDepth", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "u32FrameBufCnt", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "enFrameBufSrc", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "bFfmpegEnable", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "waitTime", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "pollingEna", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "pollingCnt", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "pollingTime", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "enOutputOrder", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;
    ret = SampleOptionsFill(pOptions, i++, "enVideoMode", '0', AX_TRUE);
    if (ret) goto ERR_RET_LOG;


    tmp_size = sizeof(SAMPLE_OPTION_NAME_T) * AX_VDEC_MAX_GRP_NUM;
    pOptName = calloc(1, tmp_size);
    if (pOptName == NULL) {
        SAMPLE_CRIT_LOG("calloc tmp_size:%d FAILED!", tmp_size);
        ret = -1;
        goto ERR_RET;
    }

    for (j = 0; j < AX_VDEC_MAX_GRP_NUM; j++) {
        if ((pOptName + j) == NULL) {
            SAMPLE_CRIT_LOG("SampleOptionsFill FAILED! j:%d\n", j);
            ret = -1;
            goto ERR_RET;
        }

        int len = snprintf((pOptName + j)->name, AX_OPTION_NAME_LEN, "%s%d", "grp_cmd_", j);
        if ((len < 0)) {
            SAMPLE_CRIT_LOG("snprintf FAILED! ret:0x%x \n", len);
            ret = -1;
            goto ERR_RET;
        }

        ret = SampleOptionsFill(pOptions, i++, (pOptName + j)->name, '0', AX_TRUE);
        if (ret) {
            SAMPLE_CRIT_LOG("SampleOptionsFill FAILED! i:%d, ret:0x%x \n", i, ret);
            ret = -1;
            goto ERR_RET;
        }

        // SAMPLE_LOG("(pOptName + j:%d):%s ", j, (pOptName + j)->name);
    }

    *ppOptions = pOptions;
    *ppOptName = pOptName;

    return 0;

ERR_RET_LOG:
    SAMPLE_CRIT_LOG("SampleOptionsFill FAILED! i:%d, ret:0x%x \n", i, ret);
ERR_RET:
    if (pOptions != NULL) {
        free(pOptions);
    }

    if (pOptName != NULL) {
        free(pOptName);
    }
    return ret;
}

static AX_S32 __VdecCmdParaPrint(SAMPLE_VDEC_CMD_PARAM_T *pstPara, AX_VDEC_GRP VdGrp)
{
    return 0;

    SAMPLE_VDEC_CHN_CFG_T *pstChnCfg = NULL;

    SAMPLE_LOG("pstPara->uGrpCount:%d", pstPara->uGrpCount);
    SAMPLE_LOG("pstPara->uStreamCount:%d", pstPara->uStreamCount);
    SAMPLE_LOG("pstPara->sLoopDecNum:%d", pstPara->sLoopDecNum);
    SAMPLE_LOG("pstPara->sRecvPicNum:%d", pstPara->sRecvPicNum);
    SAMPLE_LOG("pstPara->sMilliSec:%d", pstPara->sMilliSec);
    SAMPLE_LOG("pstPara->u32MaxPicWidth:%d", pstPara->u32MaxPicWidth);
    SAMPLE_LOG("pstPara->u32MaxPicHeight:%d", pstPara->u32MaxPicHeight);
    SAMPLE_LOG("pstPara->sWriteFrames:%d", pstPara->sWriteFrames);
    SAMPLE_LOG("pstPara->DestMD5:%d", pstPara->DestMD5);
    SAMPLE_LOG("pstPara->bJpegDecOneFrm:%d", pstPara->bJpegDecOneFrm);
    SAMPLE_LOG("pstPara->enMultimode:%d", pstPara->enMultimode);
    SAMPLE_LOG("pstPara->enDecType:%d", pstPara->enDecType);

    SAMPLE_LOG("pstPara->enInputMode:%d", pstPara->enInputMode);
    SAMPLE_LOG("pstPara->sStreamSize:%d", pstPara->sStreamSize);
    SAMPLE_LOG("pstPara->pInputFilePath:%s", pstPara->pInputFilePath);
    SAMPLE_LOG("pstPara->bFfmpegEnable:%d", pstPara->bFfmpegEnable);
    SAMPLE_LOG("pstPara->enVideoMode:%d", pstPara->enVideoMode);

    for (int i = 0; i < AX_DEC_MAX_CHN_NUM; i++) {
        pstChnCfg = &pstPara->tChnCfg[i];

        SAMPLE_LOG("pstChnCfg->VdChn:%d", pstChnCfg->VdChn);
        SAMPLE_LOG("pstChnCfg->u32PicWidth:%d", pstChnCfg->u32PicWidth);
        SAMPLE_LOG("pstChnCfg->u32PicHeight:%d", pstChnCfg->u32PicHeight);
        SAMPLE_LOG("pstChnCfg->u32FrameStride:%d", pstChnCfg->u32FrameStride);
        SAMPLE_LOG("pstChnCfg->u32FramePadding:%d", pstChnCfg->u32FramePadding);
        SAMPLE_LOG("pstChnCfg->u32CropX:%d", pstChnCfg->u32CropX);
        SAMPLE_LOG("pstChnCfg->u32CropY:%d", pstChnCfg->u32CropY);

        SAMPLE_LOG("pstChnCfg->u32ScaleRatioX:%d", pstChnCfg->u32ScaleRatioX);
        SAMPLE_LOG("pstChnCfg->u32ScaleRatioY:%d", pstChnCfg->u32ScaleRatioY);
        SAMPLE_LOG("pstChnCfg->enOutputMode:%d", pstChnCfg->enOutputMode);
        SAMPLE_LOG("pstChnCfg->enImgFormat:%d", pstChnCfg->enImgFormat);
        SAMPLE_LOG("pstChnCfg->uCompressLevel:%d", pstChnCfg->uCompressLevel);
        SAMPLE_LOG("pstChnCfg->u32OutputFifoDepth:%d", pstChnCfg->u32OutputFifoDepth);
    }

    return 0;
}

static AX_VOID __VdecParameterAdjust(SAMPLE_VDEC_CMD_PARAM_T *pstCmd)
{
    if ((PT_JPEG == pstCmd->enDecType) || (PT_MJPEG == pstCmd->enDecType)) {
        pstCmd->u32MaxPicWidth = pstCmd->u32MaxPicWidth > pstCmd->tChnCfg[0].u32PicWidth ?
                                 pstCmd->u32MaxPicWidth : pstCmd->tChnCfg[0].u32PicWidth;
        pstCmd->u32MaxPicHeight = pstCmd->u32MaxPicHeight > pstCmd->tChnCfg[0].u32PicHeight ?
                                  pstCmd->u32MaxPicHeight : pstCmd->tChnCfg[0].u32PicHeight;

        if (pstCmd->enInputMode == AX_VDEC_INPUT_MODE_NAL)
            pstCmd->enInputMode = AX_VDEC_INPUT_MODE_FRAME;
    } else {
        for (int ci = 0; ci < AX_DEC_MAX_CHN_NUM; ci++) {
            if (pstCmd->tChnCfg[ci].bChnEnable == AX_FALSE) continue;

            pstCmd->u32MaxPicWidth = pstCmd->u32MaxPicWidth > pstCmd->tChnCfg[ci].u32PicWidth ?
                                        pstCmd->u32MaxPicWidth : pstCmd->tChnCfg[ci].u32PicWidth;
            pstCmd->u32MaxPicHeight = pstCmd->u32MaxPicHeight > pstCmd->tChnCfg[ci].u32PicHeight ?
                                        pstCmd->u32MaxPicHeight : pstCmd->tChnCfg[ci].u32PicHeight;
        }

        for (int ci = 1; ci < AX_DEC_MAX_CHN_NUM; ci++) {
            if (pstCmd->tChnCfg[ci].bChnEnable == AX_FALSE) continue;
            if (pstCmd->tChnCfg[ci].enOutputMode != AX_VDEC_OUTPUT_ORIGINAL) continue;

            pstCmd->tChnCfg[ci].u32PicWidth = pstCmd->tChnCfg[ci].u32PicWidth ?
                                              pstCmd->tChnCfg[ci].u32PicWidth : pstCmd->tChnCfg[0].u32PicWidth;
            pstCmd->tChnCfg[ci].u32PicHeight = pstCmd->tChnCfg[ci].u32PicHeight ?
                                               pstCmd->tChnCfg[ci].u32PicHeight : pstCmd->tChnCfg[0].u32PicHeight;
        }
    }

    if (pstCmd->highRes) {
        pstCmd->uGrpCount = 1;
    }
}

static AX_S32 __VdecParameterCheck(SAMPLE_VDEC_CMD_PARAM_T *pCml, AX_VDEC_GRP VdGrp)
{
    AX_S32 ret;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = pCml;
    AX_S32 maxStreamBufSize = 0;
    int ci;

    maxStreamBufSize = pstCmd->highRes ? STREAM_BUFFER_MAX_SIZE_HIGH_RES : STREAM_BUFFER_MAX_SIZE;
    if (pstCmd->uGrpCount < 1 || pstCmd->uGrpCount > AX_VDEC_MAX_GRP_NUM) {
        SAMPLE_CRIT_LOG("Invalid group number:%d\n", pstCmd->uGrpCount);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->uMaxGrpCnt < 0) {
        SAMPLE_CRIT_LOG("Invalid group number:%d\n", pstCmd->uMaxGrpCnt);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->uStartGrpId < 0 || pstCmd->uStartGrpId > AX_VDEC_MAX_GRP_NUM) {
        SAMPLE_CRIT_LOG("Invalid group id:%d\n", pstCmd->uStartGrpId);
        ret = -1;
        goto ERR_RET;
    }

    if (NULL == pstCmd->pInputFilePath) {
        if (pstCmd->pGrpCmdlFile[VdGrp] == NULL) {
            SAMPLE_CRIT_LOG("pstCmd->pGrpCmdlFile[%d] == NULL, NULL == pstCmd->pInputFilePath\n",
                           VdGrp);
            ret = -1;
            goto ERR_RET;
        }
    }

    if (pstCmd->uStreamCount > 0) {
        if (pstCmd->pGrpCmdlFile[pstCmd->uStreamCount - 1] == NULL) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pstCmd->pGrpCmdlFile[%d - 1] == NULL\n",
                            VdGrp, pstCmd->uStreamCount);
            ret = -1;
            goto ERR_RET;
        }
    }

    if ((pstCmd->enDecType != PT_H265) && (pstCmd->enDecType != PT_H264) &&
            (pstCmd->enDecType != PT_JPEG) && (pstCmd->enDecType != PT_MJPEG)) {
        SAMPLE_CRIT_LOG("Invalid decode type:%d, AX650 can not supported\n", pstCmd->enDecType);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->enInputMode >= AX_VDEC_INPUT_MODE_BUTT) {
        SAMPLE_CRIT_LOG("Invalid decode mode:%d\n", pstCmd->enInputMode);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->enMultimode >= AX_VDEC_MULTI_MODE_PROCESS) {
        SAMPLE_CRIT_LOG("Invalid multi mode:%d\n", pstCmd->enMultimode);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->enDisplayMode >= AX_VDEC_DISPLAY_MODE_BUTT) {
        SAMPLE_CRIT_LOG("Invalid display mode:%d\n", pstCmd->enDisplayMode);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->enSelectMode >= AX_VDEC_SELECT_MODE_POSIX) {
        SAMPLE_CRIT_LOG("Invalid select mode:%d\n", pstCmd->enSelectMode);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->sStreamSize > maxStreamBufSize) {
        SAMPLE_CRIT_LOG("Invalid stream size:%d, it is bigger than stream buffer size:%d\n",
                        pstCmd->sStreamSize, maxStreamBufSize);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->sStreamSize <= 0) {
        SAMPLE_CRIT_LOG("Invalid stream size:%d\n", pstCmd->sStreamSize);
        ret = -1;
        goto ERR_RET;
    }

    if ((pstCmd->enFrameBufSrc != POOL_SOURCE_PRIVATE)
            && (pstCmd->enFrameBufSrc != POOL_SOURCE_USER)) {
        SAMPLE_CRIT_LOG("Unsupport frame buf source mode:%d\n", pstCmd->enFrameBufSrc);
        ret = -1;
        goto ERR_RET;
    }

    if (pstCmd->uStreamCount > pstCmd->uGrpCount) {
        pstCmd->uGrpCount = pstCmd->uStreamCount;
    }

    if ((pstCmd->enDecModule < AX_ENABLE_BOTH_VDEC_JDEC)
            || (pstCmd->enDecModule > AX_ENABLE_ONLY_JDEC)) {
        SAMPLE_CRIT_LOG("Invalid decode module:%d\n", pstCmd->enDecModule);
        ret = -1;
        goto ERR_RET;
    }


    for (ci = 0; ci < AX_VDEC_MAX_CHN_NUM; ci++) {
        if (pstCmd->tChnCfg[ci].bChnEnable) break;
    }

    if (ci == AX_VDEC_MAX_CHN_NUM) {
        SAMPLE_CRIT_LOG("AX_VDEC_MAX_CHN_NUM:%d is not enabled\n", AX_VDEC_MAX_CHN_NUM);
        ret = -1;
        goto ERR_RET;
    }

    for (ci = 0; ci < AX_VDEC_MAX_CHN_NUM; ci++) {
        if ((pstCmd->tChnCfg[ci].VdChn < 0)
                || (pstCmd->tChnCfg[ci].VdChn > AX_VDEC_MAX_CHN_NUM)) {
            SAMPLE_CRIT_LOG("Invalid tChnCfg[ci:%d].VdChn %d\n", ci, pstCmd->tChnCfg[ci].VdChn);
            ret = -1;
            goto ERR_RET;
        }

        if (pstCmd->tChnCfg[ci].u32PicHeight > pstCmd->u32MaxPicHeight) {
            SAMPLE_CRIT_LOG("tChnCfg[ci:%d].u32PicHeight:%d > pstCmd->u32MaxPicHeight:%d\n",
                           ci, pstCmd->tChnCfg[ci].u32PicHeight, pstCmd->u32MaxPicHeight);
            ret = -1;
            goto ERR_RET;
        }

        if (pstCmd->tChnCfg[ci].u32PicWidth > pstCmd->u32MaxPicWidth) {
            SAMPLE_CRIT_LOG("tChnCfg[ci:%d].u32PicWidth:%d > pstCmd->u32MaxPicWidth:%d\n",
                           ci, pstCmd->tChnCfg[ci].u32PicWidth, pstCmd->u32MaxPicWidth);
            ret = -1;
            goto ERR_RET;
        }

        if (pstCmd->tChnCfg[ci].uCompressLevel > AX_VDEC_FBC_COMPRESS_LEVEL_MAX) {
            SAMPLE_CRIT_LOG("tChnCfg[ci:%d].uCompressLevel:%d > %d\n",
                           ci, pstCmd->tChnCfg[ci].uCompressLevel, AX_VDEC_FBC_COMPRESS_LEVEL_MAX);
            ret = -1;
            goto ERR_RET;
        }

        if ((pstCmd->tChnCfg[ci].u32OutputFifoDepth < AX_VDEC_MIN_OUTPUT_FIFO_DEPTH)
                || (pstCmd->tChnCfg[ci].u32OutputFifoDepth > AX_VDEC_MAX_OUTPUT_FIFO_DEPTH)) {
            SAMPLE_CRIT_LOG("Invalid chn %d output fifo depth:%d\n",
                            ci, pstCmd->tChnCfg[ci].u32OutputFifoDepth);
            ret = -1;
            goto ERR_RET;
        }

        if (pstCmd->tChnCfg[ci].enOutputMode >= AX_VDEC_OUTPUT_MODE_BUTT) {
            SAMPLE_CRIT_LOG("tChnCfg[ci:%d].enOutputMode:%d >= %d\n",
                           ci, pstCmd->tChnCfg[ci].enOutputMode, AX_VDEC_OUTPUT_MODE_BUTT);
            ret = -1;
            goto ERR_RET;
        }

        if (pstCmd->tChnCfg[ci].enImgFormat >= AX_FORMAT_YUV444_PACKED_10BIT_P010) {
            SAMPLE_CRIT_LOG("tChnCfg[ci:%d].enOutputMode:%d >= %d\n",
                           ci, pstCmd->tChnCfg[ci].enImgFormat,
                           AX_FORMAT_YUV444_PACKED_10BIT_P010);
            ret = -1;
            goto ERR_RET;
        }

    }

    return 0;
ERR_RET:

    return ret;
}


AX_S32 VdecCmdLineParseAndCheck(AX_S32 argc, AX_CHAR **argv, SAMPLE_VDEC_CMD_PARAM_T *pstPara,
                                AX_VDEC_GRP VdGrp, AX_BOOL bLink)
{
    SAMPLE_PARAMETER_T prm;
    AX_S32 sRet = 0;
    AX_S32 ret = 0;
    AX_S32 s32Ret = 0;
    AX_CHAR *p;
    SAMPLE_PARAMETER_T *pPrm = &prm;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = pstPara;
    AX_CHAR *optarg;
    AX_CHAR arg[VDEC_CMD_MAX_ARG_LEN];
    AX_S32 argLen = 0;
    size_t slen = 0;
    AX_VDEC_CHN VdChn = 0;
    int len;
    int i;

    SAMPLE_OPTION_T *options = NULL;
    SAMPLE_OPTION_NAME_T *pOptName = NULL;

    static AX_BOOL streamcfg_para = AX_FALSE;

    if (argc < 2) {
        __PrintHelp_Com();
        if (bLink)
            __PrintHelp_Link();
        exit(0);
    }

    for (i = 0; i < argc; i++) {
        SAMPLE_LOG_N("VdGrp=%d, argc:%d, argv[%d]:%s", VdGrp, argc, i, argv[i]);
    }

    ret = __SampleInitOptions(&options, &pOptName);
    if (ret) {
        SAMPLE_CRIT_LOG("VdGrp=%d, __SampleInitOptions FAILED! ret:0x%x", VdGrp, ret);
        goto ERR_RET;
    }

    prm.cnt = 1;
    while ((s32Ret = SampleGetOption(argc, argv, options, &prm)) != -1) {
        if (s32Ret == -2) {
            // SAMPLE_CRIT_LOG("SampleGetOption ret -2");
            sRet = -1;
            goto ERR_RET;
        }

        p = prm.argument;
        optarg = p;

        SAMPLE_LOG_N("VdGrp=%d, pPrm->short_opt:%c, pPrm->longOpt:%s",
                  VdGrp, pPrm->short_opt, pPrm->longOpt);

        switch (pPrm->short_opt) {
        case 'c':
            pstCmd->uGrpCount = atoi(optarg);
            break;
        case 'L':
            pstCmd->sLoopDecNum = atoi(optarg);
            break;
        case 'N':
            pstCmd->sRecvPicNum = atoi(optarg);
            break;
        case 'w':
            pstCmd->sWriteFrames = atoi(optarg);
            break;
        case 'm':
            pstCmd->DestMD5 = atoi(optarg);
            break;
        case 'W':
            pstCmd->u32MaxPicWidth = atoi(optarg);
            break;
        case 'H':
            pstCmd->u32MaxPicHeight = atoi(optarg);
            break;
        case 'i':
            pstCmd->pInputFilePath = malloc(AX_VDEC_FILE_PATH_LEN);
            if (pstCmd->pInputFilePath == NULL) {
                SAMPLE_CRIT_LOG("VdGrp=%d, malloc %d Bytes FAILED!\n",
                                VdGrp, AX_VDEC_FILE_PATH_LEN);
                ret = -3;
                goto ERR_RET;
            }

            slen = strlen(optarg);
            if (slen >= AX_VDEC_FILE_PATH_LEN) {
                SAMPLE_CRIT_LOG("VdGrp=%d, (strlen(optarg):%ld >= AX_VDEC_FILE_PATH_LEN:%d\n",
                               VdGrp, slen, AX_VDEC_FILE_PATH_LEN);
                ret = -1;
                goto ERR_RET;
            }

            len = snprintf(pstCmd->pInputFilePath, slen + 1, "%s", optarg);
            if ((len < 0) || (len != slen)) {
                SAMPLE_CRIT_LOG("VdGrp=%d, snprintf FAILED! ret:0x%x optarg:%s slen:%ld\n",
                                VdGrp, len, optarg, slen);
                ret = -1;
                goto ERR_RET;
            }
            break;
        case 'o':
            pstCmd->tChnCfg[0].pOutputFilePath = malloc(AX_VDEC_FILE_PATH_LEN);
            if (pstCmd->tChnCfg[0].pOutputFilePath == NULL) {
                SAMPLE_CRIT_LOG("VdGrp=%d, malloc %d Bytes FAILED!\n",
                                VdGrp, AX_VDEC_FILE_PATH_LEN);
                ret = -3;
                goto ERR_RET;
            }

            slen = strlen(optarg);
            if (slen >= AX_VDEC_FILE_DIR_LEN) {
                SAMPLE_CRIT_LOG("VdGrp=%d, (strlen(optarg):%ld >= AX_VDEC_FILE_DIR_LEN:%d\n",
                                VdGrp, slen, AX_VDEC_FILE_DIR_LEN);
                ret = -1;
                goto ERR_RET;
            }

            len = snprintf(pstCmd->tChnCfg[0].pOutputFilePath, slen + 1, "%s", optarg);
            if ((len < 0) || (len != slen)) {
                SAMPLE_CRIT_LOG("VdGrp=%d, snprintf FAILED! ret:0x%x optarg:%s slen:%ld\n",
                                VdGrp, len, optarg, slen);
                ret = -1;
                goto ERR_RET;
            }
            break;
        case 'M':
            pstCmd->enInputMode = atoi(optarg);
            break;
        case 's':
            pstCmd->sStreamSize = atoi(optarg);
            break;
        case 'T':
            pstCmd->enDecType = atoi(optarg);
            break;
        case 'j':
            pstCmd->bJpegDecOneFrm = atoi(optarg);
            break;
        case 'q':
            pstCmd->bQuitWait = atoi(optarg);
            break;
        case 'h':
            ret = -1;
            break;
        case '0': {
            if (strcmp(pPrm->longOpt, "sMilliSec") == 0) {
                pstCmd->sMilliSec = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "VdChn") == 0) {
                VdChn = atoi(optarg);
                if (VdChn >= AX_DEC_MAX_CHN_NUM || VdChn < 0) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, Invalid index for option --VdChn: %s\n",
                                    VdGrp, optarg);
                    ret = 1;
                    goto ERR_RET;
                }
                pstCmd->tChnCfg[VdChn].bChnEnable = 1;
                pstCmd->tChnCfg[VdChn].VdChn = VdChn;
            }
            else if (strcmp(pPrm->longOpt, "crop") == 0) {
                argLen = (strlen(optarg) + 1) < VDEC_CMD_MAX_ARG_LEN ?
                            (strlen(optarg) + 1) : VDEC_CMD_MAX_ARG_LEN;
                memcpy(arg, optarg, argLen);
                if (__ParseCropParams(optarg, &pstCmd->tChnCfg[VdChn].u32CropX,
                                            &pstCmd->tChnCfg[VdChn].u32CropY,
                                            &pstCmd->tChnCfg[VdChn].u32PicWidth,
                                            &pstCmd->tChnCfg[VdChn].u32PicHeight)) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, Illegal cropping argument: --crop=%s\n",
                                    VdGrp, arg);
                    ret = 1;
                    goto ERR_RET;
                }
                pstCmd->tChnCfg[VdChn].enOutputMode = AX_VDEC_OUTPUT_CROP;
            }
            else if (strcmp(pPrm->longOpt, "scale") == 0) {
                argLen = (strlen(optarg) + 1) < VDEC_CMD_MAX_ARG_LEN ?
                            (strlen(optarg) + 1) : VDEC_CMD_MAX_ARG_LEN;
                memcpy(arg, optarg, argLen);
                if (__ParseScaleParams(optarg, &pstCmd->tChnCfg[VdChn].u32PicWidth,
                                            &pstCmd->tChnCfg[VdChn].u32PicHeight,
                                            &pstCmd->tChnCfg[VdChn].u32ScaleRatioX,
                                            &pstCmd->tChnCfg[VdChn].u32ScaleRatioY)) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, Illegal scaling argument: --scale=%s\n",
                                    VdGrp, arg);
                    ret = 1;
                    goto ERR_RET;
                }
                pstCmd->tChnCfg[VdChn].enOutputMode = AX_VDEC_OUTPUT_SCALE;
            }
            else if (strcmp(pPrm->longOpt, "res") == 0) {
                argLen = (strlen(optarg) + 1) < VDEC_CMD_MAX_ARG_LEN ?
                            (strlen(optarg) + 1) : VDEC_CMD_MAX_ARG_LEN;
                memcpy(arg, optarg, argLen);
                if (pstCmd->tChnCfg[VdChn].enOutputMode == AX_VDEC_OUTPUT_ORIGINAL) {
                    if (__ParseResParams(optarg, &pstCmd->tChnCfg[VdChn].u32PicWidth,
                                         &pstCmd->tChnCfg[VdChn].u32PicHeight)) {
                        SAMPLE_CRIT_LOG("VdGrp=%d, Illegal resolution argument: --res=%s\n",
                                        VdGrp, arg);
                        ret = 1;
                        goto ERR_RET;
                    }
                }
            } else if (strcmp(pPrm->longOpt, "usrPicFile") == 0) {
                pstCmd->tChnCfg[VdChn].pUsrPicFilePath = malloc(AX_VDEC_FILE_PATH_LEN);
                if (pstCmd->tChnCfg[VdChn].pUsrPicFilePath == NULL) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, malloc %d Bytes FAILED!\n",
                                    VdGrp, AX_VDEC_FILE_PATH_LEN);
                    ret = -3;
                    goto ERR_RET;
                }

                slen = strlen(optarg);
                if (slen >= AX_VDEC_FILE_PATH_LEN) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, (strlen(optarg):%ld >= AX_VDEC_FILE_PATH_LEN:%d\n",
                                   VdGrp, slen, AX_VDEC_FILE_PATH_LEN);
                    ret = -1;
                    goto ERR_RET;
                }

                len = snprintf(pstCmd->tChnCfg[VdChn].pUsrPicFilePath, slen + 1, "%s", optarg);
                if ((len < 0) || (len != slen)) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, snprintf FAILED! ret:0x%x optarg:%s slen:%ld\n",
                                    VdGrp, len, optarg, slen);
                    ret = -1;
                    goto ERR_RET;
                }

                pstCmd->tChnCfg[VdChn].bUserPicEnable = AX_TRUE;
            } else if (strcmp(pPrm->longOpt, "usrPicIdx") == 0) {
               pstCmd->usrPicIdx = atoi(optarg);
            } else if (strcmp(pPrm->longOpt, "bDynRes") == 0) {
               pstCmd->bDynRes = atoi(optarg);
            } else if (strcmp(pPrm->longOpt, "newInput") == 0) {
                pstCmd->pNewInputFilePath = malloc(AX_VDEC_FILE_PATH_LEN);
                if (pstCmd->pNewInputFilePath == NULL) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, malloc %d Bytes FAILED!\n",
                                    VdGrp, AX_VDEC_FILE_PATH_LEN);
                    ret = -3;
                    goto ERR_RET;
                }

                slen = strlen(optarg);
                if (slen >= AX_VDEC_FILE_PATH_LEN) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, (strlen(optarg):%ld >= AX_VDEC_FILE_PATH_LEN:%d\n",
                                   VdGrp, slen, AX_VDEC_FILE_PATH_LEN);
                    ret = -1;
                    goto ERR_RET;
                }

                len = snprintf(pstCmd->pNewInputFilePath, slen + 1, "%s", optarg);
                if ((len < 0) || (len != slen)) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, snprintf FAILED! ret:0x%x optarg:%s slen:%ld\n",
                                    VdGrp, len, optarg, slen);
                    ret = -1;
                    goto ERR_RET;
                }
            }
            else if (strcmp(pPrm->longOpt, "highRes") == 0) {
               pstCmd->highRes = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "bUsrInstant") == 0) {
               pstCmd->bUsrInstant = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "recvStmAfUsrPic") == 0) {
               pstCmd->recvStmAfUsrPic = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "bPerfTest") == 0) {
               pstCmd->bPerfTest = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "bGetUserData") == 0) {
               pstCmd->bGetUserData = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "bGetRbInfo") == 0) {
               pstCmd->bGetRbInfo = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "bQueryStatus") == 0) {
               pstCmd->bQueryStatus = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "bGetVuiParam") == 0) {
               pstCmd->bGetVuiParam = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "outFormat") == 0) {
                pstCmd->tChnCfg[VdChn].enImgFormat = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "outputfile") == 0) {
                pstCmd->tChnCfg[VdChn].pOutputFilePath = malloc(AX_VDEC_FILE_PATH_LEN);
                if (pstCmd->tChnCfg[VdChn].pOutputFilePath == NULL) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, malloc %d Bytes FAILED!\n",
                                    VdGrp, AX_VDEC_FILE_PATH_LEN);
                    ret = -3;
                    goto ERR_RET;
                }

                slen = strlen(optarg);
                if (slen >= AX_VDEC_FILE_DIR_LEN) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, (strlen(optarg):%ld >= AX_VDEC_FILE_DIR_LEN:%d\n",
                                VdGrp, slen, AX_VDEC_FILE_DIR_LEN);
                    ret = -1;
                    goto ERR_RET;
                }

                len = snprintf(pstCmd->tChnCfg[VdChn].pOutputFilePath, slen + 1, "%s", optarg);
                if ((len < 0) || (len != slen)) {
                    SAMPLE_CRIT_LOG("VdGrp=%d, snprintf FAILED! ret:0x%x optarg:%s slen:%ld\n",
                                    VdGrp, len, optarg, slen);
                    ret = -1;
                    goto ERR_RET;
                }
                break;
            }
            else if (strcmp(pPrm->longOpt, "tb_cfg_file") == 0) {
                pstCmd->pTbCfgFilePath = optarg;
                // SAMPLE_LOG("pstCmd->tb_cfg_file: %s\n", pstCmd->tb_cfg_file);
            }
            else if (strcmp(pPrm->longOpt, "f32SrcFrmRate") == 0) {
                pstCmd->f32SrcFrmRate = (AX_F32)atof(optarg);
            }
            else if (strcmp(pPrm->longOpt, "f32DstFrmRate") == 0) {
                pstCmd->tChnCfg[VdChn].f32DstFrmRate = (AX_F32)atof(optarg);
            }
            else if (strcmp(pPrm->longOpt, "bFrmRateCtrl") == 0) {
                pstCmd->tChnCfg[VdChn].bFrmRateCtrl = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "enDisplayMode") == 0) {
                pstCmd->enDisplayMode = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "multimode") == 0) {
                pstCmd->enMultimode = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "select") == 0) {
                pstCmd->enSelectMode = atoi(optarg);
                if (pstCmd->enSelectMode != AX_VDEC_SELECT_MODE_DISABLE) {
                    pstCmd->sMilliSec = 0;
                }
            }
            else if (strcmp(pPrm->longOpt, "streamcfg") == 0) {
                pstCmd->pGrpCmdlFile[pstCmd->uStreamCount] = optarg;
                streamcfg_para = AX_TRUE;
                SAMPLE_LOG("VdGrp=%d, pstCmd->pGrpCmdlFile[pstCmd->uStreamCount:%d]:%s",
                           VdGrp, pstCmd->uStreamCount,
                           pstCmd->pGrpCmdlFile[pstCmd->uStreamCount]);
                pstCmd->uStreamCount++;
            }
            else if (strcmp(pPrm->longOpt, "nstream") == 0) {
                pstCmd->uStreamCount = atoi(optarg);
                break;
            }
            else if (strcmp(pPrm->longOpt, "fbc_complevel") == 0) {
                pstCmd->tChnCfg[VdChn].uCompressLevel = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "uMaxGrpCnt") == 0) {
                pstCmd->uMaxGrpCnt = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "uStartGrpId") == 0) {
                pstCmd->uStartGrpId = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "enDecType") == 0) {
                pstCmd->enDecType = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "uOutputFifoDepth") == 0) {
                pstCmd->tChnCfg[VdChn].u32OutputFifoDepth = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "u32FrameBufCnt") == 0) {
                pstCmd->tChnCfg[VdChn].u32FrameBufCnt = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "enFrameBufSrc") == 0) {
                pstCmd->enFrameBufSrc = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "bFfmpegEnable") == 0) {
                pstCmd->bFfmpegEnable = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "enOutputOrder") == 0) {
                pstCmd->enOutputOrder = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "enVideoMode") == 0) {
                pstCmd->enVideoMode = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "waitTime") == 0) {
                pstCmd->waitTime = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "pollingEna") == 0) {
                pstCmd->pollingEna = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "pollingCnt") == 0) {
                pstCmd->pollingCnt = atoi(optarg);
            }
            else if (strcmp(pPrm->longOpt, "pollingTime") == 0) {
                pstCmd->pollingTime = atoi(optarg);
            }
            else {
                if ((pstCmd->uStreamCount > 0) && (pstCmd->uStreamCount < AX_VDEC_MAX_GRP_NUM)) {
                    AX_CHAR StreamCfgName[AX_VDEC_FILE_NAME_LEN];

                    if (streamcfg_para == AX_FALSE) {
                        for (i = 0; i < pstCmd->uStreamCount; i++) {
                            memset(StreamCfgName, 0, sizeof(StreamCfgName));
                            sprintf(StreamCfgName, "grp_cmd_%d", i);
                            if (strcmp(pPrm->longOpt, StreamCfgName) == 0) {
                                pstCmd->pGrpCmdlFile[i] = optarg;
                                continue;
                            }
                        }
                        break;
                    } else {
                        for (i = 0; i < pstCmd->uStreamCount; i++) {
                            memset(StreamCfgName, 0, sizeof(StreamCfgName));
                            sprintf(StreamCfgName, "grp_cmd_%d", i);

                            if (strcmp(pPrm->longOpt, StreamCfgName) == 0) {
                                SAMPLE_CRIT_LOG("VdGrp=%d, pGrpCmdlFile[0]:%s and %s:%s "
                                                "cannot coexist! uStreamCount:%d",
                                                VdGrp, pstCmd->pGrpCmdlFile[0],
                                                StreamCfgName, optarg, pstCmd->uStreamCount);
                                break;
                            }
                        }

                        if (optopt == 0) {
                            break;
                        }
                    }
                } else {
                    SAMPLE_LOG("pstCmd->uStreamCount:%d", pstCmd->uStreamCount);
                }

                SAMPLE_CRIT_LOG("VdGrp=%d, Unknown option character 0x%x ", VdGrp, optopt);
                ret = 1;
                goto ERR_RET;
            }
            break;
        }
        case '?':
            if (isprint(optopt))
                SAMPLE_CRIT_LOG("Unknown option `-%c'.\n", optopt);
            else
                SAMPLE_CRIT_LOG("Unknown option character `\\x%x'.\n", optopt);
            ret = 1;
            goto ERR_RET;
        default:
            SAMPLE_CRIT_LOG("VdGrp=%d, unknow options:%c.\n", VdGrp, prm.short_opt);
            ret = -1;
            goto ERR_RET;
        }
    }

    __VdecParameterAdjust(pstCmd);
    if (__VdecParameterCheck(pstCmd, VdGrp) != 0) {
        SAMPLE_CRIT_LOG("VdGrp=%d, __VdecParameterCheck FAILED!", VdGrp);
        sRet = -1;
    }

    __VdecCmdParaPrint(pstCmd, VdGrp);

ERR_RET:
    if (sRet || ret) {
        __PrintHelp_Com();
        if (bLink)
            __PrintHelp_Link();
    }

    if (options != NULL) {
        free(options);
    }

    if (pOptName != NULL) {
        free(pOptName);
    }

    return sRet || ret;
}


AX_S32 VdecDefaultParamsSet(SAMPLE_VDEC_CMD_PARAM_T *pstCmdPara)
{
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = pstCmdPara;
    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("pstCmd == NULL");
        return -1;
    }
    memset(pstCmd, 0, sizeof(SAMPLE_VDEC_CMD_PARAM_T));

    pstCmd->uGrpCount = 1;
    pstCmd->uStreamCount = 0;
    pstCmd->sLoopDecNum = 1;
    pstCmd->sRecvPicNum = 0;
    pstCmd->sMilliSec = -1;
    pstCmd->u32MaxPicWidth = AX_VDEC_MAX_WIDTH;
    pstCmd->u32MaxPicHeight = AX_VDEC_MAX_HEIGHT;
    pstCmd->sWriteFrames = 0; // 0: not write, others: write
    pstCmd->DestMD5 = 0;
    pstCmd->uMaxGrpCnt = AX_VDEC_MAX_GRP_NUM;
    pstCmd->bJpegDecOneFrm = AX_FALSE;
    pstCmd->f32SrcFrmRate = 0;
    pstCmd->enDisplayMode = AX_VDEC_DISPLAY_MODE_PLAYBACK;
    pstCmd->enMultimode = AX_VDEC_MULTI_MODE_DISABLE;
    pstCmd->enSelectMode = AX_VDEC_SELECT_MODE_PRIVATE;
    pstCmd->enFrameBufSrc = POOL_SOURCE_PRIVATE;
    pstCmd->enDecType = PT_H264;
    pstCmd->enInputMode = AX_VDEC_INPUT_MODE_FRAME;
    pstCmd->enDecModule = AX_ENABLE_BOTH_VDEC_JDEC;
    pstCmd->pInputFilePath = NULL;
    pstCmd->sStreamSize = 1024 * 1024;
    pstCmd->pollingTime = 10;
    pstCmd->usrPicIdx = 7;

    for (int ci = 0; ci < AX_DEC_MAX_CHN_NUM; ci++) {
        pstCmd->tChnCfg[ci].VdChn = ci;
        pstCmd->tChnCfg[ci].bChnEnable = AX_FALSE;
        pstCmd->tChnCfg[ci].u32OutputFifoDepth = SAMPLE_VDEC_OUTPUT_FRAMEBUF_CNT;
        pstCmd->tChnCfg[ci].u32FrameBufCnt = SAMPLE_VDEC_OUTPUT_FRAMEBUF_CNT + 2;
        pstCmd->tChnCfg[ci].enOutputMode = AX_VDEC_OUTPUT_ORIGINAL;
        pstCmd->tChnCfg[ci].enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
    }

    pstCmd->tChnCfg[0].bChnEnable = AX_TRUE;
    pstCmd->bFfmpegEnable = AX_TRUE;
    pstCmd->enVideoMode = VIDEO_DEC_MODE_IPB;

    return 0;
}

