/**********************************************************************************
 *
 * Copyright (c) 2019-2022 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#include "Md.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "AppLog.hpp"
#include "CommonUtils.hpp"
#include "ElapsedTimer.hpp"
#include "AlgoOptionHelper.h"
#include "IvesResult.hpp"

#define MD ("MD")
#define IS_MDCHN_CREATE(ch) ((ch) >= 0)

static inline AX_U32 GetConfidenceMBCount(AX_U8 nConfidence, const AX_IVES_RECT_T &stArea, const AX_IVES_MB_SIZE_T &stMB) {
    return (nConfidence * 1.0 / 100) * (stArea.u32W / stMB.u32W) * (stArea.u32H / stMB.u32H);
}

static inline AX_U32 align_down(AX_U32 x, AX_U32 a) {
    if (a > 0) {
        return ((x / a) * a);
    } else {
        return x;
    }
}

AX_BOOL CMD::Startup(AX_U32 nWidth, AX_U32 nHeight) {
    std::lock_guard<std::mutex> lck(m_mutx);
    LOG_M_D(MD, "+++");

    if (m_bInited) {
        LOG_M_W(MD, "MD is already inited");
        LOG_M_D(MD, "---");
        return AX_TRUE;
    }

    m_mdImgW = nWidth;
    m_mdImgH = nHeight;

    m_vecAreas.reserve(8);
    m_vecRslts.reserve(8);
    m_bInited = AX_TRUE;

    LOG_M_D(MD, "---");
    return AX_TRUE;
}

AX_VOID CMD::Cleanup(AX_VOID) {
    std::lock_guard<std::mutex> lck(m_mutx);
    LOG_MM_C(MD, "+++");

    if (m_bInited) {
        DestoryAreas();
        m_bInited = AX_FALSE;
    }

    LOG_MM_C(MD, "---");
}

AX_VOID CMD::DestoryAreas(AX_VOID) {
    for (auto &m : m_vecAreas) {
        if (m) {
            if (IS_MDCHN_CREATE(m->stAttr.mdChn)) {
                AX_S32 ret = AX_IVES_MD_DestoryChn(m->stAttr.mdChn);
                if (0 != ret) {
                    /* ignore error */
                    LOG_M_E(MD, "destory MD channel %d fail, ret = 0x%x", m->stAttr.mdChn, ret);
                } else {
                    LOG_MM_C(MD, "AX_IVES_MD_DestoryChn[%d]", m->stAttr.mdChn);
                }
            }

            free(m);
            m = nullptr;
        }
    }

    m_vecAreas.clear();
    m_vecRslts.clear();
}

AX_BOOL CMD::LoadConfig(AX_S32 nSnsID) {
    return AX_TRUE;
}

