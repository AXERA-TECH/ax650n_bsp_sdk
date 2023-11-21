/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "region.hpp"
#include <sys/prctl.h>
#include <chrono>
#include <map>
#include "AXFrame.hpp"
#include "AXThread.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"

#define TAG "REGION"
#define APP_REGION_CHANNEL_FILTER (0)
#define APP_REGION_SLEEP_MS (33)
#define APP_REGION_THREAD_UPDATE_NUM (10)

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif

CRegion* CRegion::CreateInstance(CONST NVR_REGION_ATTR_T& stAttr) {
    CRegion* obj = new (std::nothrow) CRegion;
    if (obj) {
        if (obj->Init(stAttr)) {
            return obj;
        } else {
            delete obj;
            obj = nullptr;
        }
    }

    return nullptr;
}

AX_VOID CRegion::Destory(AX_VOID) {
    DeInit();
    delete this;
}

AX_BOOL CRegion::Init(NVR_REGION_ATTR_T stAttr) {
    AX_S32 nRet = AX_SUCCESS;
    IVPS_RGN_HANDLE hRgn = AX_IVPS_INVALID_REGION_HANDLE;
    if (stAttr.enType == AX_NVR_RGN_TYPE::IVPS) {
        AX_U8 nChn = stAttr.nChn;
        hRgn = AX_IVPS_RGN_Create();
        if (AX_IVPS_INVALID_REGION_HANDLE != hRgn) {
            AX_U32 nIvpsGrp = stAttr.nGroup;
            AX_S32 nFilter = ((nChn + 1) << 4) + APP_REGION_CHANNEL_FILTER;
            nRet = AX_IVPS_RGN_AttachToFilter(hRgn, nIvpsGrp, nFilter);
            if (AX_SUCCESS != nRet) {
                LOG_M_E(TAG, "[%d] AX_IVPS_RGN_AttachToFilter(Grp: %d, Filter: 0x%x, Handle: %d) failed, ret=0x%x", nChn, nIvpsGrp, nFilter,
                        hRgn, nRet);
                AX_IVPS_RGN_Destroy(hRgn);
                return AX_FALSE;
            }
        }
    }
    m_RgnAttr.setTaskCfg(hRgn, stAttr, this);
    m_bStarted = AX_FALSE;
    return AX_TRUE;
}

AX_BOOL CRegion::DeInit() {
    AX_S32 nRet = AX_SUCCESS;
    if (m_RgnAttr.enType == AX_NVR_RGN_TYPE::IVPS) {
        AX_S32 hRgn = m_RgnAttr.nRgnHandle;
        if (APP_REGION_INVALID_VALUE == hRgn) {
            return AX_FALSE;
        }
        AX_U32 nIvpsGrp = m_RgnAttr.nGroup;

        AX_S32 nFilter = ((m_RgnAttr.nChn + 1) << 4) + APP_REGION_CHANNEL_FILTER;
        nRet = AX_IVPS_RGN_DetachFromFilter(hRgn, nIvpsGrp, nFilter);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(TAG, "AX_IVPS_RGN_DetachFromFilter(Grp: %d, Filter: %x, Handle: %d) failed, ret=0x%x", nIvpsGrp, nFilter, hRgn, nRet);
            return AX_FALSE;
        } else {
            LOG_M_I(TAG, "AX_IVPS_RGN_DetachFromFilter(Grp: %d, Filter: %x, Handle: %d) successfully.", nIvpsGrp, nFilter, hRgn);
        }
        nRet = AX_IVPS_RGN_Destroy(hRgn);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(TAG, "AX_IVPS_RGN_Destroy(Handle: %d) failed, ret=0x%x", hRgn, nRet);
            return AX_FALSE;
        } else {
            LOG_M_I(TAG, "AX_IVPS_RGN_Destroy(Handle: %d) successfully.", hRgn, nRet);
        }
        m_RgnAttr.nRgnHandle = APP_REGION_INVALID_VALUE;
    } else {
        m_RgnAttr.pFb = nullptr;
    }

    m_RgnAttr.enType = AX_NVR_RGN_TYPE::BUTT;
    m_bStarted = AX_FALSE;

    return AX_TRUE;
}

