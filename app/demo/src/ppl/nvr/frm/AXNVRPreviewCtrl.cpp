/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "AXNVRPreviewCtrl.h"
#include "AppLogApi.h"

#define TAG "PREVIEW"


AX_VOID CAXNVRPreviewCtrl::Init(const AX_NVR_DEVICE_MGR_ATTR_T &stAttr) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    if (m_bInit) {
        return;
    }

    m_bInit = AX_TRUE;
    m_stAttr = std::move(stAttr);
    for (AX_U32 nDevID = 0; nDevID < m_stAttr.nMaxCount; ++nDevID) {
        AX_NVR_DEVICE_T stDevice;
        stDevice.pRPatrolChn = this->createRPatrolChannel(nDevID);
        stDevice.pThreadRPatrol = new (std::nothrow) CAXThread;
        stDevice.pThreadRPatrolUpdate = new (std::nothrow) CAXThread;
        stDevice.pPreviewChnMain = this->createPreviewChannel(nDevID, AX_NVR_CHN_IDX_TYPE::MAIN);
        stDevice.pPreviewChnSub1 = this->createPreviewChannel(nDevID, AX_NVR_CHN_IDX_TYPE::SUB1);
        stDevice.pThreadPreview = new (std::nothrow) CAXThread;
        m_mapDevice.emplace(nDevID, stDevice);
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_VOID CAXNVRPreviewCtrl::DeInit() {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    if (!m_bInit) {
        return;
    }

    m_bInit = AX_FALSE;

    for (auto& it : m_mapDevice) {
        const auto& nDevID = it.first;
        auto& stDevice = it.second;

        // deinit round patrol channel
        if (stDevice.pRPatrolChn) {
            stDevice.pRPatrolChn->StopDisp();
            stDevice.pRPatrolChn->StopRtsp(AX_TRUE);
            delete stDevice.pRPatrolChn;
            stDevice.pRPatrolChn = nullptr;
        }

        // deinit preview channel - main
        if (stDevice.pPreviewChnMain) {
            stDevice.pPreviewChnMain->StopDisp();
            stDevice.pPreviewChnMain->StopRtsp(AX_TRUE);
            delete stDevice.pPreviewChnMain;
            stDevice.pPreviewChnMain = nullptr;
        }

        // deinit preview channel - main
        if (stDevice.pPreviewChnSub1) {
            stDevice.pPreviewChnSub1->StopDisp();
            stDevice.pPreviewChnSub1->StopRtsp(AX_TRUE);
            delete stDevice.pPreviewChnSub1;
            stDevice.pPreviewChnSub1 = nullptr;
        }

        if (stDevice.pThreadRPatrol) {
            delete stDevice.pThreadRPatrol;
            stDevice.pThreadRPatrol = nullptr;
        }

        if (stDevice.pThreadRPatrolUpdate) {
            delete stDevice.pThreadRPatrolUpdate;
            stDevice.pThreadRPatrolUpdate = nullptr;
        }

        if (stDevice.pThreadPreview) {
            delete stDevice.pThreadPreview;
            stDevice.pThreadPreview = nullptr;
        }
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_BOOL CAXNVRPreviewCtrl::InsertDevice(const AX_NVR_DEV_INFO_T &stDeviceInfo) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        auto it = m_mapDevice.find(stDeviceInfo.nChannelId);
        if (it == m_mapDevice.end()) break;
        auto& stDevice = it->second;

        if (stDevice.stDevInfo.nChannelId != -1) break;
        stDevice.stDevInfo = stDeviceInfo;

        // start preview channel rtsp and record
        if (!this->startRtsp(stDevice.pPreviewChnMain, stDevice.stDevInfo.stChnMain, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::MAIN, stDevice.stDevInfo.nChannelId))) {
            break;
        }
        if (!this->startRtsp(stDevice.pPreviewChnSub1, stDevice.stDevInfo.stChnSub1, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::SUB1, stDevice.stDevInfo.nChannelId))) {
            break;
        }

        bRet = AX_TRUE;
    } while(0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

AX_BOOL CAXNVRPreviewCtrl::UpdateDevice(const AX_NVR_DEV_INFO_T &stDeviceInfo) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        auto it = m_mapDevice.find(stDeviceInfo.nChannelId);
        if (it == m_mapDevice.end()) break;
        auto& stDevice = it->second;

        if (stDevice.stDevInfo.nChannelId == -1) break;
        if (stDevice.stDevInfo.nChannelId != stDeviceInfo.nChannelId) break;
        // FixMe: stDevInfoTmp for check to stop and start
        AX_NVR_DEV_INFO_T stDevInfoTmp = stDevice.stDevInfo;
        stDevice.stDevInfo = stDeviceInfo;

#if 0 //
        if (stDevInfoTmp.nType != it->second.stDevInfo.nType
        || 0 != strcmp(stDevInfoTmp.szRtspUrl, it->second.stDevInfo.szRtspUrl)
        || 0 != strcmp(stDevInfoTmp.szRtspUrlSecond, it->second.stDevInfo.szRtspUrlSecond)) {
            // do sth.
        }
#endif

        // stop and start preview channel rtsp and record
        stDevice.pPreviewChnMain->StopRtsp();
        if (!this->startRtsp(stDevice.pPreviewChnMain, stDevice.stDevInfo.stChnMain, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::MAIN, stDevice.stDevInfo.nChannelId))) {
            break;
        }

        stDevice.pPreviewChnSub1->StopRtsp();
        if (!this->startRtsp(stDevice.pPreviewChnSub1, stDevice.stDevInfo.stChnSub1, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::SUB1, stDevice.stDevInfo.nChannelId))) {
            break;
        }


        // stop and start round patrol channel rtsp and display
        stDevice.pRPatrolChn->StopRtsp();
        if (AX_NVR_CHN_IDX_TYPE::MAIN == stDevice.stDevInfo.enPatrolIndex) {
            if (!this->startRtsp(stDevice.pRPatrolChn, stDevice.stDevInfo.stChnMain, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PATROL, stDevice.stDevInfo.enPatrolIndex, stDevice.stDevInfo.nChannelId))) {
                break;
            }
        } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == stDevice.stDevInfo.enPatrolIndex) {
            if (!this->startRtsp(stDevice.pRPatrolChn, stDevice.stDevInfo.stChnSub1, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PATROL, stDevice.stDevInfo.enPatrolIndex, stDevice.stDevInfo.nChannelId))) {
                break;
            }
        }

        bRet = AX_TRUE;
    } while(0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

AX_BOOL CAXNVRPreviewCtrl::DeleteDevice(const vector<AX_NVR_DEV_ID> &vecDevID) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    do {

        for (auto nDevID:vecDevID) {

            auto it = m_mapDevice.find(nDevID);
            if (it == m_mapDevice.end()) break;
            if (it->second.stDevInfo.nChannelId == -1) break;
            if (it->second.stDevInfo.nChannelId != nDevID) break;
            it->second.stDevInfo.nChannelId = -1;

            // stop preview channel rtsp and record
            it->second.pPreviewChnMain->StopRtsp(AX_TRUE);
            it->second.pPreviewChnSub1->StopRtsp(AX_TRUE);

            // stop round patrol channel rtsp and display
            it->second.pRPatrolChn->StopDisp();
            it->second.pRPatrolChn->StopRtsp(AX_TRUE);
        }

        bRet = AX_TRUE;
    } while(0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

AX_NVR_DEV_INFO_T CAXNVRPreviewCtrl::SelectDevice(AX_NVR_DEV_ID nDeviceID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = m_mapDevice.find(nDeviceID);
    if (it == m_mapDevice.end()) return AX_NVR_DEV_INFO_T();
    if (it->second.stDevInfo.nChannelId != nDeviceID) return AX_NVR_DEV_INFO_T();

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return it->second.stDevInfo;
}

vector<AX_NVR_DEV_ID> CAXNVRPreviewCtrl::GetFreeDevices() {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    vector<AX_NVR_DEV_ID> vecFreeDevices;
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& it : m_mapDevice) {
        const auto& nDevID = it.first;
        const auto& stDevice = it.second;
        if (stDevice.stDevInfo.nChannelId == -1) {
            vecFreeDevices.emplace_back(nDevID);
        }
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return vecFreeDevices;
}

vector<AX_NVR_DEV_INFO_T> CAXNVRPreviewCtrl::GetDevices() {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    vector<AX_NVR_DEV_INFO_T> vecDeviceList;
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& it : m_mapDevice) {
        const auto& nDevID = it.first;
        const auto& stDevice = it.second;
        if (stDevice.stDevInfo.nChannelId == -1) continue;
        if (stDevice.stDevInfo.nChannelId != nDevID) continue;
        vecDeviceList.emplace_back(stDevice.stDevInfo);
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return vecDeviceList;
}

AX_BOOL CAXNVRPreviewCtrl::ZoomAndMove(AX_NVR_DEV_ID nDeviceID, const AX_NVR_RECT_T &stCropRect, AX_BOOL bCrop) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (m_stAttr.pPrimary == nullptr) break;
        CVO *pVo = const_cast<CVO *>(m_stAttr.pPrimary->GetVo());
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nCount = attr.stChnInfo.nCount;

        auto it = m_mapDevice.find(nDeviceID);
        if (it == m_mapDevice.end()) break;
        if (it->second.stDevInfo.nChannelId == -1) break;
        if (it->second.stDevInfo.nChannelId != nDeviceID) break;
        if (it->second.pPreviewChnMain == nullptr) break;
        if (it->second.pPreviewChnSub1 == nullptr) break;
        const auto& stDevice = it->second;

        int nVoChn = stDevice.stDevInfo.nChannelId%nCount;

        AX_IVPS_RECT_T stCropRectTmp;
        stCropRectTmp.nX = stCropRect.x;
        stCropRectTmp.nY = stCropRect.y;
        stCropRectTmp.nW = stCropRect.w;
        stCropRectTmp.nH = stCropRect.h;

        if (bCrop) {
            if (stCropRectTmp.nW * 32 < attr.stChnInfo.arrChns[nVoChn].stRect.u32Width
                || stCropRectTmp.nH * 32 < attr.stChnInfo.arrChns[nVoChn].stRect.u32Height) {
                break;
            }

            pVo->UpdateChnCropRect(nVoChn, stCropRectTmp);
        } else {
            pVo->UpdateChnCropRect(nVoChn, {0, 0, 0, 0});
        }

        if (AX_NVR_CHN_IDX_TYPE::MAIN == stDevice.stDevInfo.enPreviewIndex) {
            bRet = stDevice.pPreviewChnMain->Crop(attr.stChnInfo.arrChns[nVoChn].stRect, stCropRectTmp, bCrop);
        } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == stDevice.stDevInfo.enPreviewIndex) {
            bRet = stDevice.pPreviewChnSub1->Crop(attr.stChnInfo.arrChns[nVoChn].stRect, stCropRectTmp, bCrop);
        }

    } while(0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

AX_BOOL CAXNVRPreviewCtrl::StartPip(AX_NVR_DEV_ID nDeviceID, AX_NVR_CHN_IDX_TYPE enIdx) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    do {

        AX_NVR_CHN_ATTR_T stAttr;
        stAttr.nDevID = nDeviceID;
        stAttr.enIndex = enIdx;
        stAttr.enView = AX_NVR_CHN_VIEW_TYPE::PREVIEW;
        stAttr.enStreamSrcType = AX_NVR_CHN_SRC_TYPE::RTSP;

        stAttr.enDetectSrcType = m_stAttr.enDetectSrc;
        stAttr.pDetector = m_stAttr.pDetect;
        stAttr.pDetectObs = m_stAttr.pDetectObs;

        stAttr.bRecord = AX_FALSE;
        stAttr.pRecord = nullptr;
        stAttr.pDisplay = m_stAttr.pPrimary;

        if (!m_chnPip.Init(stAttr)) {
            break;
        }

        auto it = m_mapDevice.find(nDeviceID);
        if (it == m_mapDevice.end()) {
            break;
        }

        auto stDevice = it->second;
        if (stDevice.stDevInfo.nChannelId == -1) continue;
        if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;

        if (AX_NVR_CHN_IDX_TYPE::MAIN == enIdx) {
            stDevice.stDevInfo.stChnMain.bRecord = AX_FALSE;
            if (!this->startRtsp(&m_chnPip, stDevice.stDevInfo.stChnMain, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, enIdx, stDevice.stDevInfo.nChannelId, AX_TRUE))) {
                break;
            }
        } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == enIdx) {
            stDevice.stDevInfo.stChnSub1.bRecord = AX_FALSE;
            if (!this->startRtsp(&m_chnPip, stDevice.stDevInfo.stChnSub1, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, enIdx, stDevice.stDevInfo.nChannelId, AX_TRUE))) {
                break;
            }
        }

        // display
        CVO *pVo = const_cast<CVO*>(m_stAttr.pPrimary->GetVo());
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nVoLayer = attr.voLayer;
        int nVoChn = NVR_PIP_CHN_NUM;
        if (!m_chnPip.StartDisp(nVoLayer, nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect)) {
            break;
        } else {
            pVo->SetPipRect(attr.stChnInfo.arrChns[nVoChn].stRect);
        }

        bRet = AX_TRUE;
    } while(0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}


