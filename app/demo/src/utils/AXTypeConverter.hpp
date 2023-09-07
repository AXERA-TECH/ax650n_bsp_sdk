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

#include <string.h>
#include "ax_global_type.h"
#include "ax_venc_rc.h"

class CAXTypeConverter {
public:
    static AX_PAYLOAD_TYPE_E Int2EncoderType(AX_U32 nEncodeType) {
        if (0 == nEncodeType) {
            return PT_H264;
        } else if (2 == nEncodeType) {
            return PT_H265;
        } else if (1 == nEncodeType) {
            return PT_MJPEG;
        }

        return PT_H264;
    }

    static AX_U32 EncoderType2Int(AX_PAYLOAD_TYPE_E eEncodeType) {
        if (PT_H264 == eEncodeType) {
            return 0;
        } else if (PT_H265 == eEncodeType) {
            return 2;
        } else if (PT_MJPEG == eEncodeType) {
            return 1;
        }

        return 0;
    }
    static AX_VENC_RC_MODE_E Str2RcMode(std::string strRcModeName, AX_PAYLOAD_TYPE_E ePayloadType) {
        if (PT_H264 == ePayloadType) {
            if (strRcModeName == "CBR") {
                return AX_VENC_RC_MODE_H264CBR;
            } else if (strRcModeName == "VBR") {
                return AX_VENC_RC_MODE_H264VBR;
            } else if (strRcModeName == "FIXQP") {
                return AX_VENC_RC_MODE_H264FIXQP;
            }
        } else if (PT_H265 == ePayloadType) {
            if (strRcModeName == "CBR") {
                return AX_VENC_RC_MODE_H265CBR;
            } else if (strRcModeName == "VBR") {
                return AX_VENC_RC_MODE_H265VBR;
            } else if (strRcModeName == "FIXQP") {
                return AX_VENC_RC_MODE_H265FIXQP;
            }
        } else if (PT_MJPEG == ePayloadType) {
            if (strRcModeName == "CBR") {
                return AX_VENC_RC_MODE_MJPEGCBR;
            } else if (strRcModeName == "VBR") {
                return AX_VENC_RC_MODE_MJPEGVBR;
            } else if (strRcModeName == "FIXQP") {
                return AX_VENC_RC_MODE_MJPEGFIXQP;
            }
        }

        return AX_VENC_RC_MODE_H264CBR;
    }

    static AX_U32 RcMode2Int(AX_VENC_RC_MODE_E eRcMode) {
        AX_U32 nRcMode = AX_VENC_RC_MODE_BUTT;
        switch (eRcMode) {
            case AX_VENC_RC_MODE_H264CBR:
            case AX_VENC_RC_MODE_H265CBR:
            case AX_VENC_RC_MODE_MJPEGCBR:
                nRcMode = 0;
                break;
            case AX_VENC_RC_MODE_H264VBR:
            case AX_VENC_RC_MODE_H265VBR:
            case AX_VENC_RC_MODE_MJPEGVBR:
                nRcMode = 1;
                break;
            case AX_VENC_RC_MODE_H264FIXQP:
            case AX_VENC_RC_MODE_H265FIXQP:
            case AX_VENC_RC_MODE_MJPEGFIXQP:
                nRcMode = 2;
                break;
            default:
                break;
        }
        return nRcMode;
    }

    static AX_VENC_RC_MODE_E FormatRcMode(AX_U8 nEncoderType, AX_U8 nRcType) {
        switch (nEncoderType) {
            case 0: {         /* H264 */
                switch (nRcType) {
                    case 0: { /* CBR */
                        return AX_VENC_RC_MODE_H264CBR;
                    }
                    case 1: { /* VBR */
                        return AX_VENC_RC_MODE_H264VBR;
                    }
                    case 2: { /* FIXQP */
                        return AX_VENC_RC_MODE_H264FIXQP;
                    }
                    default:
                        break;
                }
            }
            case 1: {         /* MJPEG */
                switch (nRcType) {
                    case 0: { /* CBR */
                        return AX_VENC_RC_MODE_MJPEGCBR;
                    }
                    case 1: { /* VBR */
                        return AX_VENC_RC_MODE_MJPEGVBR;
                    }
                    case 2: { /* FIXQP */
                        return AX_VENC_RC_MODE_MJPEGFIXQP;
                    }
                    default:
                        break;
                }
            }
            case 2: {         /* H265 */
                switch (nRcType) {
                    case 0: { /* CBR */
                        return AX_VENC_RC_MODE_H265CBR;
                    }
                    case 1: { /* VBR */
                        return AX_VENC_RC_MODE_H265VBR;
                    }
                    case 2: { /* FIXQP */
                        return AX_VENC_RC_MODE_H265FIXQP;
                    }
                    default:
                        break;
                }
            }
            default:
                break;
        }
        return AX_VENC_RC_MODE_BUTT;
    }
};