AX_S32 CMD::AddArea(AX_U32 x, AX_U32 y, AX_U32 w, AX_U32 h,
                        AX_U32 mbW, AX_U32 mbH, AX_S32 nThrdY, AX_S32 nConfidenceY,
                        AX_U32 nWidth, AX_U32 nHeight, AX_S32 nSnsID) {
    std::lock_guard<std::mutex> lck(m_mutx);
    LOG_M_D(MD, "+++");
    AX_F32 fx = (nWidth > 0) ? (m_mdImgW * 1.0 / nWidth) : 1.0;
    AX_F32 fy = (nHeight > 0) ? (m_mdImgH * 1.0 / nHeight) : 1.0;

    LoadConfig(nSnsID);

    AX_IVES_MB_SIZE_T stMbSize = {mbW, mbH};

    /* Area should be divided by MB */
    AX_IVES_RECT_T stArea;
    stArea.u32X = align_down(fx * x, 2);
    stArea.u32Y = align_down(fy * y, 2);
    stArea.u32W = align_down(fx * w, stMbSize.u32W);
    stArea.u32H = align_down(fy * h, stMbSize.u32H);

    if (0 == stArea.u32W || 0 == stArea.u32H) {
        LOG_M_E(MD, "invalid area [%d, %d, %d, %d] in %dx%d, mb: %dx%d!", x, y, w, h, nWidth, nHeight, stMbSize.u32W, stMbSize.u32H);
        return -1;
    }

    AX_S32 nAreaId = -1;
    AX_U32 nCount = m_vecAreas.size();
    if (nCount > 0) {
        /* find the same area or freed area id */
        for (AX_U32 i = 0; i < nCount; ++i) {
            if (m_vecAreas[i]) {
                if (IsEqualArea(m_vecAreas[i]->stAttr.stArea, stArea)) {
                    LOG_M_W(MD, "area [%d, %d, %d, %d] already exists, id: %d", stArea.u32X, stArea.u32Y, stArea.u32W, stArea.u32H, i);
                    LOG_M_D(MD, "---");
                    return (AX_S32)i;
                }
            } else {
                if (nAreaId < 0) {
                    /* found a freed channel */
                    nAreaId = (AX_S32)i;
                }
            }
        }
    }

    MD_AREA_ATTR_T *pArea = (MD_AREA_ATTR_T *)malloc(sizeof(MD_AREA_ATTR_T));
    if (!pArea) {
        LOG_M_E(MD, "malloc md area memory fail, %s", strerror(errno));
        return -1;
    }

    memset(pArea, 0, sizeof(*pArea));
    pArea->stAttr.mdChn = -1; /* indicated area channel is not created */
    pArea->stAttr.enAlgMode = AX_MD_MODE_REF;
    pArea->stAttr.stMbSize = stMbSize;
    pArea->stAttr.stArea = stArea;
    pArea->stAttr.u8ThrY = nThrdY;
    pArea->nConfidence = GetConfidenceMBCount(nConfidenceY, stArea, stMbSize);

    if (m_bInited) {
        if (nAreaId < 0) {
            pArea->stAttr.mdChn = (AX_S32)nCount;
        } else {
            pArea->stAttr.mdChn = nAreaId;
        }
        if (-1 != nSnsID) {
            pArea->stAttr.mdChn = nSnsID;
        }
        AX_S32 ret = AX_IVES_MD_CreateChn(pArea->stAttr.mdChn, &pArea->stAttr);
        if (0 != ret) {
            LOG_M_E(MD, "Create MD channel %d fail, ret = 0x%x", pArea->stAttr.mdChn, ret);
            free(pArea);
            return -1;
        }
    }

    if (nAreaId < 0) {
        m_vecAreas.push_back(pArea);
        m_vecRslts.push_back(0);
        nAreaId = (AX_S32)nCount;
    } else {
        m_vecAreas[nAreaId] = pArea;
        m_vecRslts[nAreaId] = 0;
    }

    LOG_M_I(MD, "MD area [%d, %d, %d, %d] is added, id: %d", stArea.u32X, stArea.u32Y, stArea.u32W, stArea.u32H, nAreaId);
    LOG_M_D(MD, "---");
    return nAreaId;
}

AX_BOOL CMD::RemoveArea(AX_S32 nAreaId) {
    std::lock_guard<std::mutex> lck(m_mutx);
    LOG_M_D(MD, "+++");

    if (nAreaId < 0) {
        LOG_M_E(MD, "invalid area id %d", nAreaId);
        return AX_FALSE;
    }

    const AX_U32 nAreaCount = m_vecAreas.size();
    for (AX_U32 i = 0; i < nAreaCount; ++i) {
        if (!m_vecAreas[i]) {
            continue;
        }

        if (nAreaId == (AX_S32)i) {
            if (IS_MDCHN_CREATE(m_vecAreas[i]->stAttr.mdChn)) {
                AX_S32 ret = AX_IVES_MD_DestoryChn(m_vecAreas[i]->stAttr.mdChn);
                if (0 != ret) {
                    LOG_M_E(MD, "destory MD channel %d fail, ret = 0x%x", m_vecAreas[i]->stAttr.mdChn, ret);
                    return AX_FALSE;
                }
            }

            free(m_vecAreas[i]);
            m_vecAreas[i] = nullptr;
            m_vecRslts[i] = 0;

            LOG_M_D(MD, "---");
            return AX_TRUE;
        }
    }

    LOG_M_E(MD, "Area %d not exist!", nAreaId);
    return AX_FALSE;
}