AX_BOOL CAXNVRPreviewCtrl::StopPip(AX_VOID) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    CVO *pVo = const_cast<CVO*>(m_stAttr.pPrimary->GetVo());
    if (pVo != nullptr) {
        pVo->SetPipRect({0, 0, 0, 0});
    }

    m_chnPip.StopDisp();
    m_chnPip.StopRtsp();

    bRet = AX_TRUE;

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

AX_VOID CAXNVRPreviewCtrl::StartRPatrolThread(AX_VOID* pArg) {
    AX_NVR_DEVICE_T* pDevice = (AX_NVR_DEVICE_T*)pArg;
    LOG_M_I(TAG, "[%s][%d] dev=%d +++ ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);

    do {

        if (m_stAttr.pSecond == nullptr) break;
        const CVO *pVo = m_stAttr.pSecond->GetVo();
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nCount = attr.stChnInfo.nCount;
        int nVoLayer = attr.voLayer;

        int nVoChn = pDevice->stDevInfo.nChannelId % nCount;

        if (AX_NVR_CHN_IDX_TYPE::MAIN == pDevice->stDevInfo.enPatrolIndex) {
            if (!this->startRtsp(pDevice->pRPatrolChn, pDevice->stDevInfo.stChnMain, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PATROL, AX_NVR_CHN_IDX_TYPE::MAIN, pDevice->stDevInfo.nChannelId))) {
                pDevice->bRes = AX_FALSE;
                break;
            }
        } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == pDevice->stDevInfo.enPatrolIndex) {
            if (!this->startRtsp(pDevice->pRPatrolChn, pDevice->stDevInfo.stChnSub1, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PATROL, AX_NVR_CHN_IDX_TYPE::SUB1, pDevice->stDevInfo.nChannelId))) {
                pDevice->bRes = AX_FALSE;
                break;
            }
        }
        if (pDevice->stDevInfo.bPatrolDisplay) {
            if (!pDevice->pRPatrolChn->StartDisp(nVoLayer, nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect)) {
                pDevice->bRes = AX_FALSE;
                break;
            }
        }
    } while(0);

    LOG_M_I(TAG, "[%s][%d] dev=%d --- ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);
}