AX_BOOL CRegion::Start() {
    if (m_RgnAttr.enType == AX_NVR_RGN_TYPE::IVPS) {
        CRegionTask::GetInstance()->RegisterTask(m_RgnAttr);
    } else if (m_RgnAttr.enType == AX_NVR_RGN_TYPE::VOFB) {
        std::lock_guard<std::mutex> lock(m_mtxRect);
        m_bStarted = AX_TRUE;
    }
    return AX_TRUE;
}

AX_BOOL CRegion::Stop() {
    if (m_RgnAttr.enType == AX_NVR_RGN_TYPE::IVPS) {
        CRegionTask::GetInstance()->UnRegisterTask(m_RgnAttr);
    } else if (m_RgnAttr.enType == AX_NVR_RGN_TYPE::VOFB){
        std::lock_guard<std::mutex> lock(m_mtxRect);
        m_bStarted = AX_FALSE;
        if (m_RgnAttr.pFb) {
            m_RgnAttr.pFb->DestroyChannel(m_RgnAttr.nVoChannel);
        }
    }
    return AX_TRUE;
}

AX_BOOL CRegion::EnableAiRegion(AX_BOOL bEnable /*= AX_TRUE*/) {
    // TODO: for frame buffer
    if (m_RgnAttr.enType == AX_NVR_RGN_TYPE::IVPS) {
        if (APP_REGION_INVALID_VALUE == m_RgnAttr.nRgnHandle) {
            return AX_FALSE;
        }

        if (!bEnable) {
            CRegionTask::GetInstance()->UnRegisterTask(m_RgnAttr);
            ClearAiRegion();
        } else {
            CRegionTask::GetInstance()->RegisterTask(m_RgnAttr);
        }
    }

    return AX_TRUE;
}

AX_VOID CRegion::ClearAiRegion() {
    if (m_RgnAttr.enType == AX_NVR_RGN_TYPE::IVPS) {
        if (APP_REGION_INVALID_VALUE == m_RgnAttr.nRgnHandle) {
            return;
        }
        AX_IVPS_RGN_DISP_GROUP_T tDisp;
        memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
        tDisp.nNum = 0;
        tDisp.tChnAttr.nAlpha = 200;
        tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
        tDisp.tChnAttr.nZindex = 0;
        AX_S32 ret = AX_IVPS_RGN_Update(m_RgnAttr.nRgnHandle, &tDisp);
        if (0 != ret) {
            LOG_M_E(TAG, "Clear ai regions %d failed.", m_RgnAttr.nRgnHandle);
        } else {
            LOG_M_I(TAG, "Clear ai regions %d ok.", m_RgnAttr.nRgnHandle);
        }
    }
}

