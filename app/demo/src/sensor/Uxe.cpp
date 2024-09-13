/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_global_type.h"
#include "AppLogApi.h"
#include "Uxe.h"

#define SENSOR "SENSOR"
#define SDRKEY "sdr"
#define HDRKEY "hdr"
#define BOARD_ID_LEN 128

CUxe::CUxe(AX_IMG_FORMAT_E eImgFormat, AX_U32 nW /* = 3840 */, AX_U32 nH /* = 2160 */, AX_U32 nFps /* = 60 */)
    : CBaseSensor(SENSOR_CONFIG_T(0, 0, AX_SNS_LINEAR_MODE, AX_VIN_DEV_OFFLINE, (AX_F32)nFps, 1))
    , m_eImgFormat(eImgFormat)
    , m_nW(nW)
    , m_nH(nH)
    , m_nFps(nFps) {
}

CUxe::~CUxe(AX_VOID) {
}

AX_BOOL CUxe::Init(AX_VOID) {
    AX_S32 nRet = 0;

    /* VIN Init */
    nRet = AX_VIN_Init();
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_Init failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    /* MIPI init */
    nRet = AX_MIPI_RX_Init();
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_Init failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    memset(&m_tSnsAttr, 0, sizeof(AX_SNS_ATTR_T));
    memset(&m_tDevAttr, 0, sizeof(AX_VIN_DEV_ATTR_T));
    memset(&m_tMipiRxDev, 0, sizeof(AX_MIPI_RX_DEV_T));

    InitMipiRxAttr();
    InitSnsAttr();
    InitSnsClkAttr();
    InitDevAttr();
    InitPipeAttr();
    InitChnAttr();
    return AX_TRUE;
    //return CBaseSensor::Init();
}

const SENSOR_CONFIG_T& CUxe::GetSensorCfg() {
    return m_tSnsCfg;
}