AX_BOOL CAXNVRPreviewCtrl::StartRoundPatrol(const vector<AX_NVR_DEV_ID> &vecDevID) {
    LOG_MM_I(TAG, "+++");

    const AX_U32 MAX_RPATROL_CNT = MAX_DEVICE_ROUNDPATROL_COUNT;
    AX_BOOL bRet = AX_FALSE;
    std::lock_guard<std::mutex> lock(mutex_);

do {
        AX_BOOL bbRet = AX_TRUE;
        AX_U32 nIndex = 0;
        for (auto nDeviceID : vecDevID) {
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }

            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pRPatrolChn == nullptr) continue;

            if (!stDevice.pThreadRPatrol->Start([this](AX_VOID* pArg) -> AX_VOID { StartRPatrolThread(pArg); }, &stDevice, "patrol")) {
                LOG_M_E(TAG, "[%s][%d] Start stop channel %d thread failed.", __func__, __LINE__, stDevice.stDevInfo.nChannelId);
            }

            nIndex ++;
            if (nIndex >= MAX_RPATROL_CNT) {
                break;
            }
        }

        nIndex = 0;
        for (auto nDeviceID : vecDevID) {
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }
            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pRPatrolChn == nullptr) continue;

            stDevice.pThreadRPatrol->Stop();
            stDevice.pThreadRPatrol->Join();

            if (!stDevice.bRes) {
                bbRet = AX_FALSE;
            }

            nIndex ++;
            if (nIndex >= MAX_RPATROL_CNT) {
                break;
            }
        }

        bRet = bbRet;
    } while(0);

    LOG_MM_I(TAG, "---");
    return bRet;
}