AX_BOOL CRegion::GetRectDisp(const NVR_REGION_TASK_CFG_T &attr, AX_IVPS_RGN_DISP_GROUP_T& tDisp) {
    // dst size for TDP
    AX_U32 nDstWidth = attr.nWidth;
    AX_U32 nDstHeight = attr.nHeight;

    CVO *pVo = attr.pVo;
    if (pVo) {
        VO_ATTR_T attrVo = pVo->GetAttr();
        nDstWidth = attr.nW;//attrVo.stChnInfo.arrChns[attr.nVoChannel].stRect.u32Width;
        nDstHeight = attr.nH;//attrVo.stChnInfo.arrChns[attr.nVoChannel].stRect.u32Height;
    }

    // printf("xxxxxxxxxxxxx %dx%d\n", nDstWidth, nDstHeight);

    AX_IVPS_PIPELINE_ATTR_T stPipeAttr;
    AX_S32 ret = AX_IVPS_GetPipelineAttr(m_RgnAttr.nGroup, &stPipeAttr);
    if (0 != ret) {
        LOG_M_E(TAG, "%s: AX_IVPS_GetPipelineAttr(ivGrp %d) fail, ret = 0x%x", __func__, m_RgnAttr.nGroup, ret);
        return AX_FALSE;
    }

    AX_IVPS_FILTER_T& stChnFilter0 = stPipeAttr.tFilter[m_RgnAttr.nChn + 1][0];

    AX_U32 index = 0;
    DETECT_RESULT_T &fhvp = m_stRect;
    // GetDetectedRect(fhvp);
    int nIndex = 0;

    for (; index < fhvp.nCount; ++index) {
        if (stChnFilter0.bCrop) {
            int nCropX1 = stChnFilter0.tCropRect.nX;
            int nCropY1 = stChnFilter0.tCropRect.nY;
            int nCropX2 = stChnFilter0.tCropRect.nX + stChnFilter0.tCropRect.nW;
            int nCropY2 = stChnFilter0.tCropRect.nY + stChnFilter0.tCropRect.nH;
            int nCropW = stChnFilter0.tCropRect.nW;
            int nCropH = stChnFilter0.tCropRect.nH;

            int nSrcRgnX1 = fhvp.item[index].tBox.fX * attr.nSrcW / fhvp.nW;
            int nSrcRgnY1 = fhvp.item[index].tBox.fY * attr.nSrcH / fhvp.nH;
            int nSrcRgnX2 = nSrcRgnX1 + fhvp.item[index].tBox.fW * attr.nSrcW / fhvp.nW;
            int nSrcRgnY2 = nSrcRgnY1 + fhvp.item[index].tBox.fH * attr.nSrcH / fhvp.nH;

            // Not included overlap of border
            // if (nCropX2 >= nSrcRgnX1 && nSrcRgnX2 >= nCropX1 && nCropY2 >= nSrcRgnY1 && nSrcRgnY2 >= nCropY1) {
            if (nCropX2 > nSrcRgnX1 && nSrcRgnX2 > nCropX1 && nCropY2 > nSrcRgnY1 && nSrcRgnY2 > nCropY1) {
                int nX1 = std::max(nCropX1, nSrcRgnX1) - nCropX1;
                int nY1 = std::max(nCropY1, nSrcRgnY1) - nCropY1;
                int nX2 = std::min(nCropX2, nSrcRgnX2) - nCropX1;
                int nY2 = std::min(nCropY2, nSrcRgnY2) - nCropY1;

                int nFx = nX1 * nDstWidth / nCropW;
                int nFy = nY1 * nDstHeight / nCropH;
                int nFw = (nX2 - nX1) * nDstWidth / nCropW;
                int nFh = (nY2 - nY1) * nDstHeight / nCropH;
                if (!nFw || !nFh) {
                    continue;
                }

                tDisp.arrDisp[nIndex].uDisp.tPolygon.tRect.nX = nFx;
                tDisp.arrDisp[nIndex].uDisp.tPolygon.tRect.nY = nFy;
                tDisp.arrDisp[nIndex].uDisp.tPolygon.tRect.nW = nFw;
                tDisp.arrDisp[nIndex].uDisp.tPolygon.tRect.nH = nFh;
            } else {
                continue;
            }

        } else {
            tDisp.arrDisp[nIndex].uDisp.tPolygon.tRect.nX = (fhvp.item[index].tBox.fX * nDstWidth / fhvp.nW);
            tDisp.arrDisp[nIndex].uDisp.tPolygon.tRect.nY = (fhvp.item[index].tBox.fY * nDstHeight / fhvp.nH);
            tDisp.arrDisp[nIndex].uDisp.tPolygon.tRect.nW = (fhvp.item[index].tBox.fW * nDstWidth / fhvp.nW);
            tDisp.arrDisp[nIndex].uDisp.tPolygon.tRect.nH = (fhvp.item[index].tBox.fH * nDstHeight / fhvp.nH);
        }

        tDisp.arrDisp[nIndex].eType = AX_IVPS_RGN_TYPE_RECT;
        tDisp.arrDisp[nIndex].bShow = AX_TRUE;
        tDisp.arrDisp[nIndex].uDisp.tPolygon.bSolid = AX_TRUE;
        tDisp.arrDisp[nIndex].uDisp.tPolygon.nLineWidth = 2;
        tDisp.arrDisp[nIndex].uDisp.tPolygon.nColor = 0x00FF00;  // GREEN
        tDisp.arrDisp[nIndex].uDisp.tPolygon.nAlpha = 255;
        nIndex++;

        LOG_MM_I(TAG, " nGrp:%d, nDstWidth:%d, nDstHeight:%d Rect: [nx:%d,x:%f, ny:%d,y:%f, nw:%d,w:%f, nh:%dh:%f]", m_RgnAttr.nGroup,
                 nDstWidth, nDstHeight, tDisp.arrDisp[index].uDisp.tPolygon.tRect.nX, fhvp.item[index].tBox.fX,
                 tDisp.arrDisp[index].uDisp.tPolygon.tRect.nY, fhvp.item[index].tBox.fY, tDisp.arrDisp[index].uDisp.tPolygon.tRect.nW,
                 fhvp.item[index].tBox.fW, tDisp.arrDisp[index].uDisp.tPolygon.tRect.nH, fhvp.item[index].tBox.fH);
    }
    tDisp.nNum = nIndex;
    return AX_TRUE;
}

