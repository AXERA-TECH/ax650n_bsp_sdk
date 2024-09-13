/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _COMMON_CONFIG_H__
#define _COMMON_CONFIG_H__

#include "ax_vin_api.h"
#include "ax_mipi_rx_api.h"
#include "ax_mipi_tx_api.h"

AX_MIPI_RX_DEV_T gDummyMipiRx = {
    .eInputMode = AX_INPUT_MODE_MIPI,
    .tMipiAttr.ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_4,
    .tMipiAttr.nDataRate =  400,
    .tMipiAttr.nDataLaneMap[0] = 0,
    .tMipiAttr.nDataLaneMap[1] = 1,
    .tMipiAttr.nDataLaneMap[2] = 2,
    .tMipiAttr.nDataLaneMap[3] = 3,
    .tMipiAttr.nDataLaneMap[4] = -1,
    .tMipiAttr.nDataLaneMap[5] = -1,
    .tMipiAttr.nDataLaneMap[6] = -1,
    .tMipiAttr.nDataLaneMap[7] = -1,
    .tMipiAttr.nClkLane[0]     = 1,
    .tMipiAttr.nClkLane[1]     = 0,
};

AX_SNS_ATTR_T gDummySnsAttr = {
    .nWidth = 3840,
    .nHeight = 2160,
    .fFrameRate = 30.0,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
};