AX_VOID CAXNVRPreviewCtrl::StopRPatrolThread(AX_VOID* pArg) {
    AX_NVR_DEVICE_T* pDevice = (AX_NVR_DEVICE_T*)pArg;
    LOG_M_I(TAG, "[%s][%d] dev=%d +++ ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);

    pDevice->pRPatrolChn->StopDisp();
    pDevice->pRPatrolChn->StopRtsp(AX_TRUE);

    LOG_M_I(TAG, "[%s][%d] dev=%d --- ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);
}

AX_BOOL CAXNVRPreviewCtrl::StopRoundPatrol(const vector<AX_NVR_DEV_ID> &vecDevID) {
    LOG_MM_I(TAG, "+++");

    std::lock_guard<std::mutex> lock(mutex_);

    AX_BOOL bRet = AX_FALSE;

    do {
        AX_BOOL bbRet = AX_TRUE;
        for (auto nDeviceID : vecDevID) {
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }

            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.pRPatrolChn == nullptr) continue;

            if (!stDevice.pThreadRPatrol->Start([this](AX_VOID* pArg) -> AX_VOID { StopRPatrolThread(pArg); }, &stDevice, "patrol")) {
                LOG_M_E(TAG, "[%s][%d] Start stop channel %d thread failed.", __func__, __LINE__, stDevice.stDevInfo.nChannelId);
                continue;
            }
        }

        for (auto nDeviceID : vecDevID) {
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }

            const auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.pRPatrolChn == nullptr) continue;

            stDevice.pThreadRPatrol->Stop();
            stDevice.pThreadRPatrol->Join();
        }

        bRet = bbRet;
    } while (0);

    LOG_MM_I(TAG, "---");

    return bRet;
}

AX_VOID CAXNVRPreviewCtrl::UpdateRPatrolThread(AX_VOID* pArg) {
    AX_NVR_DEVICE_T* pDevice = (AX_NVR_DEVICE_T*)pArg;
    LOG_M_I(TAG, "[%s][%d] dev=%d +++ ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);

    do {

        if (m_stAttr.pSecond == nullptr) break;
        const CVO *pVo = m_stAttr.pSecond->GetVo();
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nCount = attr.stChnInfo.nCount;
        int nVoLayer = attr.voLayer;

        int nVoChn = pDevice->stDevInfo.nChannelId % nCount;
        if (pDevice->stDevInfo.bPatrolDisplay) {
            if (!pDevice->pRPatrolChn->UpdateRPatrolRect(attr.stChnInfo.arrChns[nVoChn].stRect)) {
                pDevice->bRes = AX_FALSE;
                break;
            }
        }
    } while(0);

    LOG_M_I(TAG, "[%s][%d] dev=%d --- ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);
}

AX_BOOL CAXNVRPreviewCtrl::UpdateRoundPatrolPreview(const vector<AX_NVR_DEV_ID> &vecDevID) {
    LOG_MM_I(TAG, "+++");

    const AX_U32 MAX_RPATROL_CNT = MAX_DEVICE_ROUNDPATROL_COUNT;
    AX_BOOL bRet = AX_FALSE;
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        AX_BOOL bbRet = AX_TRUE;
        AX_U32 nIndex = 0;
        for (auto nDeviceID : vecDevID) {
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }
            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pRPatrolChn == nullptr) continue;

            if (!stDevice.pThreadRPatrolUpdate->Start([this](AX_VOID* pArg) -> AX_VOID { UpdateRPatrolThread(pArg); }, &stDevice, "patrol")) {
                LOG_M_E(TAG, "[%s][%d] Start update channel %d thread failed.", __func__, __LINE__, stDevice.stDevInfo.nChannelId);
            }

            nIndex ++;
            if (nIndex >= MAX_RPATROL_CNT) {
                break;
            }
        }

        nIndex = 0;
        for (auto nDeviceID : vecDevID) {
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }
            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pRPatrolChn == nullptr) continue;

            stDevice.pThreadRPatrolUpdate->Stop();
            stDevice.pThreadRPatrolUpdate->Join();

            if (!stDevice.bRes) {
                bbRet = AX_FALSE;
            }

            nIndex ++;
            if (nIndex >= MAX_RPATROL_CNT) {
                break;
            }
        }

        bRet = bbRet;
    } while(0);

    LOG_MM_I(TAG, "---");

    return bRet;
}

