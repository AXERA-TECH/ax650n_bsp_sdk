/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_global_type.h"
#include "AppLogApi.h"
#include "Uxe.h"

#define SENSOR "SENSOR"
#define SDRKEY "sdr"
#define HDRKEY "hdr"
#define BOARD_ID_LEN 128

CUxe::CUxe(AX_BOOL bYUV444) : CBaseSensor(SENSOR_CONFIG_T(0, 0, AX_SNS_LINEAR_MODE, AX_VIN_DEV_OFFLINE, 30.0, 1)), m_bYUV444(bYUV444) {

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


    AX_MIPI_RX_DEV_E eMipiDev = AX_MIPI_RX_DEV_0; //yuv 444

    if (!m_bYUV444) {
        m_tSnsCfg.nDevID = 4;
        eMipiDev = AX_MIPI_RX_DEV_4;
    }

    AX_U8 nDevID = m_tSnsCfg.nDevID;

    // MIPI proc
    AX_LANE_COMBO_MODE_E emode = (m_bYUV444 == AX_TRUE ? AX_LANE_COMBO_MODE_0 : AX_LANE_COMBO_MODE_4);
    nRet = AX_MIPI_RX_SetLaneCombo(emode);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_SetLaneCombo failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_MIPI_RX_SetAttr(eMipiDev, &m_tMipiRxDev);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_SetAttr failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_MIPI_RX_Reset(eMipiDev);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_Reset failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_MIPI_RX_Start(eMipiDev);
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
    if (m_bYUV444) {
        nRet = AX_VIN_SetDevBindMipi(nDevID, eMipiDev);
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

    nRet = AX_MIPI_RX_Stop((AX_MIPI_RX_DEV_E)nDevID);
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
    m_tSnsAttr.nWidth = 3840;
    m_tSnsAttr.nHeight = 2160;
    m_tSnsAttr.fFrameRate = 60.0; //m_tSnsCfg.fFrameRate;  // 60.0,
    m_tSnsAttr.eSnsMode = AX_SNS_LINEAR_MODE; //m_tSnsCfg.eSensorMode;  //
    if (m_bYUV444) {
        m_tSnsAttr.eRawType = AX_RT_RAW8;
    } else {
        m_tSnsAttr.eRawType = AX_RT_YUV422;
    }
    m_tSnsAttr.eBayerPattern = AX_BP_RGGB;
    m_tSnsAttr.bTestPatternEnable = AX_FALSE;
}

AX_VOID CUxe::InitSnsClkAttr() { //?
    /* Referenced by AX_VIN_OpenSnsClk */
    if (m_bYUV444) {
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
    if (m_bYUV444) {
        m_tMipiRxDev.tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_8;
        m_tMipiRxDev.tMipiAttr.nDataRate = 1900;
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
    if (m_bYUV444) {
        m_tDevAttr.eDevWorkMode = AX_VIN_DEV_WORK_MODE_4MULTIPLEX;
        m_tDevAttr.eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW;
        m_tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_8BPP;

        /* ROI config */
        m_tDevAttr.tDevImgRgn[0].nStartX = 0;
        m_tDevAttr.tDevImgRgn[0].nStartY = 0;
        m_tDevAttr.tDevImgRgn[0].nWidth = 3840 * 3 / 2;
        m_tDevAttr.tDevImgRgn[0].nHeight = 2160 / 2;

        m_tDevAttr.tDevImgRgn[1].nStartX = 3840 * 3 / 2;
        m_tDevAttr.tDevImgRgn[1].nStartY = 0;
        m_tDevAttr.tDevImgRgn[1].nWidth = 3840 * 3 / 2;
        m_tDevAttr.tDevImgRgn[1].nHeight = 2160 / 2;

        m_tDevAttr.tDevImgRgn[2].nStartX = 0;
        m_tDevAttr.tDevImgRgn[2].nStartY = 2160 / 2;
        m_tDevAttr.tDevImgRgn[2].nWidth = 3840 * 3 / 2;
        m_tDevAttr.tDevImgRgn[2].nHeight = 2160 / 2;

        m_tDevAttr.tDevImgRgn[3].nStartX = 3840 * 3 / 2;
        m_tDevAttr.tDevImgRgn[3].nStartY = 2160 / 2;
        m_tDevAttr.tDevImgRgn[3].nWidth = 3840 * 3 / 2;
        m_tDevAttr.tDevImgRgn[3].nHeight = 2160 / 2;

        for (AX_U8 i = 0; i < AX_HDR_CHN_NUM; i++) {
            m_tDevAttr.nWidthStride[i] = 3840 * 3;
        }

        /* configure the attribute of early reporting of frame interrupts */
        m_tDevFrmIntAttr.bImgRgnIntEn[0] = AX_FALSE;
        m_tDevFrmIntAttr.bImgRgnIntEn[1] = AX_FALSE;
        m_tDevFrmIntAttr.bImgRgnIntEn[2] = AX_FALSE;
        m_tDevFrmIntAttr.bImgRgnIntEn[3] = AX_TRUE;
    } else {
        m_tDevAttr.eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_YUV;
        m_tDevAttr.ePixelFmt = AX_FORMAT_YUV422_INTERLEAVED_YUYV;
        m_tDevAttr.nConvYuv422To420En = 1;
        m_tDevAttr.nConvFactor = 2;

        for (AX_U8 i = 0; i < AX_HDR_CHN_NUM; i++) {
            m_tDevAttr.tDevImgRgn[i] = {0, 0, 3840, 2160};
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
        tPipeAttr.tPipeImgRgn = {0, 0, 3840, 2160};
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
                .nWidth  = 3840, //(AX_U32)tPipeAttr.arrChannelAttr[0].nWidth,   // 3840
                .nHeight = 2160, //(AX_U32)tPipeAttr.arrChannelAttr[0].nHeight,  // 2160
                .nWidthStride = 3840,
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

