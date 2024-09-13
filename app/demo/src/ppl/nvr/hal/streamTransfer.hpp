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

#include "streamContainer.hpp"
#include "VideoEncoderEx.hpp"
#include "istream.hpp"
#include <memory>
#include <list>

class CVideoStreamTransfer : public IVencPackObserver {
public:
    CVideoStreamTransfer() = default;

    AX_BOOL Init(AX_S32 nStream, AX_U32 nFps, AX_U32 nGop);
    AX_BOOL DeInit();
    AX_BOOL Start();
    AX_BOOL Stop();

    AX_BOOL SendStream(AX_S32 nStream, CONST AX_VENC_PACK_T& tVencPacked, AX_BOOL bGopStart = AX_TRUE, AX_S32 nTimeOut = INFINITE);

    AX_BOOL RegisterObserver(IStreamObserver* pObs);
    AX_BOOL UnRegisterObserver(IStreamObserver* pObs);

protected:
    AX_BOOL OnRecvStreamPack(AX_S32 nStream, CONST AX_VENC_PACK_T& stPack, AX_BOOL bGopStart = AX_TRUE) override;
    AX_VOID DispatchThread(AX_VOID*);

private:
    AX_BOOL NotifyAll(VIDEO_STREAM_INFO_T* pStream);

private:
    std::unique_ptr<CVideoStreamContainer> m_pStreamContainer;
    std::list<IStreamObserver*> m_lstObs;
    std::mutex m_mtxObs;
    AX_S32 m_nStreamID {-1};
    AX_U32 m_nGop {30};
    AX_U32 m_nFps {30};
    AX_S32 m_nLastGopIndex {-1};

    CAXThread m_threadDispatch;
};