AX_VOID CAXNVRPreviewCtrl::StartPreviewThread(AX_VOID* pArg) {
    AX_NVR_DEVICE_T* pDevice = (AX_NVR_DEVICE_T*)pArg;
    LOG_M_D(TAG, "[%s][%d] dev=%d +++ ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);
    do {
        if (m_stAttr.pPrimary == nullptr) break;
        const CVO *pVo = m_stAttr.pPrimary->GetVo();
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nCount = attr.stChnInfo.nCount;
        int nVoLayer = attr.voLayer;
        int nVoChn = pDevice->stDevInfo.nChannelId%nCount;

        pDevice->bRes = AX_TRUE;
        if (AX_NVR_CHN_IDX_TYPE::MAIN == pDevice->stDevInfo.enPreviewIndex) {
            // init main rtsp and start within StartDisp
            if (!this->startRtsp(pDevice->pPreviewChnMain, pDevice->stDevInfo.stChnMain, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::MAIN, pDevice->stDevInfo.nChannelId))) {
                pDevice->bRes = AX_FALSE;
                break;
            }
            // init sub1 rtsp while record
            if (!this->startRtsp(pDevice->pPreviewChnSub1, pDevice->stDevInfo.stChnSub1, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::SUB1, pDevice->stDevInfo.nChannelId))) {
                pDevice->bRes = AX_FALSE;
                break;
            }
            // display
            if (pDevice->stDevInfo.bPreviewDisplay) {
                if (!pDevice->pPreviewChnMain->StartDisp(nVoLayer, nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect)) {
                    pDevice->bRes = AX_FALSE;
                    break;
                }
            }
        } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == pDevice->stDevInfo.enPreviewIndex) {
            // init main rtsp while record
            if (!this->startRtsp(pDevice->pPreviewChnMain, pDevice->stDevInfo.stChnMain, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::MAIN, pDevice->stDevInfo.nChannelId))) {
                pDevice->bRes = AX_FALSE;
                break;
            }
            // init sub1 rtsp and start within StartDisp
            if (!this->startRtsp(pDevice->pPreviewChnSub1, pDevice->stDevInfo.stChnSub1, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::SUB1, pDevice->stDevInfo.nChannelId))) {
                pDevice->bRes = AX_FALSE;
                break;
            }
            // display
            if (pDevice->stDevInfo.bPreviewDisplay) {
                if (!pDevice->pPreviewChnSub1->StartDisp(nVoLayer, nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect)) {
                    pDevice->bRes = AX_FALSE;
                    break;
                }
            }
        }

    } while (0);

    LOG_M_D(TAG, "[%s][%d] dev=%d --- ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);
}

AX_BOOL CAXNVRPreviewCtrl::StartPreview(const ax_nvr_channel_vector &vecChn) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

#if 1
    do {
        if (m_stAttr.pPrimary == nullptr) break;
        const CVO *pVo = m_stAttr.pPrimary->GetVo();
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nCount = attr.stChnInfo.nCount;
        int nVoLayer = attr.voLayer;

        AX_BOOL bbRet = AX_TRUE;

        for (auto &chn: vecChn) {
            AX_NVR_DEV_ID nDeviceID = get<1>(chn);
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }
            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pPreviewChnMain == nullptr) continue;
            if (stDevice.pPreviewChnSub1 == nullptr) continue;

            if (!stDevice.pThreadPreview->Start([this](AX_VOID* pArg) -> AX_VOID { StartPreviewThread(pArg); }, &stDevice, "preview")) {
                LOG_M_E(TAG, "[%s][%d] Start stop channel %d thread failed.", __func__, __LINE__, stDevice.stDevInfo.nChannelId);
            }
        }

        for (auto &chn: vecChn) {
            AX_NVR_DEV_ID nDeviceID = get<1>(chn);
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }
            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pPreviewChnMain == nullptr) continue;
            if (stDevice.pPreviewChnSub1 == nullptr) continue;

            stDevice.pThreadPreview->Stop();
            stDevice.pThreadPreview->Join();

            if (!stDevice.bRes) {
                bbRet = AX_FALSE;
            }
        }

        bRet = bbRet;
    } while(0);

