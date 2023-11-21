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
#include "istream.hpp"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class CFFMpegStream : public CStream0 {
public:
    CFFMpegStream(AX_VOID) = default;

    AX_BOOL Init(CONST STREAM_ATTR_T &stAttr) override;
    AX_BOOL DeInit(AX_VOID) override;

    AX_BOOL Start(AX_VOID) override;
    AX_BOOL Stop(AX_VOID) override;

protected:
    AX_VOID DemuxThread(AX_VOID *);

private:
    AX_S32 m_nVideoTrack = {-1};
    CONST AX_CHAR *m_url = {nullptr};
    AX_BOOL m_bLoop = {AX_TRUE};
    AVFormatContext *m_pAvFmtCtx = {nullptr};
    AVBSFContext *m_pAvBSFCtx = {nullptr};
    AVPacket *m_pAvPkt = {nullptr};
    CAXThread m_DemuxThread;
};
