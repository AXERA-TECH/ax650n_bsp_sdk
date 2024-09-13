/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "ITSFrameCollector.h"
#include "GlobalDef.h"

#define COLLECT "COLLECT"

using namespace std;

CFrameCollector::CFrameCollector(AX_U8 nGroup) : m_nGroup(nGroup) {
}

AX_BOOL CFrameCollector::Init() {
    return AX_TRUE;
}

AX_BOOL CFrameCollector::DeInit() {
    return AX_TRUE;
}

AX_BOOL CFrameCollector::Start() {
    return AX_TRUE;
}

AX_BOOL CFrameCollector::Stop() {
    return AX_TRUE;
}

/*
 * @brief Receive two pipes of VIN yuv frames and dispatch to multiple destinations directly.
 */
AX_BOOL CFrameCollector::RecvFrame(AX_U8 nSrcGroup, AX_U8 nSrcChannel, CAXFrame* pFrame) {
    std::unique_lock<std::mutex> lock(m_mtxFrame);
    if (nullptr == pFrame) {
        return AX_FALSE;
    }
    if (!IsTarget(nSrcGroup, nSrcChannel)) {
        LOG_MM_E(COLLECT, "collector[%d,0] src[%d,%d] don't register ,then release frame", m_nGroup, nSrcGroup, nSrcChannel);
        pFrame->FreeMem();
        return AX_FALSE;
    }
    /* Set multiplex flag for SensorMgr to release this frame when all observers finished dealing with it. */
    // if (m_vecObserver.size() > 1) {
    //     pFrame->bMultiplex = AX_TRUE;
    // }

    /*Make sure every observer is notified*/
    pFrame->bMultiplex = AX_TRUE;
    pFrame->IncFrmRef();
    NotifyAll(pFrame);
    pFrame->FreeMem();

    return AX_TRUE;
}

AX_VOID CFrameCollector::RegObserver(IObserver* pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nGroup = m_nGroup;
        tTransAttr.nChannel = 0;
        tTransAttr.bLink = AX_FALSE;
        tTransAttr.nWidth = m_stAttr.nWidth;
        tTransAttr.nHeight = m_stAttr.nHeight;
        tTransAttr.nSnsSrc = m_stAttr.nSnsSrc;
        tTransAttr.bEnableFBC = m_stAttr.bEnableFBC;
        tTransAttr.fFramerate = m_stAttr.fFramerate;
        LOG_MM_I(COLLECT, "m_nGroup:%d, tTransAttr.nWidth:%d, height:%d, frameRate:%lf", m_nGroup, tTransAttr.nWidth, tTransAttr.nHeight,
                 tTransAttr.fFramerate);
        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_COLLECT, m_nGroup, 0, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CFrameCollector::UnregObserver(IObserver* pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        if (*it == pObserver) {
            m_vecObserver.erase(it);
            break;
        }
    }
}

AX_VOID CFrameCollector::NotifyAll(AX_VOID* pFrame) {
    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        ((CAXFrame*)pFrame)->IncFrmRef();
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_COLLECT, m_nGroup, 0, pFrame);
    }
}

AX_VOID CFrameCollector::RegTargetChannel(AX_U8 nGroup, AX_U8 nChannel) {
    LOG_MM_I(COLLECT, "RegTargetChannel nGroup:%d,nChannel:%d", nGroup, nChannel);
    m_vecTargetChannel.emplace_back(std::make_pair(nGroup, nChannel));
}

AX_BOOL CFrameCollector::IsTarget(AX_U8 nGroup, AX_U8 nChannel) {
    for (auto& pair : m_vecTargetChannel) {
        if (pair.first == nGroup && pair.second == nChannel) {
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}