#else
    do {
        if (m_stAttr.pPrimary == nullptr) break;
        const CVO *pVo = m_stAttr.pPrimary->GetVo();
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nCount = attr.stChnInfo.nCount;
        int nVoLayer = attr.voLayer;

        AX_BOOL bbRet = AX_TRUE;
        for (auto &chn: vecChn) {
            AX_NVR_DEV_ID nDeviceID = get<1>(chn);
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }
            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pPreviewChnMain == nullptr) continue;
            if (stDevice.pPreviewChnSub1 == nullptr) continue;

            int nVoChn = stDevice.stDevInfo.nChannelId%nCount;

            if (AX_NVR_CHN_IDX_TYPE::MAIN == stDevice.stDevInfo.enPreviewIndex) {
                // start main rtsp
                if (!this->startRtsp(stDevice.pPreviewChnMain, stDevice.stDevInfo.stChnMain, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::MAIN, stDevice.stDevInfo.nChannelId))) {
                    break;
                }
                // start sub1 rtsp while record
                if (!this->startRtsp(stDevice.pPreviewChnSub1, stDevice.stDevInfo.stChnSub1, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::SUB1, stDevice.stDevInfo.nChannelId))) {
                    break;
                }
                // display
                if (!stDevice.pPreviewChnMain->StartDisp(nVoLayer, nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect)) {
                    bbRet = AX_FALSE;
                    break;
                }
            } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == stDevice.stDevInfo.enPreviewIndex) {
                // start main rtsp  while record
                if (!this->startRtsp(stDevice.pPreviewChnMain, stDevice.stDevInfo.stChnMain, AX_FALSE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::MAIN, stDevice.stDevInfo.nChannelId))) {
                    break;
                }
                // start sub1 rtsp
                if (!this->startRtsp(stDevice.pPreviewChnSub1, stDevice.stDevInfo.stChnSub1, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::SUB1, stDevice.stDevInfo.nChannelId))) {
                    break;
                }
                // display
                if (!stDevice.pPreviewChnSub1->StartDisp(nVoLayer, nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect)) {
                    bbRet = AX_FALSE;
                    break;
                }
            }

            LOG_M_D(TAG, "[%s][%d] Start %d --- ", __func__, __LINE__, nDeviceID);
        }
        bRet = bbRet;
    } while(0);
#endif
    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

AX_BOOL CAXNVRPreviewCtrl::UpdatePreview(const ax_nvr_channel_vector &vecChn) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (m_stAttr.pPrimary == nullptr) break;
        const CVO *pVo = m_stAttr.pPrimary->GetVo();
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nCount = attr.stChnInfo.nCount;
        int nVoLayer = attr.voLayer;

        AX_BOOL bbRet = AX_TRUE;
        for (auto &chn: vecChn) {
            AX_NVR_DEV_ID nDeviceID = get<1>(chn);
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                continue;
            }
            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pPreviewChnMain == nullptr) continue;
            if (stDevice.pPreviewChnSub1 == nullptr) continue;

            int nVoChn = stDevice.stDevInfo.nChannelId%nCount;
            // main display update
            if (AX_NVR_CHN_IDX_TYPE::MAIN == stDevice.stDevInfo.enPreviewIndex) {
                if (!it->second.pPreviewChnMain->UpdateRect(attr.stChnInfo.arrChns[nVoChn].stRect)) {
                    LOG_MM_W(TAG, "[%d] Update rect(%d, %d) failed.", nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect.u32Width, attr.stChnInfo.arrChns[nVoChn].stRect.u32Height);
                    bbRet = AX_FALSE;
                    continue;
                }
            }
            // sub1 display update
            else if (AX_NVR_CHN_IDX_TYPE::SUB1 == stDevice.stDevInfo.enPreviewIndex) {
                if (!it->second.pPreviewChnSub1->UpdateRect(attr.stChnInfo.arrChns[nVoChn].stRect)) {
                    LOG_MM_W(TAG, "[%d] Update rect(%d, %d) failed.", nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect.u32Width, attr.stChnInfo.arrChns[nVoChn].stRect.u32Height);
                    bbRet = AX_FALSE;
                    continue;
                }
            }
        }
        bRet = bbRet;
    } while(0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

AX_BOOL CAXNVRPreviewCtrl::SwitchPreviewMainSub(const ax_nvr_channel_vector &vecChn) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (m_stAttr.pPrimary == nullptr) break;
        CVO *pVo = const_cast<CVO *>(m_stAttr.pPrimary->GetVo());
        if (pVo == nullptr) break;
        VO_ATTR_T attr = pVo->GetAttr();
        int nCount = attr.stChnInfo.nCount;
        int nVoLayer = attr.voLayer;

        AX_BOOL bbRet = AX_TRUE;
        for (auto &chn: vecChn) {
            AX_NVR_DEV_ID nDeviceID = get<1>(chn);
            auto it = m_mapDevice.find(nDeviceID);
            if (it == m_mapDevice.end()) {
                bbRet = AX_FALSE;
                break;
            }
            auto& stDevice = it->second;
            if (stDevice.stDevInfo.nChannelId == -1) continue;
            if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
            if (stDevice.pPreviewChnMain == nullptr) continue;
            if (stDevice.pPreviewChnSub1 == nullptr) continue;

            int nVoChn = stDevice.stDevInfo.nChannelId % nCount;

            pVo->UpdateChnCropRect(nVoChn, {0, 0, 0, 0});

            // main to sub1
            if (AX_NVR_CHN_IDX_TYPE::MAIN == stDevice.stDevInfo.enPreviewIndex) {
                // stop main
                stDevice.pPreviewChnMain->StopDisp();
                stDevice.pPreviewChnMain->StopRtsp();
                // change channel stream type to sub1
                stDevice.stDevInfo.enPreviewIndex = AX_NVR_CHN_IDX_TYPE::SUB1;
                // start sub1 rtsp
                if (!this->startRtsp(stDevice.pPreviewChnSub1, stDevice.stDevInfo.stChnSub1, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::SUB1, stDevice.stDevInfo.nChannelId))) {
                    break;
                }
                if (!stDevice.pPreviewChnSub1->StartDisp(nVoLayer, nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect)) {
                    bbRet = AX_FALSE;
                    break;
                }
            }
            // sub1 to main
            else if (AX_NVR_CHN_IDX_TYPE::SUB1 == stDevice.stDevInfo.enPreviewIndex) {
                // stop sub1
                stDevice.pPreviewChnSub1->StopDisp();
                stDevice.pPreviewChnSub1->StopRtsp();
                // change channel stream type to main
                stDevice.stDevInfo.enPreviewIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
                // start main
                if (!this->startRtsp(stDevice.pPreviewChnMain, stDevice.stDevInfo.stChnMain, AX_TRUE, GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE::PREVIEW, AX_NVR_CHN_IDX_TYPE::MAIN, stDevice.stDevInfo.nChannelId))) {
                    break;
                }
                if (!stDevice.pPreviewChnMain->StartDisp(nVoLayer, nVoChn, attr.stChnInfo.arrChns[nVoChn].stRect)) {
                    bbRet = AX_FALSE;
                    break;
                }
            }
        }
        bRet = bbRet;
    } while(0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}