AX_BOOL CRegion::SetDetectedRect(DETECT_RESULT_T& stRect) {
    std::lock_guard<std::mutex> lock(m_mtxRect);
    AX_BOOL bRet = AX_FALSE;

    do {
        m_stRect = stRect;
        if (m_RgnAttr.enType == AX_NVR_RGN_TYPE::VOFB && m_bStarted) {

            CVO *pVo = m_RgnAttr.pVo;
            if (pVo == nullptr) {
                break;
            }
            VO_ATTR_T attr = pVo->GetAttr();
            AX_U32 x = 0; //attr.stChnInfo.arrChns[nVoChn].stRect.u32X;
            AX_U32 y = 0; //attr.stChnInfo.arrChns[nVoChn].stRect.u32Y;

            CFramebufferPaint::RectList rects;
            AX_IVPS_RGN_DISP_GROUP_T tDisp;
            if (!this->GetRectDisp(m_RgnAttr, tDisp)) {
                break;
            }
            for (AX_U32 i = 0; i < tDisp.nNum; ++i) {
                rects.emplace_back(FB_RECT_T{tDisp.arrDisp[i].uDisp.tPolygon.tRect.nX + x,
                                            tDisp.arrDisp[i].uDisp.tPolygon.tRect.nY + y,
                                            tDisp.arrDisp[i].uDisp.tPolygon.tRect.nW,
                                            tDisp.arrDisp[i].uDisp.tPolygon.tRect.nH});
            }

            VO_CHN nVoChn = m_RgnAttr.nVoChannel;
            if (!pVo->IsHidden(nVoChn) && m_RgnAttr.pFb) {
                if (!m_RgnAttr.pFb->DrawChannelRects((AX_U8)nVoChn, rects)) {
                    break;
                }
            }
        }
        bRet = AX_TRUE;
    } while (0);
    return bRet;
}

AX_VOID CRegion::GetDetectedRect(DETECT_RESULT_T& stRect) {
    std::lock_guard<std::mutex> lock(m_mtxRect);
    stRect = m_stRect;
}

AX_BOOL CRegionTask::InitOnce() {
    return AX_TRUE;
}

AX_BOOL CRegionTask::RegisterTask(const NVR_REGION_TASK_CFG_T& stAttr) {
    std::lock_guard<std::mutex> lck(m_mtxVec);

    for (auto attr : m_vecRgnAttr) {
        if (attr.nGroup == stAttr.nGroup) {
            return AX_FALSE;
        }
    }

    m_vecRgnAttr.emplace_back(stAttr);
#if 0

    AX_U32 nRemained = (m_vecRgnAttr.size() % APP_REGION_THREAD_UPDATE_NUM) ? 1 : 0;
    AX_U32 nNum = (m_vecRgnAttr.size() / APP_REGION_THREAD_UPDATE_NUM) + nRemained;
    while (m_nTotalThreadNum != nNum) {
        CreateThread();
    }
#endif

    return AX_TRUE;
}