AX_VIN_DEV_ATTR_T gDummyDevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_OFFLINE,
    .eSnsIntfType = AX_SNS_INTF_TYPE_TPG,
    .tDevImgRgn[0] = {0, 0, 3840, 2160},
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gDummyPipeAttr = {
    .tPipeImgRgn = {0, 0, 3840, 2160},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_NONE, 0}}, {1, {AX_COMPRESS_MODE_LOSSLESS, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gDummyChn0Attr = {
    .nWidth = 3840,
    .nHeight = 2160,
    .nWidthStride = 3840,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gDummyChn1Attr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .nWidthStride = 1920,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gDummyChn2Attr = {
    .nWidth = 720,
    .nHeight = 576,
    .nWidthStride = 720,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_MIPI_RX_DEV_T gOs08a20MipiRx = {
    .eInputMode = AX_INPUT_MODE_MIPI,
    .tMipiAttr.ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_4,
    .tMipiAttr.nDataRate =  1440, //80000,  //AX_MIPI_DATA_RATE_80M,     //maps?
    .tMipiAttr.nDataLaneMap[0] = 0,
    .tMipiAttr.nDataLaneMap[1] = 1,
    .tMipiAttr.nDataLaneMap[2] = 2,
    .tMipiAttr.nDataLaneMap[3] = 3,
    .tMipiAttr.nDataLaneMap[4] = -1,
    .tMipiAttr.nDataLaneMap[5] = -1,
    .tMipiAttr.nDataLaneMap[6] = -1,
    .tMipiAttr.nDataLaneMap[7] = -1,
    .tMipiAttr.nClkLane[0]     = 1,
    .tMipiAttr.nClkLane[1]     = 0,
};

AX_SNS_ATTR_T gOs08a20SnsAttr = {
    .nWidth = 3840,
    .nHeight = 2160,
    .fFrameRate = 30.0,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
};

AX_SNS_CLK_ATTR_T gOs08a20SnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gOs08a20DevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_OFFLINE,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 3840, 2160},
    .tDevImgRgn[1] = {0, 0, 3840, 2160},
    .tDevImgRgn[2] = {0, 0, 3840, 2160},
    .tDevImgRgn[3] = {0, 0, 3840, 2160},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    //.tMipiIntfAttr.szImgVc[0] = 0,
    //.tMipiIntfAttr.szImgDt[0] = MIPI_CSI_DT_RAW10,
    //.tMipiIntfAttr.szImgVc[1] = 1,
    //.tMipiIntfAttr.szImgDt[1] = MIPI_CSI_DT_RAW10,

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gOs08a20PipeAttr = {
    .tPipeImgRgn = {0, 0, 3840, 2160},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_NONE, 0}}, {1, {AX_COMPRESS_MODE_LOSSLESS, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gOs08a20Chn0Attr = {
    .nWidth = 3840,
    .nHeight = 2160,
    .nWidthStride = 3840,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gOs08a20Chn1Attr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .nWidthStride = 1920,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gOs08a20Chn2Attr = {
    .nWidth = 720,
    .nHeight = 576,
    .nWidthStride = 720,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_MIPI_RX_DEV_T gOs08b10MipiRx = {
    .eInputMode = AX_INPUT_MODE_MIPI,
    .tMipiAttr.ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_4,
    .tMipiAttr.nDataRate =  1440, //80000,  //AX_MIPI_DATA_RATE_80M,
    .tMipiAttr.nDataLaneMap[0] = 0,
    .tMipiAttr.nDataLaneMap[1] = 1,
    .tMipiAttr.nDataLaneMap[2] = 2,
    .tMipiAttr.nDataLaneMap[3] = 3,
    .tMipiAttr.nDataLaneMap[4] = -1,
    .tMipiAttr.nDataLaneMap[5] = -1,
    .tMipiAttr.nDataLaneMap[6] = -1,
    .tMipiAttr.nDataLaneMap[7] = -1,
    .tMipiAttr.nClkLane[0]     = 1,
    .tMipiAttr.nClkLane[1]     = 0,
};

AX_SNS_ATTR_T gOs08b10SnsAttr = {
    .nWidth = 3840,
    .nHeight = 2160,
    .fFrameRate = 25.0,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
};

AX_SNS_CLK_ATTR_T gOs08b10SnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gOs08b10DevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_OFFLINE,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 3840, 2160},
    .tDevImgRgn[1] = {0, 0, 3840, 2160},
    .tDevImgRgn[2] = {0, 0, 3840, 2160},
    .tDevImgRgn[3] = {0, 0, 3840, 2160},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    //.tMipiIntfAttr.szImgVc[0] = 0,
    //.tMipiIntfAttr.szImgDt[0] = MIPI_CSI_DT_RAW10,
    //.tMipiIntfAttr.szImgVc[1] = 1,
    //.tMipiIntfAttr.szImgDt[1] = MIPI_CSI_DT_RAW10,

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gOs08b10PipeAttr = {
    .tPipeImgRgn = {0, 0, 3840, 2160},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_NONE, 0}}, {1, {AX_COMPRESS_MODE_LOSSLESS, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gOs08b10Chn0Attr = {
    .nWidth = 3840,
    .nHeight = 2160,
    .nWidthStride = 3840,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gOs08b10Chn1Attr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .nWidthStride = 1920,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gOs08b10Chn2Attr = {
    .nWidth = 720,
    .nHeight = 576,
    .nWidthStride = 720,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_MIPI_RX_DEV_T gSc910gsMipiRx = {
    .eInputMode = AX_INPUT_MODE_SUBLVDS,
    .tLvdsAttr.eLaneNum = AX_SLVDS_DATA_LANE_8,
    .tLvdsAttr.nDataRate = 750,
    .tLvdsAttr.nDataLaneMap[0] = 0,
    .tLvdsAttr.nDataLaneMap[1] = 1,
    .tLvdsAttr.nDataLaneMap[2] = 2,
    .tLvdsAttr.nDataLaneMap[3] = 3,
    .tLvdsAttr.nDataLaneMap[4] = 4,
    .tLvdsAttr.nDataLaneMap[5] = 5,
    .tLvdsAttr.nDataLaneMap[6] = 6,
    .tLvdsAttr.nDataLaneMap[7] = 7,
    .tLvdsAttr.nClkLane[0]     = 1,
    .tLvdsAttr.nClkLane[1]     = 0,
};

AX_SNS_ATTR_T gSc910gsSnsAttr = {
    .nWidth = 3840,
    .nHeight = 2336,
    .fFrameRate = 50.0,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    .eMasterSlaveSel = AX_SNS_SLAVE,
};

AX_SNS_CLK_ATTR_T gSc910gsSnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gSc910gsDevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_OFFLINE,
    .eSnsIntfType = AX_SNS_INTF_TYPE_SUB_LVDS,
    .tDevImgRgn[0] = {0, 1, 3840, 2336},
    .tDevImgRgn[1] = {0, 1, 3840, 2336},
    .tDevImgRgn[2] = {0, 1, 3840, 2336},
    .tDevImgRgn[3] = {0, 1, 3840, 2336},
    .tLvdsIntfAttr.nLineNum = 8,
    .tLvdsIntfAttr.nHispiFirCodeEn= 0,
    .tLvdsIntfAttr.nContSyncCodeMatchEn = 1,
    .tLvdsIntfAttr.eSyncMode = AX_VIN_LVDS_SYNC_MODE_SOF_EOF_NORMAL,
    .tLvdsIntfAttr.szSyncCode[0][0][0] = 0xab00,
    .tLvdsIntfAttr.szSyncCode[0][0][1] = 0x8000,
    .tLvdsIntfAttr.szSyncCode[1][0][0] = 0xab00,
    .tLvdsIntfAttr.szSyncCode[1][0][1] = 0x8000,
    .tLvdsIntfAttr.szSyncCode[2][0][0] = 0xab00,
    .tLvdsIntfAttr.szSyncCode[2][0][1] = 0x8000,
    .tLvdsIntfAttr.szSyncCode[3][0][0] = 0xab00,
    .tLvdsIntfAttr.szSyncCode[3][0][1] = 0x8000,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
};

AX_VIN_PIPE_ATTR_T gSc910gsPipeAttr = {
    .tPipeImgRgn = {0, 0, 3840, 2336},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_16BPP,
    .eSnsMode = AX_SNS_LINEAR_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_NONE, 0}}, {1, {AX_COMPRESS_MODE_LOSSLESS, 0}}},
};

AX_VIN_CHN_ATTR_T gSc910gsChn0Attr = {
    .nWidth = 3840,
    .nHeight = 2336,
    .nWidthStride = 3840,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
};

AX_VIN_CHN_ATTR_T gSc910gsChn1Attr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .nWidthStride = 1920,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
};

AX_VIN_CHN_ATTR_T gSc910gsChn2Attr = {
    .nWidth = 720,
    .nHeight = 576,
    .nWidthStride = 720,
    .ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 2,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
};

AX_VIN_POWER_SYNC_ATTR_T  gSc910gsPowerAttr =  {
    .ePowerTriggerMode = AX_VIN_SYNC_OUTSIDE_ELEC_ADAPTIVE_TRIGGER,
    .nGpioElecInPin = 22,
    .nGpioSyncOutPin = 135,
    .nFollowCycle = 3,
    .nFreqTolLeft = 2,
    .nFreqTolRight = 2,
    .nElecFreq = 50,
    .nSyncTriggerFreq = 50,
    .nSyncDelayElcUs = 2000,
    .nStrobeGpioNum[0] = 90,
    .nStrobeGpioNum[1] = 89,
    .nStrobeGpioNum[2] = 65,
    .nStrobeGpioNum[3] = 91,
    .nStrobeGpioNum[4] = 92,
};

AX_VIN_SYNC_SIGNAL_ATTR_T gSc910gsVsyncAttr = {
    .nSyncIdx = 0,
    .eSyncInv = AX_VIN_SYNC_POLARITY_HIGH,
    .nSyncFreq = 2700,
    .nSyncDutyRatio = 1,
};

AX_VIN_SYNC_SIGNAL_ATTR_T gSc910gsHsyncAttr = {
    .nSyncIdx = 0,
    .eSyncInv = AX_VIN_SYNC_POLARITY_HIGH,
    .nSyncFreq = 7407,
    .nSyncDutyRatio = 1,
};

AX_VIN_LIGHT_SYNC_INFO_T gSc910gsLightSyncInfo = {
    .nVts = 2700,
    .nHts = 7407,
    .nIntTime = 8000,
    .nElecToVsyncTime = 1350,
    .nVbbTime = 100,
    .szShutterParam[0].nShutterSeq = 1,
    .szShutterParam[0].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[1].nShutterSeq = 1,
    .szShutterParam[1].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[2].nShutterSeq = 1,
    .szShutterParam[2].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[3].nShutterSeq = 1,
    .szShutterParam[3].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[4].nShutterSeq = 1,
    .szShutterParam[4].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[5].nShutterSeq = 1,
    .szShutterParam[5].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[6].nShutterSeq = 1,
    .szShutterParam[6].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[7].nShutterSeq = 1,
    .szShutterParam[7].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[8].nShutterSeq = 1,
    .szShutterParam[8].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
    .szShutterParam[9].nShutterSeq = 1,
    .szShutterParam[9].eShutterMode = AX_VIN_SHUTTER_MODE_PICTURE,
};

#define LIGHT_STROBE (0)
AX_VIN_STROBE_LIGHT_TIMING_ATTR_T gSc910gsSnapStrobeAttr = {
    .eStrobeSyncInv = AX_VIN_SYNC_POLARITY_HIGH,
    .nStrobeDutyTime = 20,
    .nStrobeDelayTime = 1430,
};

#define LIGHT_FLASH (2)
AX_VIN_FLASH_LIGHT_TIMING_ATTR_T gSc910gsSnapFlashAttr = {
    .eFlashSyncInv = AX_VIN_SYNC_POLARITY_HIGH,
    .nFlashDutyTime = 20,
    .nFlashDelayTime = 1430,
};

#endif //_COMMON_CONFIG_H__