AX_VOID CAXNVRPreviewCtrl::StopPreviewThread(AX_VOID* pArg) {
    AX_NVR_DEVICE_T* pDevice = (AX_NVR_DEVICE_T*)pArg;
    LOG_M_D(TAG, "[%s][%d] dev=%d +++ ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);

    pDevice->pPreviewChnMain->StopDisp();
    pDevice->pPreviewChnMain->StopRtsp();
    pDevice->pPreviewChnSub1->StopDisp();
    pDevice->pPreviewChnSub1->StopRtsp();

    LOG_M_D(TAG, "[%s][%d] dev=%d --- ", __func__, __LINE__, pDevice->stDevInfo.nChannelId);
}

// AX_VOID CAXNVRPreviewCtrl::StopPreview(const ax_nvr_channel_vector &vecChn) {
//
//     LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
//     std::lock_guard<std::mutex> lock(mutex_);
//
//     do {
//         AX_BOOL bbRet = AX_TRUE;
//         for (auto &chn: vecChn) {
//             AX_NVR_DEV_ID nDeviceID = get<1>(chn);
//             auto it = m_mapDevice.find(nDeviceID);
//             if (it == m_mapDevice.end()) {
//                 bbRet = AX_FALSE;
//                 break;
//             }
//             auto& stDevice = it->second;
//             if (stDevice.stDevInfo.nChannelId == -1) continue;
//             if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
//             if (stDevice.pPreviewChnMain == nullptr) continue;
//             if (stDevice.pPreviewChnSub1 == nullptr) continue;
//
//             if (!stDevice.pThreadPreview->Start([this](AX_VOID* pArg) -> AX_VOID { StopPreviewThread(pArg); }, &stDevice, "preview")) {
//                 LOG_M_E(TAG, "[%s][%d] Start stop channel %d thread failed.", __func__, __LINE__, stDevice.stDevInfo.nChannelId);
//             }
//         }
//
//         for (auto &chn: vecChn) {
//             AX_NVR_DEV_ID nDeviceID = get<1>(chn);
//             auto it = m_mapDevice.find(nDeviceID);
//             if (it == m_mapDevice.end()) {
//                 bbRet = AX_FALSE;
//                 break;
//             }
//             auto& stDevice = it->second;
//             if (stDevice.stDevInfo.nChannelId == -1) continue;
//             if (stDevice.stDevInfo.nChannelId != nDeviceID) continue;
//             if (stDevice.pPreviewChnMain == nullptr) continue;
//             if (stDevice.pPreviewChnSub1 == nullptr) continue;
//
//             stDevice.pThreadPreview->Stop();
//             stDevice.pThreadPreview->Join();
//         }
//
//     } while(0);
//
//     LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
// }

AX_VOID CAXNVRPreviewCtrl::StopPreview(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    std::lock_guard<std::mutex> lock(mutex_);

#if 1
    // CRegionTask::GetInstance()->Stop();
    for (auto& it : m_mapDevice) {
        // const auto& nDevID = it.first;
        auto &stDevice = it.second;
        if (stDevice.stDevInfo.nChannelId == -1) continue;
        if (stDevice.pPreviewChnMain == nullptr) continue;
        if (stDevice.pPreviewChnSub1 == nullptr) continue;

        if (!stDevice.pThreadPreview->Start([this](AX_VOID* pArg) -> AX_VOID { StopPreviewThread(pArg); }, &stDevice, "preview")) {
            LOG_M_E(TAG, "[%s][%d] Start stop channel %d thread failed.", __func__, __LINE__, stDevice.stDevInfo.nChannelId);
        }
    }
    for (const auto& it : m_mapDevice) {
        // const auto& nDevID = it.first;
        const auto& stDevice = it.second;
        if (stDevice.stDevInfo.nChannelId == -1) continue;
        if (stDevice.pPreviewChnMain == nullptr) continue;
        stDevice.pThreadPreview->Stop();
        stDevice.pThreadPreview->Join();
    }

#else
    for (auto& it : m_mapDevice) {
        // const auto& nDevID = it.first;
        auto &stDevice = it.second;
        if (stDevice.stDevInfo.nChannelId == -1) continue;
        if (stDevice.pPreviewChnMain == nullptr) continue;
        if (stDevice.pPreviewChnSub1 == nullptr) continue;

        stDevice.pPreviewChnMain->StopDisp();
        stDevice.pPreviewChnMain->StopRtsp();
        stDevice.pPreviewChnSub1->StopDisp();
        stDevice.pPreviewChnSub1->StopRtsp();
    }
#endif
    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_BOOL CAXNVRPreviewCtrl::GetResolution(AX_NVR_DEV_ID nDeviceID, AX_U32 &nWidth, AX_U32 &nHeight) {
    AX_BOOL bRet = AX_FALSE;

    std::lock_guard<std::mutex> lock(mutex_);

    do {
        auto it = m_mapDevice.find(nDeviceID);
        if (it == m_mapDevice.end()) break;
        const auto& stDevice = it->second;
        if (stDevice.stDevInfo.nChannelId == -1) break;
        if (stDevice.stDevInfo.nChannelId != nDeviceID) break;

        if (AX_NVR_CHN_IDX_TYPE::MAIN == stDevice.stDevInfo.enPreviewIndex) {
            const AX_NVR_CHN_RES_T chnres = it->second.pPreviewChnMain->GetResolution();
            nWidth = chnres.w;
            nHeight = chnres.h;
        }
        else if (AX_NVR_CHN_IDX_TYPE::SUB1 == stDevice.stDevInfo.enPreviewIndex) {
            const AX_NVR_CHN_RES_T chnres = it->second.pPreviewChnSub1->GetResolution();
            nWidth = chnres.w;
            nHeight = chnres.h;
        }

        bRet = AX_TRUE;
    } while(0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

CAXNVRChannel *CAXNVRPreviewCtrl::createRPatrolChannel(AX_NVR_DEV_ID nDevID) {
    AX_NVR_CHN_ATTR_T stAttr;
    stAttr.nDevID = nDevID;
    stAttr.enView = AX_NVR_CHN_VIEW_TYPE::PATROL;
    stAttr.enStreamSrcType = AX_NVR_CHN_SRC_TYPE::RTSP;

    stAttr.enDetectSrcType = m_stAttr.enDetectSrc;
    stAttr.pDetector = m_stAttr.pDetect;
    stAttr.pDetectObs = m_stAttr.pDetectObs;

    stAttr.bRecord = AX_FALSE;
    stAttr.pDisplay = m_stAttr.pSecond;
    stAttr.nFrmBufNum = 3;
    return CAXNVRChannel::CreateInstance(stAttr);
}

CAXNVRChannel *CAXNVRPreviewCtrl::createPreviewChannel(AX_NVR_DEV_ID nDevID, AX_NVR_CHN_IDX_TYPE enIdx){
    AX_NVR_CHN_ATTR_T stAttr;
    stAttr.nDevID = nDevID;
    stAttr.enIndex = enIdx;
    stAttr.enView = AX_NVR_CHN_VIEW_TYPE::PREVIEW;
    stAttr.enStreamSrcType = AX_NVR_CHN_SRC_TYPE::RTSP;

    stAttr.enDetectSrcType = m_stAttr.enDetectSrc;
    stAttr.pDetector = m_stAttr.pDetect;
    stAttr.pDetectObs = m_stAttr.pDetectObs;

    stAttr.pRecord = m_stAttr.pRecord;
    stAttr.pDisplay = m_stAttr.pPrimary;
    return CAXNVRChannel::CreateInstance(stAttr);
}

AX_BOOL CAXNVRPreviewCtrl::startRtsp(CAXNVRChannel *pChannel, const AX_NVR_DEV_CHN_INFO_T &stChannelInfo, AX_BOOL bForce, AX_S32 nCookie) {
    if (!pChannel->StartRtsp(string(((const char*)stChannelInfo.szRtspUrl)), stChannelInfo.bRecord, bForce, nCookie)) {
        return AX_FALSE;
    }
    return AX_TRUE;
}

AX_S32 CAXNVRPreviewCtrl::GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE enView, AX_NVR_CHN_IDX_TYPE enChn, AX_NVR_DEV_ID devId, AX_BOOL bPip) {
    /*
        PREVIEW:
            pip : 128
            main: 0, 2, 4, 6, ... 126
            sub1: 1, 3, 5, 7, ... 127
        PATROL:
            129, 130, ... 161
    */
    if (AX_NVR_CHN_VIEW_TYPE::PREVIEW == enView) {
        if (bPip) {
            return MAX_DEVICE_COUNT * 2;
        } else {
            if (AX_NVR_CHN_IDX_TYPE::MAIN == enChn) {
                return devId * 2;
            } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == enChn) {
                return devId * 2 + 1;
            } else {
                return -1;
            }
        }
    } else if (AX_NVR_CHN_VIEW_TYPE::PATROL == enView) {
        return MAX_DEVICE_COUNT * 2 + 1 + devId;
    } else {
        return -1;
    }
}