AX_VOID CRegionTask::UnRegisterTask(const NVR_REGION_TASK_CFG_T& stAttr) {
    std::lock_guard<std::mutex> lck(m_mtxVec);
    for (auto it = m_vecRgnAttr.begin(); it != m_vecRgnAttr.end(); it++) {
        if (it->nGroup == stAttr.nGroup) {
            m_vecRgnAttr.erase(it);
            break;
        }
    }

#if 0
    if (m_vecRgnAttr.empty()) {
        Stop();
        return;
    }

    AX_U32 nRemained = (m_vecRgnAttr.size() % APP_REGION_THREAD_UPDATE_NUM) ? 1 : 0;
    AX_U32 nNum = (m_vecRgnAttr.size() / APP_REGION_THREAD_UPDATE_NUM) + nRemained;
    if (m_nTotalThreadNum > nNum) {
        DestoryThread();
        return;
    }
#endif
    return;
}

AX_BOOL CRegionTask::CreateThread(AX_VOID) {
    if (m_nTotalThreadNum >= APP_REGION_MAX_THREAD) {
        LOG_MM_E(TAG, "The number of threads >=  APP_REGION_MAX_THREAD[%d]", APP_REGION_MAX_THREAD);
        return AX_FALSE;
    }
    AX_CHAR strThreadName[32];
    sprintf(strThreadName, "RGNTask-T%d", m_nTotalThreadNum++);
    if (!m_thread[m_nTotalThreadNum - 1].Start(std::bind(&CRegionTask::UpdateThread, this, std::placeholders::_1),
                                               (AX_VOID*)&m_nTotalThreadNum, strThreadName)) {
        LOG_MM_E(TAG, "create detect thread fail");
        return AX_FALSE;
    }
    m_bStarted = AX_TRUE;
    return AX_TRUE;
}

AX_VOID CRegionTask::DestoryThread(AX_VOID) {
    if (m_nTotalThreadNum && m_thread[m_nTotalThreadNum - 1].IsRunning()) {
        m_thread[m_nTotalThreadNum - 1].Stop();
        m_thread[m_nTotalThreadNum - 1].Join();
        m_nTotalThreadNum--;
    }
}

AX_VOID CRegionTask::Stop(AX_VOID) {
    if (m_bStarted) {
        // if (!m_vecRgnAttr.empty()) {
        //     LOG_MM_E(TAG, "RGN count not empty");
        //     std::lock_guard<std::mutex> lck(m_mtxVec);
        //     m_vecRgnAttr.clear();
        // }

        for (int i = 0; i < APP_REGION_MAX_THREAD; i++) {
            if (m_thread[i].IsRunning()) {
                m_thread[i].Stop();
                m_thread[i].Join();
            }
        }
        m_bStarted = AX_FALSE;
    }
}

AX_VOID CRegionTask::Start(AX_VOID) {
    if (!m_bStarted) {
        AX_U32 nThreadId = 0;
        for (int i = 0; i < APP_REGION_MAX_THREAD; i++) {

            AX_CHAR strThreadName[32];
            sprintf(strThreadName, "RGNTask-T%d", i);
            // if (!m_thread[i].Start([this](AX_VOID* pArg) -> AX_VOID { UpdateThread(pArg); }, &i, "preview")) {
            if (!m_thread[i].Start(std::bind(&CRegionTask::UpdateThread, this, std::placeholders::_1), (AX_VOID*)&i, strThreadName)) {
                LOG_MM_E(TAG, "create detect thread fail");
            }
        }

        m_bStarted = AX_TRUE;
    }
}

AX_BOOL CRegionTask::UpdateRegionAttr(const NVR_REGION_TASK_CFG_T& stAttr) {
    std::lock_guard<std::mutex> lck(m_mtxVec);
    for (auto& attr : m_vecRgnAttr) {
        if (attr.nGroup == stAttr.nGroup && attr.nChn == stAttr.nChn) {
            attr = stAttr;
            return AX_TRUE;
        }
    }
    return AX_FALSE;
}