const std::vector<AX_U8> &CMD::ProcessFrame(const AX_U32 nSnsID, const CAXFrame *pFrame) {
    std::lock_guard<std::mutex> lck(m_mutx);
    LOG_M_D(MD, "+++");

    AX_S32 ret;
    AX_IVES_IMAGE_T *pstImg = (AX_IVES_IMAGE_T *)&pFrame->stFrame.stVFrame;

    AX_U32 nMdCount = 0;
    AX_APP_ALGO_IVES_ITEM_T stMds[MAX_IVES_MD_RESULT_COUNT];
    memset(stMds, 0x00, sizeof(stMds));

    const AX_U32 nAreaCount = m_vecAreas.size();
    for (AX_U32 i = 0; i < nAreaCount; ++i) {
        MD_AREA_ATTR_T *m = m_vecAreas[i];
        if (m) {
            if (!IS_MDCHN_CREATE(m->stAttr.mdChn)) {
                m->stAttr.mdChn = i;
                ret = AX_IVES_MD_CreateChn(m->stAttr.mdChn, &m_vecAreas[i]->stAttr);
                if (0 != ret) {
                    LOG_M_E(MD, "Create MD channel %d fail, ret = 0x%x", m->stAttr.mdChn, ret);
                    m->stAttr.mdChn = -1; /* restore to uncreated */
                    continue;
                }
            }

            AX_MD_MB_THR_T stThrs{0, nullptr};
            AX_IVES_CCBLOB_T stBlob;
            ret = AX_IVES_MD_Process(m->stAttr.mdChn, pstImg, &stThrs, &stBlob);
            if (0 != ret) {
                LOG_M_E(MD, "frame id: %lld, MD process area %d fail, ret = 0x%x", pstImg->u64SeqNum, i, ret);
                continue;
            } else {
                AX_U32 nSumThrs = 0;
                for (AX_U32 k = 0; k < stThrs.u32Count; ++k) {
                    nSumThrs += stThrs.pMbThrs[k];
                }

                AX_U8 nLastRslt = m_vecRslts[i];

                /* marked 1 if the count of '1' mb > confidence */
                m_vecRslts[i] = (nSumThrs >= m->nConfidence) ? 1 : 0;

                if (1 == m_vecRslts[i] && nLastRslt != m_vecRslts[i]) {
                    stMds[nMdCount].eType = AX_APP_ALGO_IVES_MOTION;
                    stMds[nMdCount].u64FrameId = pFrame->stFrame.stVFrame.stVFrame.u64SeqNum;
                    stMds[nMdCount].fConfidence = (AX_F32)nSumThrs;
                    stMds[nMdCount].stBox.fX = (AX_F32)m->stAttr.stArea.u32X / pFrame->stFrame.stVFrame.stVFrame.u32Width;
                    stMds[nMdCount].stBox.fY = (AX_F32)m->stAttr.stArea.u32Y / pFrame->stFrame.stVFrame.stVFrame.u32Height;
                    stMds[nMdCount].stBox.fW = (AX_F32)m->stAttr.stArea.u32W / pFrame->stFrame.stVFrame.stVFrame.u32Width;
                    stMds[nMdCount].stBox.fH = (AX_F32)m->stAttr.stArea.u32H / pFrame->stFrame.stVFrame.stVFrame.u32Height;

                    stMds[nMdCount].stBox.nImgWidth = pFrame->stFrame.stVFrame.stVFrame.u32Width;
                    stMds[nMdCount].stBox.nImgHeight = pFrame->stFrame.stVFrame.stVFrame.u32Height;

                    stMds[nMdCount].stPanoramaImg.bExist = AX_FALSE;

                    nMdCount ++;
                }
            }
        }
    }

    IVES_RESULT_T ives;
    CIvesResult::GetInstance()->Get(nSnsID, ives);

    ives.nMdCount = nMdCount;
    memcpy(ives.stMds, stMds, sizeof(ives.stMds));

    CIvesResult::GetInstance()->Set(nSnsID, ives);

    LOG_M_D(MD, "---");
    return m_vecRslts;
}

AX_BOOL CMD::SetThresholdY(AX_S32 nSnsId, AX_S32 nAreaId, AX_U8 nThrd, AX_U8 nConfidence) {
    if (nAreaId < 0) {
        LOG_M_E(MD, "invalid area id %d", nAreaId);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mutx);
    LOG_M_D(MD, "+++, area: %d, Y thrd: %d, confidence: %d", nAreaId, nThrd, nConfidence);

    if (nAreaId >= (AX_S32)m_vecAreas.size() || !m_vecAreas[nAreaId]) {
        LOG_M_E(MD, "area %d not exist!", nAreaId);
        return AX_FALSE;
    } else {
        m_vecAreas[nAreaId]->stAttr.u8ThrY = nThrd;
        m_vecAreas[nAreaId]->nConfidence =
            GetConfidenceMBCount(nConfidence, m_vecAreas[nAreaId]->stAttr.stArea, m_vecAreas[nAreaId]->stAttr.stMbSize);
    }

    // update param
    {
        AX_APP_ALGO_MOTION_PARAM_T stParam = ALGO_MD_PARAM(nSnsId);
        if (nAreaId < stParam.nRegionSize) {
            stParam.stRegions[nAreaId].fThresholdY = (AX_F32)nThrd;
            stParam.stRegions[nAreaId].fConfidence = (AX_F32)nConfidence;
            SET_ALGO_MD_PARAM(nSnsId, stParam);
        }
    }

    LOG_M_D(MD, "---, area: %d, Y thrd: %d, confidence: %d", nAreaId, nThrd, nConfidence);
    return AX_TRUE;
}

AX_VOID CMD::GetDefaultThresholdY(AX_S32 nSnsID, AX_U8 &nThrd, AX_U8 &nConfidence) {
    nThrd = (AX_U8)ALGO_MD_PARAM(nSnsID).stRegions[0].fThresholdY;
    nConfidence = (AX_U8)ALGO_MD_PARAM(nSnsID).stRegions[0].fConfidence;
}
