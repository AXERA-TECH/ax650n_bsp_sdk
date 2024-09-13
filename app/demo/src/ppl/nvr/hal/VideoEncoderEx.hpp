/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once
#include "AXThread.hpp"
#include "ax_venc_comm.h"
#include "ax_venc_rc.h"
#include "ax_venc_api.h"
#include "GlobalDef.h"
#include "haltype.hpp"
#include <vector>

#define APP_ENCODER_TYPE_MAX 2
#define APP_RC_TYPE_MAX 4
#define APP_ENCODE_PARSE_INVALID (0)


typedef struct _stAppVencRcParams {
    AX_VENC_H264_CBR_T tH264Cbr;
    AX_VENC_H264_VBR_T tH264Vbr;
    AX_VENC_H264_FIXQP_T tH264FixQp;
    AX_VENC_H265_CBR_T tH265Cbr;
    AX_VENC_H265_VBR_T tH265Vbr;
    AX_VENC_H265_FIXQP_T tH265FixQp;
    AX_VENC_MJPEG_CBR_T tMjpegCbr;
    AX_VENC_MJPEG_VBR_T tMjpegVbr;
    AX_VENC_MJPEG_FIXQP_T tMjpegFixQp;

    _stAppVencRcParams() {
        memset(this, 0, sizeof(_stAppVencRcParams));
    }
} APP_VENC_RC_PARAMS_T;

typedef struct _stRCInfo {
    AX_VENC_RC_MODE_E eRcType{AX_VENC_RC_MODE_BUTT};
    AX_S32 nMinQp{APP_ENCODE_PARSE_INVALID};
    AX_S32 nMaxQp{APP_ENCODE_PARSE_INVALID};
    AX_S32 nMinIQp{APP_ENCODE_PARSE_INVALID};
    AX_S32 nMaxIQp{APP_ENCODE_PARSE_INVALID};
    AX_S32 nMinIProp{APP_ENCODE_PARSE_INVALID};
    AX_S32 nMaxIProp{APP_ENCODE_PARSE_INVALID};
    AX_S32 nFixQp{APP_ENCODE_PARSE_INVALID};
    AX_S32 nIntraQpDelta{APP_ENCODE_PARSE_INVALID};
    AX_S32 nBitrate{APP_ENCODE_PARSE_INVALID};
} RC_INFO_T;

typedef struct _APP_ENC_RC_CONFIG {
    AX_PAYLOAD_TYPE_E ePayloadType{PT_BUTT};
    RC_INFO_T stRCInfo[APP_RC_TYPE_MAX];
    AX_BOOL GetRcInfo(AX_VENC_RC_MODE_E type, RC_INFO_T& rcInfo) {
        for (AX_U8 i = 0; i < APP_RC_TYPE_MAX; i++) {
            if (stRCInfo[i].eRcType == type) {
                rcInfo = stRCInfo[i];
                return AX_TRUE;
            }
        }
        return AX_FALSE;
    }
    AX_BOOL SetRcInfo(AX_VENC_RC_MODE_E type, RC_INFO_T rcInfo) {
        for (AX_U8 i = 0; i < APP_RC_TYPE_MAX; i++) {
            if (stRCInfo[i].eRcType == type) {
                stRCInfo[i] = rcInfo;
                return AX_TRUE;
            }
        }
        return AX_FALSE;
    }
} APP_ENC_RC_CONFIG;

typedef struct _stVideoAttr {
    AX_S32 nChannel{APP_ENCODE_PARSE_INVALID};
    AX_PAYLOAD_TYPE_E ePayloadType{PT_BUTT};
    AX_VENC_RC_MODE_E eRcType{AX_VENC_RC_MODE_BUTT};
    AX_U32 nGOP{APP_ENCODE_PARSE_INVALID};
    AX_F32 fFramerate{APP_ENCODE_PARSE_INVALID};
    AX_U32 nWidth{APP_ENCODE_PARSE_INVALID};
    AX_U32 nHeight{APP_ENCODE_PARSE_INVALID};
    AX_U32 nMaxWidth{APP_ENCODE_PARSE_INVALID};
    AX_U32 nMaxHeight{APP_ENCODE_PARSE_INVALID};
    AX_U32 nBufSize{APP_ENCODE_PARSE_INVALID};
    AX_U8 nInFifoDepth{APP_ENCODE_PARSE_INVALID};
    AX_U8 nOutFifoDepth{APP_ENCODE_PARSE_INVALID};
    AX_MEMORY_SOURCE_E eMemSource {AX_MEMORY_SOURCE_CMM};
    AX_S32 nOffsetCropX{APP_ENCODE_PARSE_INVALID};
    AX_S32 nOffsetCropY{APP_ENCODE_PARSE_INVALID};
    AX_S32 nOffsetCropW{APP_ENCODE_PARSE_INVALID};
    AX_S32 nOffsetCropH{APP_ENCODE_PARSE_INVALID};
    AX_IMG_FORMAT_E eImgFormat {AX_FORMAT_YUV420_SEMIPLANAR};
    AX_S32 nBitrate{APP_ENCODE_PARSE_INVALID};
    APP_ENC_RC_CONFIG arrRcConfig[APP_ENCODER_TYPE_MAX];
    AX_BOOL bFBC{AX_FALSE};
    AX_BOOL bLink{AX_FALSE};
    AX_U32 nGdrNum {0};

    AX_BOOL GetRcCfg(AX_PAYLOAD_TYPE_E type, APP_ENC_RC_CONFIG& cfg) {
        for (AX_U8 i = 0; i < APP_ENCODER_TYPE_MAX; i++) {
            if (arrRcConfig[i].ePayloadType == type) {
                cfg = arrRcConfig[i];
                return AX_TRUE;
            }
        }
        return AX_FALSE;
    }

} VENC_ATTR_T;


class IVencPackObserver {
public:
    virtual ~IVencPackObserver(AX_VOID) = default;
    virtual AX_BOOL OnRecvStreamPack(AX_S32 nStream, CONST AX_VENC_PACK_T& stPack, AX_BOOL bGopStart = AX_TRUE) = 0;
};


class CVideoEncoderEx {
public:
    CVideoEncoderEx(VENC_ATTR_T tAttr);

    AX_BOOL Init();
    AX_BOOL DeInit();
    AX_BOOL Start();
    AX_BOOL Stop();

    AX_BOOL InitParams();

    AX_VOID StartRecv();
    AX_VOID StopRecv();

    AX_VOID RegObserver(IVencPackObserver* pObserver);
    AX_VOID UnregObserver(IVencPackObserver* pObserver);

    AX_BOOL IsKeyFrame(const AX_VENC_PACK_T &stPack);
    AX_BOOL ActiveGDR(AX_U32 nRefreshNum);

    AX_S32 GetChannel() {
        return m_tVideoConfig.nChannel;
    }

protected:
    AX_BOOL InitRcParams();
    AX_VOID DispatchThread(AX_VOID*);
    AX_BOOL NotifyAll(AX_U32 nChannel, AX_VOID* pStream, AX_BOOL bGopStart = AX_TRUE);

protected:
    VENC_ATTR_T m_tVideoConfig;
    AX_VENC_CHN_ATTR_T m_tVencChnAttr;
    APP_ENC_RC_CONFIG m_tRcCfg;
    APP_VENC_RC_PARAMS_T m_tVencRcParams;

    CAXThread m_thread;
    std::vector<IVencPackObserver*> m_vecObserver;
};