AX_BOOL CUxe::Open() {
    LOG_MM(SENSOR, "[Dev:%d] +++", m_tSnsCfg.nDevID);

    LOG_MM(SENSOR, "Sensor Attr => w:%d h:%d framerate:%.2f sensor mode:%d rawType:%d", m_tSnsAttr.nWidth, m_tSnsAttr.nHeight,
           m_tSnsAttr.fFrameRate, m_tSnsAttr.eSnsMode, m_tSnsAttr.eRawType);

    AX_S32 nRet = 0;

    AX_LANE_COMBO_MODE_E emode;
    AX_U32 nMipiDev = 0; //yuv 444

    // if (!(m_eImgFormat == AX_FORMAT_BAYER_RAW_8BPP)) {
    //     m_tSnsCfg.nDevID = 4;
    //     nMipiDev = 4;
    // }

    // depends on HW connections
    switch (m_eImgFormat) {
        case AX_FORMAT_BAYER_RAW_8BPP: /* mipi 8 lane */
            m_tSnsCfg.nDevID = 0;
            nMipiDev = 0;
            emode = AX_LANE_COMBO_MODE_0;
            break;
        case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010: /* mipi 8 lane */
            m_tSnsCfg.nDevID = 4;
            nMipiDev = 0;
            emode = AX_LANE_COMBO_MODE_0;
            break;
        case AX_FORMAT_YUV420_SEMIPLANAR: /* mipi 4 lane */
            m_tSnsCfg.nDevID = 4;
            nMipiDev = 4;
            emode = AX_LANE_COMBO_MODE_4;
            break;
        default:
            break;
    }

    AX_U8 nDevID = m_tSnsCfg.nDevID;

    // MIPI proc
    nRet = AX_MIPI_RX_SetLaneCombo(emode);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_SetLaneCombo failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_MIPI_RX_SetAttr(nMipiDev, &m_tMipiRxDev);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_SetAttr failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_MIPI_RX_Reset(nMipiDev);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_Reset failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_MIPI_RX_Start(nMipiDev);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_Start failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    // DEV proc
    nRet = AX_VIN_CreateDev(nDevID, &m_tDevAttr);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_CreateDev failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_VIN_SetDevAttr(nDevID, &m_tDevAttr);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_SetDevAttr failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    AX_VIN_DEV_BIND_PIPE_T tDevBindPipe = {0};
    for (AX_U8 i = 0; i < 1; i++) {
        AX_U8 nPipeID = 0;

        tDevBindPipe.nPipeId[i] = nPipeID;
        tDevBindPipe.nNum += 1;

        switch (AX_SNS_LINEAR_MODE) {
            case AX_SNS_LINEAR_MODE:
                tDevBindPipe.nHDRSel[i] = 0x1;
                break;
            case AX_SNS_HDR_2X_MODE:
                tDevBindPipe.nHDRSel[i] = 0x1 | 0x2;
                break;
            case AX_SNS_HDR_3X_MODE:
                tDevBindPipe.nHDRSel[i] = 0x1 | 0x2 | 0x4;
                break;
            case AX_SNS_HDR_4X_MODE:
                tDevBindPipe.nHDRSel[i] = 0x1 | 0x2 | 0x4 | 0x8;
                break;
            default:
                tDevBindPipe.nHDRSel[i] = 0x1;
                break;
        }
    }

    nRet = AX_VIN_SetDevBindPipe(nDevID, &tDevBindPipe);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_SetDevBindPipe failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_VIN_SetDevBindMipi(nDevID, nMipiDev);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_SetDevBindMipi failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    /* configure the attribute of early reporting of frame interrupts */
    if (AX_VIN_DEV_WORK_MODE_1MULTIPLEX < m_tDevAttr.eDevWorkMode) {
        nRet = AX_VIN_SetDevFrameInterruptAttr(nDevID, &m_tDevFrmIntAttr);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "AX_VIN_SetDevFrameInterruptAttr failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }
    }

    if (AX_VIN_DEV_OFFLINE == m_tDevAttr.eDevMode) {
        AX_VIN_DEV_DUMP_ATTR_T tDumpAttr;
        tDumpAttr.bEnable = AX_TRUE;
        tDumpAttr.nDepth = 3;
        tDumpAttr.eDumpType = AX_VIN_DUMP_QUEUE_TYPE_DEV;
        nRet = AX_VIN_SetDevDumpAttr(nDevID, &tDumpAttr);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "AX_VIN_SetDevDumpAttr failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }
    }

    nRet = AX_VIN_EnableDev(nDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_EnableDev failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    LOG_MM(SENSOR, "[%d] ---", m_tSnsCfg.nDevID);

    return AX_TRUE;
}

AX_BOOL CUxe::Close() {
    LOG_MM(SENSOR, "[Dev:%d] +++", m_tSnsCfg.nDevID);

    AX_S32 nRet = AX_SUCCESS;

    AX_U8 nDevID = m_tSnsCfg.nDevID;

    nRet = AX_VIN_DisableDev(nDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_DisableDev failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    if (AX_VIN_DEV_OFFLINE == m_tSnsCfg.eDevMode) {
        AX_VIN_DEV_DUMP_ATTR_T tDumpAttr;
        tDumpAttr.bEnable = AX_FALSE;
        tDumpAttr.eDumpType = AX_VIN_DUMP_QUEUE_TYPE_DEV;
        nRet = AX_VIN_SetDevDumpAttr(nDevID, &tDumpAttr);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "AX_VIN_SetDevDumpAttr failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }
    }

    nRet = AX_MIPI_RX_Stop(nDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_Stop failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_VIN_DestroyDev(nDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_DestroyDev failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    LOG_MM(SENSOR, "[Dev:%d] ---", m_tSnsCfg.nDevID);

    return AX_TRUE;
}

AX_BOOL CUxe::DeInit(AX_VOID) {
    AX_S32 axRet = 0;

    //CBaseSensor::DeInit();
    /* VIN Init */
    axRet = AX_VIN_Deinit();
    if (0 != axRet) {
        printf("AX_VIN_Deinit failed, ret=0x%x.\n", axRet);
        return AX_FALSE;
    }

    Close();

    /* MIPI init */
    axRet = AX_MIPI_RX_DeInit();
    if (0 != axRet) {
        printf("AX_MIPI_RX_DeInit failed, ret=0x%x.\n", axRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_VOID CUxe::InitSnsLibraryInfo(AX_VOID) {
}

AX_VOID CUxe::InitSnsAttr() {
    /* Referenced by AX_VIN_SetSnsAttr */
    m_tSnsAttr.nWidth = m_nW;
    m_tSnsAttr.nHeight = m_nH;
    m_tSnsAttr.fFrameRate = (AX_F32)m_nFps;
    m_tSnsAttr.eSnsMode = AX_SNS_LINEAR_MODE; //m_tSnsCfg.eSensorMode;  //
    if (m_eImgFormat == AX_FORMAT_BAYER_RAW_8BPP) {
        m_tSnsAttr.eRawType = AX_RT_RAW8;
    } else {
        m_tSnsAttr.eRawType = AX_RT_YUV422;
    }
    m_tSnsAttr.eBayerPattern = AX_BP_RGGB;
    m_tSnsAttr.bTestPatternEnable = AX_FALSE;
}

AX_VOID CUxe::InitSnsClkAttr() { //?
    /* Referenced by AX_VIN_OpenSnsClk */
    if (m_eImgFormat == AX_FORMAT_BAYER_RAW_8BPP) {
        m_tSnsClkAttr.nSnsClkIdx = 0;
    } else {
        m_tSnsClkAttr.nSnsClkIdx = 1;
    }
    m_tSnsClkAttr.eSnsClkRate = AX_SNS_CLK_24M;
}

AX_VOID CUxe::InitMipiRxAttr() {
    /* Referenced by AX_MIPI_RX_SetAttr */
    m_tMipiRxDev.eInputMode = AX_INPUT_MODE_MIPI;
    m_tMipiRxDev.tMipiAttr.ePhyMode = AX_MIPI_PHY_TYPE_DPHY;
    if (m_eImgFormat == AX_FORMAT_BAYER_RAW_8BPP) {
        m_tMipiRxDev.tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_8;
        m_tMipiRxDev.tMipiAttr.nDataRate = 1900;
    } else if (m_eImgFormat == AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010) {
        m_tMipiRxDev.tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_8;
        if (60 == m_nFps) {
            m_tMipiRxDev.tMipiAttr.nDataRate = 1600;
        } else {
            m_tMipiRxDev.tMipiAttr.nDataRate = 800;
        }
    } else {
        m_tMipiRxDev.tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_4;
        m_tMipiRxDev.tMipiAttr.nDataRate = 1300;
    }

    m_tMipiRxDev.tMipiAttr.nDataLaneMap[0] = 0;
    m_tMipiRxDev.tMipiAttr.nDataLaneMap[1] = 1;
    m_tMipiRxDev.tMipiAttr.nDataLaneMap[2] = 2;
    m_tMipiRxDev.tMipiAttr.nDataLaneMap[3] = 3;
    m_tMipiRxDev.tMipiAttr.nDataLaneMap[4] = -1;
    m_tMipiRxDev.tMipiAttr.nDataLaneMap[5] = -1;
    m_tMipiRxDev.tMipiAttr.nDataLaneMap[6] = -1;
    m_tMipiRxDev.tMipiAttr.nDataLaneMap[7] = -1;
    m_tMipiRxDev.tMipiAttr.nClkLane[0] = 1;
    m_tMipiRxDev.tMipiAttr.nClkLane[1] = 0;
}

AX_VOID CUxe::InitDevAttr() {
    /* Referenced by AX_VIN_SetDevAttr */
    m_tDevAttr.bImgDataEnable = AX_TRUE;
    m_tDevAttr.bNonImgDataEnable = AX_FALSE;
    m_tDevAttr.eDevMode = AX_VIN_DEV_OFFLINE;
    if (m_eImgFormat == AX_FORMAT_BAYER_RAW_8BPP) {
        m_tDevAttr.eDevWorkMode = AX_VIN_DEV_WORK_MODE_4MULTIPLEX;
        m_tDevAttr.eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW;
        m_tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_8BPP;

        /* ROI config */
        m_tDevAttr.tDevImgRgn[0].nStartX = 0;
        m_tDevAttr.tDevImgRgn[0].nStartY = 0;
        m_tDevAttr.tDevImgRgn[0].nWidth = m_nW * 3 / 2;
        m_tDevAttr.tDevImgRgn[0].nHeight = m_nH / 2;

        m_tDevAttr.tDevImgRgn[1].nStartX = m_nW * 3 / 2;
        m_tDevAttr.tDevImgRgn[1].nStartY = 0;
        m_tDevAttr.tDevImgRgn[1].nWidth = m_nW * 3 / 2;
        m_tDevAttr.tDevImgRgn[1].nHeight = m_nH / 2;

        m_tDevAttr.tDevImgRgn[2].nStartX = 0;
        m_tDevAttr.tDevImgRgn[2].nStartY = m_nH / 2;
        m_tDevAttr.tDevImgRgn[2].nWidth = m_nW * 3 / 2;
        m_tDevAttr.tDevImgRgn[2].nHeight = m_nH / 2;

        m_tDevAttr.tDevImgRgn[3].nStartX = m_nW * 3 / 2;
        m_tDevAttr.tDevImgRgn[3].nStartY = m_nH / 2;
        m_tDevAttr.tDevImgRgn[3].nWidth = m_nW * 3 / 2;
        m_tDevAttr.tDevImgRgn[3].nHeight = m_nH / 2;

        for (AX_U8 i = 0; i < AX_HDR_CHN_NUM; i++) {
            m_tDevAttr.nWidthStride[i] = m_nW * 3;
        }

        /* configure the attribute of early reporting of frame interrupts */
        m_tDevFrmIntAttr.bImgRgnIntEn[0] = AX_FALSE;
        m_tDevFrmIntAttr.bImgRgnIntEn[1] = AX_FALSE;
        m_tDevFrmIntAttr.bImgRgnIntEn[2] = AX_FALSE;
        m_tDevFrmIntAttr.bImgRgnIntEn[3] = AX_TRUE;
    } else {
        m_tDevAttr.eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_YUV;
        m_tDevAttr.ePixelFmt = m_eImgFormat;
        m_tDevAttr.nConvYuv422To420En = 1;
        m_tDevAttr.nConvFactor = 2;

        if (m_eImgFormat == AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010) {
            m_tDevAttr.tMipiIntfAttr.szImgVc[0] = 0;
            m_tDevAttr.tMipiIntfAttr.szImgDt[0] = AX_MIPI_CSI_DT_YUV422_10BIT;
        } else if (m_eImgFormat == AX_FORMAT_YUV420_SEMIPLANAR) {
            m_tDevAttr.tMipiIntfAttr.szImgVc[0] = 0;
            m_tDevAttr.tMipiIntfAttr.szImgDt[0] = AX_MIPI_CSI_DT_YUV422_8BIT;
        }

        for (AX_U8 i = 0; i < AX_HDR_CHN_NUM; i++) {
            m_tDevAttr.tDevImgRgn[i] = {0, 0, m_nW, m_nH};
        }
    }

    m_tDevAttr.eBayerPattern = AX_BP_RGGB;
    m_tDevAttr.eSnsMode = AX_SNS_LINEAR_MODE; // m_tSnsCfg.eSensorMode;  // AX_SNS_LINEAR_MODE
    m_tDevAttr.eSnsOutputMode = AX_SNS_NORMAL; //m_tSnsCfg.eSensorMode > AX_SNS_LINEAR_MODE ? AX_SNS_DOL_HDR : AX_SNS_NORMAL;  // AX_SNS_NORMAL
    m_tDevAttr.tCompressInfo = { AX_COMPRESS_MODE_NONE, 0 };
    m_tDevAttr.tFrameRateCtrl.fSrcFrameRate = AX_INVALID_FRMRATE;
    m_tDevAttr.tFrameRateCtrl.fDstFrameRate = AX_INVALID_FRMRATE;
}

AX_VOID CUxe::InitPrivAttr() {
    m_tPrivAttr.bEnable = AX_FALSE;
    m_tPrivAttr.ePrivDataMode = AX_PRIVATE_DATA_MODE_BOTTOM;
    for (AX_U8 i = 0; i < AX_HDR_CHN_NUM; i++) {
        m_tPrivAttr.tPrivDataRoiRgn[i] = {0, 0 , 0, 0};
    }
}

AX_VOID CUxe::InitPipeAttr() {
    /* Referenced by AX_VIN_SetPipeAttr */
    for (AX_U8 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipe = m_tSnsCfg.arrPipeAttr[i].nPipeID;
        AX_VIN_PIPE_ATTR_T tPipeAttr;
        tPipeAttr.tPipeImgRgn = {0, 0, m_nW, m_nH};
        tPipeAttr.eBayerPattern = AX_BP_RGGB;
        tPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP;  // m_tDevAttr.ePixelFmt;  // AX_FORMAT_BAYER_RAW_10BPP
        tPipeAttr.eSnsMode = AX_SNS_LINEAR_MODE;          // m_tSnsCfg.eSensorMode;  // AX_SNS_LINEAR_MODE
        tPipeAttr.bAiIspEnable = AX_FALSE; //m_tSnsCfg.arrPipeAttr[i].bAiEnable;  // AX_FALSE
        tPipeAttr.tCompressInfo = {AX_COMPRESS_MODE_NONE, 0};
        tPipeAttr.tNrAttr = {{AX_FALSE, {AX_COMPRESS_MODE_NONE, 0}}, {AX_FALSE, {AX_COMPRESS_MODE_NONE, 0}}};
        tPipeAttr.tFrameRateCtrl.fSrcFrameRate = AX_INVALID_FRMRATE;
        tPipeAttr.tFrameRateCtrl.fDstFrameRate = AX_INVALID_FRMRATE;
        m_mapPipe2Attr[nPipe] = tPipeAttr;
    }
}

AX_VOID CUxe::InitChnAttr() {
    /* Referenced by AX_VIN_SetChnAttr */
    for (AX_U8 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        PIPE_CONFIG_T& tPipeAttr = m_tSnsCfg.arrPipeAttr[i];

        AX_VIN_CHN_ATTR_T arrChnAttr[AX_VIN_CHN_ID_MAX] =
        {
            {
                .nWidth  = m_nW, //(AX_U32)tPipeAttr.arrChannelAttr[0].nWidth,   // 3840
                .nHeight = m_nH, //(AX_U32)tPipeAttr.arrChannelAttr[0].nHeight,  // 2160
                .nWidthStride = m_nW,
                .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
                .nDepth = 3, //tPipeAttr.arrChannelAttr[0].nYuvDepth, //3
                //.tFrameRateCtrl.tFrmRateCtrl = {0, 0},
                //.tFrameRateCtrl = {.nFrmRateCtrl = 0},
                .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
                .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0}
            },
            {
                .nWidth  = 1920, //(AX_U32)tPipeAttr.arrChannelAttr[0].nWidth,   // 1920
                .nHeight = 1080, //(AX_U32)tPipeAttr.arrChannelAttr[0].nHeight,  // 1080
                .nWidthStride = 1920,
                .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
                .nDepth = 3, //tPipeAttr.arrChannelAttr[0].nYuvDepth, //3
                .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
                .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0}
            },
            {
                .nWidth  = 720, //(AX_U32)tPipeAttr.arrChannelAttr[0].nWidth,   // 720
                .nHeight = 576, //(AX_U32)tPipeAttr.arrChannelAttr[0].nHeight,  // 576
                .nWidthStride = 720,
                .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
                .nDepth = 3, //tPipeAttr.arrChannelAttr[0].nYuvDepth, //3
                .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
                .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0}
            }
        };

        m_mapPipe2ChnAttr[tPipeAttr.nPipeID][0] = arrChnAttr[0];
        m_mapPipe2ChnAttr[tPipeAttr.nPipeID][1] = arrChnAttr[1];
        m_mapPipe2ChnAttr[tPipeAttr.nPipeID][2] = arrChnAttr[2];
    }
}

AX_VOID CUxe::InitAbilities() {
    m_tAbilities.bSupportHDR = AX_TRUE;
    m_tAbilities.bSupportHDRSplit = AX_TRUE;
}

AX_VOID CUxe::InitTriggerAttr() {
}