AX_VOID CRegionTask::UpdateThread(AX_VOID* pArg) {

    AX_S32 nthreadID = *(AX_U32*)pArg;
    LOG_M_D(TAG, "[%s][%d] +++ nthreadID = %d", __func__, __LINE__, nthreadID);

    AX_S32 ret = IVPS_SUCC;
    AX_U32 nSleepMs = APP_REGION_SLEEP_MS;

    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    CFramebufferPaint::RectList rects;
    AX_BOOL bExit = AX_FALSE;
    AX_U32 nStart = nthreadID*APP_REGION_THREAD_UPDATE_NUM;
    AX_U32 nEnd = (nthreadID + 1)*APP_REGION_THREAD_UPDATE_NUM - 1;
    AX_U32 size = m_vecRgnAttr.size();

    while (1) {
        if (!m_thread[nthreadID].IsRunning()) {
            break;
        }

        if (bExit) {
            break;
        }

        AX_U64 startTime = CElapsedTimer::GetInstance()->GetTickCount();

        {
            std::lock_guard<std::mutex> lck(m_mtxVec);
            size = m_vecRgnAttr.size();
            if (0 == size) {
                // CElapsedTimer::mSleep(1000);
                if (!m_thread[nthreadID].IsRunning()) {
                    break;
                }

            } else {

                for (AX_U32 i = nStart; i < nEnd && i < size; ++i) {
                    if (!m_thread[nthreadID].IsRunning()) {
                        bExit = AX_TRUE;
                        break;
                    }
                    auto rgn = m_vecRgnAttr[i];

                    // Action
                    if (rgn.enType == AX_NVR_RGN_TYPE::BUTT) {
                        break;
                    }

                    if (!rgn.pRegion->GetRectDisp(rgn, tDisp)) {
                        break;
                    }

                    if (rgn.enType == AX_NVR_RGN_TYPE::VOFB) {
                        CVO *pVo = rgn.pRegion->m_RgnAttr.pVo;
                        if (pVo == nullptr) {
                            // error
                            break;
                        }

                        VO_CHN nVoChn = rgn.pRegion->m_RgnAttr.nVoChannel;
                        VO_ATTR_T attr = pVo->GetAttr();
                        AX_U32 x = 0; //attr.stChnInfo.arrChns[nVoChn].stRect.u32X;
                        AX_U32 y = 0; //attr.stChnInfo.arrChns[nVoChn].stRect.u32Y;
                        rects.clear();
                        for (AX_U32 i = 0; i < tDisp.nNum; ++i) {
                            rects.emplace_back(FB_RECT_T{tDisp.arrDisp[i].uDisp.tPolygon.tRect.nX + x,
                                                        tDisp.arrDisp[i].uDisp.tPolygon.tRect.nY + y,
                                                        tDisp.arrDisp[i].uDisp.tPolygon.tRect.nW,
                                                        tDisp.arrDisp[i].uDisp.tPolygon.tRect.nH});
                        }

                        if (!pVo->IsHidden(nVoChn) && rgn.pRegion->m_RgnAttr.pFb) {
                            if (!rgn.pRegion->m_RgnAttr.pFb->DrawChannelRects((AX_U8)nVoChn, rects)) {
                                break;
                            }
                        }
                    }
                    else {
                        if (rgn.nRgnHandle != APP_REGION_INVALID_VALUE) {
                            AX_S32 ret = AX_IVPS_RGN_Update(rgn.nRgnHandle, &tDisp);
                            if (IVPS_SUCC != ret) {
                                LOG_MM_E(TAG, "AX_IVPS_RGN_Update fail, ret=0x%x, size=%d hRgn=%d", ret, m_vecRgnAttr.size(), rgn.nRgnHandle);
                                break;
                            }
                        }
                    }
                }
            }
        }

        AX_U64 endTime = CElapsedTimer::GetInstance()->GetTickCount();
        AX_U64 tookTime = endTime - startTime;
        if (tookTime > nSleepMs) {
            LOG_MM_W(TAG, "AX_IVPS_RGN_Update took time: %lld", tookTime);
        } else {
            CElapsedTimer::GetInstance()->mSleep(nSleepMs - tookTime);
        }

        if (!m_thread[nthreadID].IsRunning()) {
            break;
        }

        if (bExit) {
            break;
        }

    }

    LOG_M_D(TAG, "[%s][%d] --- nthreadID = %d\n", __func__, __LINE__, nthreadID);
}
