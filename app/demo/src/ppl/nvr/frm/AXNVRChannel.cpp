/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "AXNVRChannel.h"
#include "ElapsedTimer.hpp"
#include "NVRConfigParser.h"

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif

#define ALIGN_UP_16(x) ALIGN_UP((x), 16)

#define AX_SHIFT_LEFT_ALIGN(a) (1 << (a))

/* VDEC stride align 256 */
#define VDEC_STRIDE_ALIGN AX_SHIFT_LEFT_ALIGN(8)
/* IVPS FBC stride align 128 */
#define IVPS_FBC_STRIDE_ALIGN AX_SHIFT_LEFT_ALIGN(7)
/* IVPS FBC input height align 4 */
#define IVPS_FBC_INPUT_HEIGHT_ALIGN AX_SHIFT_LEFT_ALIGN(2)

#define TAG "NVRCHN"

CAXNVRChannel *CAXNVRChannel::CreateInstance(const AX_NVR_CHN_ATTR_T &stAttr) {
    CAXNVRChannel *obj = new (std::nothrow) CAXNVRChannel;
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

AX_BOOL CAXNVRChannel::Init(const AX_NVR_CHN_ATTR_T &attr) {
    m_stAttr = attr;

    return AX_TRUE;
}

AX_VOID CAXNVRChannel::DeInit(AX_VOID) {
    m_stAttr.reset();
}

AX_BOOL CAXNVRChannel::TrySwitchMainSub1(AX_U32 nDate, AX_U32 nTime) {
    if (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD) {
        AX_NVR_CHN_IDX_TYPE enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
        if (AX_NVR_CHN_IDX_TYPE::MAIN == m_stAttr.enIndex) {
            enIndex = AX_NVR_CHN_IDX_TYPE::SUB1;
        } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == m_stAttr.enIndex) {
            enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
        }
        AXDS_STREAM_INFO_T stStream;
        if (!m_stAttr.pPlayback->GetStreamInfo(m_stAttr.nDevID, (AX_U8)enIndex, nDate, stStream)) {
            LOG_MM_W(TAG, "[%02d-%02d] Get record info failed.", m_stAttr.nDevID, m_stAttr.enIndex);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::SwitchMainSub1(AX_U32 nDate, AX_U32 nTime) {
    if (AX_NVR_CHN_IDX_TYPE::MAIN == m_stAttr.enIndex) {
        m_stAttr.enIndex = AX_NVR_CHN_IDX_TYPE::SUB1;
    } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == m_stAttr.enIndex) {
        m_stAttr.enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
    }
    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::StartRtsp(const std::string &strURL, AX_BOOL bRecord, AX_BOOL bForce, AX_S32 nCookie) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (m_enState != AX_NVR_CHN_STATE::IDLE) {
            bRet = AX_TRUE;
            break;
        }

        // if no disaplay and no record, stream not start
        m_stAttr.bRecord = bRecord;
        if (!m_stAttr.bRecord && m_stAttr.enView == AX_NVR_CHN_VIEW_TYPE::PREVIEW) {
            if (!bForce) {
                bRet = AX_TRUE;
                break;
            }
        }

        if (!this->init_rtsp(strURL, nCookie)) {
            break;
        }

        this->set_state(AX_NVR_CHN_STATE::TRANSFER);

        bRet = AX_TRUE;

    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::StartFile(AX_U32 nDate, AX_U32 nTime, AX_S32 nReverse /*=-1*/) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        AX_BOOL bReverse = AX_FALSE;
        if (-1 == nReverse) {
            bReverse = m_stAttr.bReverse;
        } else {
            bReverse = (AX_BOOL)nReverse;
        }
        if (m_enState != AX_NVR_CHN_STATE::IDLE) {
            LOG_MM_I(TAG, "[%02d:%02d][state:%d] state invalid.", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
            if (AX_NVR_CHN_SRC_TYPE::RECORD == m_stAttr.enStreamSrcType) {
                m_stAttr.bReverse = bReverse;
            }
            bRet = AX_TRUE;
            break;
        }

        LOG_MM_I(TAG, "[%02d:%02d]start datetime:%08d %06d", m_stAttr.nDevID, m_stAttr.enIndex, nDate, nTime);
        if (AX_NVR_CHN_SRC_TYPE::RECORD == m_stAttr.enStreamSrcType) {
            m_stAttr.nStartDate = nDate;
            m_stAttr.nStartTime = nTime;
            m_stAttr.bReverse = bReverse;

            AXDS_STREAM_INFO_T stStream;
            if (!m_stAttr.pPlayback->GetStreamInfo(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.nStartDate, stStream)) {
                LOG_MM_E(TAG, "[%02d-%02d] Get record info failed.", m_stAttr.nDevID, m_stAttr.enIndex);
                break;
            }
        } else if (AX_NVR_CHN_SRC_TYPE::FFMPEG == m_stAttr.enStreamSrcType) {
            if (!this->init_ffmpeg()) {
                break;
            }
        }

        this->set_state(AX_NVR_CHN_STATE::TRANSFER);
        bRet = AX_TRUE;
    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::StartDisp(VO_LAYER nVoLayer, VO_CHN nVoChannel, const AX_VO_RECT_T &stRect) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d][reverse:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState, m_stAttr.bReverse);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (AX_NVR_CHN_STATE::PAUSED == m_enState) {
            if (!this->pause_resume()) {
                break;
            }
            bRet = AX_TRUE;
            break;
        }

        if (AX_NVR_CHN_STATE::STARTED == m_enState && m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD) {
            m_stAttr.pPlayback->ChangeDirection(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.bReverse);
            bRet = AX_TRUE;
            break;
        }

        if (AX_NVR_CHN_STATE::TRANSFER != m_enState) {
            bRet = AX_TRUE;
            break;
        }

        m_stAttr.nVoLayer = nVoLayer;
        m_stAttr.nVoChannel = nVoChannel;
        this->set_state(AX_NVR_CHN_STATE::STARTED);

        // 0. get stream resulotion
        AX_PAYLOAD_TYPE_E enPayload = AX_PAYLOAD_TYPE_E::PT_BUTT;
        AX_F32 fFps = 0;
        AX_U32 nGop = 0;
        AX_U32 nWidth = 0;
        AX_U32 nHeight = 0;
        if (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD) {
            AXDS_STREAM_INFO_T stStream;
            if (!m_stAttr.pPlayback->GetStreamInfo(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.nStartDate, stStream)) {
                LOG_MM_E(TAG, "[%02d-%02d] Get record info failed.", m_stAttr.nDevID, m_stAttr.enIndex);
                break;
            }

            fFps = stStream.uFrameRate;
            nGop = stStream.uGop;
            enPayload = stStream.uEncodeType;
            nWidth = stStream.uWidth;
            nHeight = stStream.uHeight;
        } else {
            const STREAM_INFO_T &streamInfo = this->GetStream().GetStreamInfo();
            fFps = streamInfo.stVideo.nFps;
            nGop = fFps;
            enPayload = streamInfo.stVideo.enPayload;
            nWidth = streamInfo.stVideo.nWidth;
            nHeight = streamInfo.stVideo.nHeight;
        }

        if (0 == nWidth || 0 == nHeight) {
            LOG_MM_E(TAG, "Channel resolution(%d x %d) error, start display failed.", nWidth, nHeight);
            this->set_state(AX_NVR_CHN_STATE::TRANSFER);
            break;
        }

        LOG_MM_I(TAG, "[%02d:%02d] Stream Resulotion %f %d %dx%d.", m_stAttr.nDevID, m_stAttr.enIndex, fFps, enPayload, nWidth, nHeight);

        // 1. VDEC
        // init and start
        AX_VDEC_GRP _vdGrp = nVoChannel;
        if (AX_NVR_CHN_VIEW_TYPE::PATROL == m_stAttr.enView) {
            _vdGrp += MAX_PRIMARY_DISP_CHN_COUNT;
        }
        if (!this->init_vdec(enPayload, nWidth, nHeight, {stRect.u32Width, stRect.u32Height}, _vdGrp)) {
            LOG_MM_E(TAG, "[%02d] Create and Start Decoder failed.", m_stAttr.nDevID);
            break;
        }

        // create VDEC link info: link to IVPS
        m_stModeInfoVdec = {AX_ID_VDEC, (AX_S32)(m_objVdec.GetGrpId()), m_stAttr.nLinkVdecIvpsChn};

        if (m_stAttr.bReverse && m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD && !m_stAttr.pPlayback->GetAttr().bOnlyIFrameOnReverse) {
            InitReversePlaybackModules(nWidth, nHeight, fFps, nGop, enPayload);
        }

        // 2. IVPS
        // init and start
        if (!this->init_ivps(nWidth, nHeight, stRect, (AX_IVPS_GRP)m_objVdec.GetGrpId())) {
            LOG_MM_E(TAG, "[%02d] Create and Start IVPS failed.", m_stAttr.nDevID);
            break;
        }

        // create IVPS link info: link to VO
        m_stModeInfoIvps = {AX_ID_IVPS, (AX_S32)(m_objIvps.GetGrpId()), m_stAttr.nLinkIvpsVoooChn};

        // 3. VO
        // set vo channel fps
        if (!this->set_fps(m_stAttr.nVoLayer, m_stAttr.nVoChannel, fFps)) {
            break;
        }
        // create VO link info
        m_stModeInfoVo = {AX_ID_VO, (AX_S32)m_stAttr.nVoLayer, (AX_S32)m_stAttr.nVoChannel};

        // 4. DETECT
        if (m_stAttr.pDetector && m_stAttr.pDetectObs && AX_NVR_DETECT_SRC_TYPE::NONE != m_stAttr.enDetectSrcType) {
            if (AX_NVR_DETECT_SRC_TYPE::IVPS == m_stAttr.enDetectSrcType) {
                m_objIvps.RegisterObserver(m_stAttr.nRegiIvpsDeteChn, m_stAttr.pDetectObs);

                if (nullptr == m_stAttr.pDisplay->GetVoRegionObs()) {
                    if (!this->init_region(m_objIvps.GetGrpId(), nWidth, nHeight, stRect)) {
                        LOG_MM_E(TAG, "[%02d] Create and Start Region failed.", m_stAttr.nDevID);
                        break;
                    }
                    m_objRgnObs.SetSink(&m_objRegion);
                    m_stAttr.pDetector->RegisterObserver(m_objIvps.GetGrpId(), &m_objRgnObs);
                } else {
                    CVOLayerRegionObserver *pVoObs = const_cast<CVOLayerRegionObserver *>(m_stAttr.pDisplay->GetVoRegionObs());
                    if (nullptr != pVoObs) {
                        m_stAttr.pDetector->RegisterObserver(m_objIvps.GetGrpId(), pVoObs);
                        pVoObs->RegisterSrcGrp(m_objIvps.GetGrpId(), nVoChannel);
                    }
                }

                LOG_MM_I(TAG, "Register vo layer %d chn %d to detect %d", nVoLayer, nVoChannel, m_objIvps.GetGrpId());
            } else if (AX_NVR_DETECT_SRC_TYPE::VDEC == m_stAttr.enDetectSrcType) {
                m_objVdec.RegisterObserver(m_stAttr.nRegiVdecDeteChn, m_stAttr.pDetectObs);

                if (nullptr == m_stAttr.pDisplay->GetVoRegionObs()) {
                    if (!this->init_region(m_objVdec.GetGrpId(), nWidth, nHeight, stRect)) {
                        LOG_MM_E(TAG, "[%02d] Create and Start Region failed.", m_stAttr.nDevID);
                        break;
                    }
                    m_objRgnObs.SetSink(&m_objRegion);
                    m_stAttr.pDetector->RegisterObserver(m_objVdec.GetGrpId(), &m_objRgnObs);
                } else {
                    CVOLayerRegionObserver *pVoObs = const_cast<CVOLayerRegionObserver *>(m_stAttr.pDisplay->GetVoRegionObs());
                    if (nullptr != pVoObs) {
                        m_stAttr.pDetector->RegisterObserver(m_objVdec.GetGrpId(), pVoObs);
                        pVoObs->RegisterSrcGrp(m_objVdec.GetGrpId(), nVoChannel);
                    }
                }

                LOG_MM_I(TAG, "Register vo layer %d chn %d to detect %d", nVoLayer, nVoChannel, m_objVdec.GetGrpId());
            }
        }

        // 5. link or register
        // STREAM register VDEC
        if (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD) {
            m_stAttr.pPlayback->RegisterObserver(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, &m_objVdec);
        } else {
            this->GetStream().RegisterObserver(&m_objVdec);
        }

        if (m_stAttr.bReverse && m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD && !m_stAttr.pPlayback->GetAttr().bOnlyIFrameOnReverse) {
            StartReversePlaybackLinkages();

            // IVPS_1 link VO
            AX_S32 ret = AX_SYS_Link(&m_stModeInfoIvps, &m_stModeInfoVo);
            if (0 != ret) {
                THROW_AX_EXCEPTION("link IVPS %d %d to DISP %d %d fail, ret = 0x%x", m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId,
                        m_stModeInfoVo.s32GrpId, m_stModeInfoVo.s32ChnId, ret);
            } else {
                LOG_MM_I(TAG, "link IVPS %d %d to DISP %d %d OK", m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId,
                        m_stModeInfoVo.s32GrpId, m_stModeInfoVo.s32ChnId);
            }
        } else {
            LOG_MM_I(TAG, "[%02d]Link vdec=%d ivps=%d vo=%d", m_stAttr.nDevID, m_objVdec.GetGrpId(), m_objIvps.GetGrpId(), m_stAttr.nVoChannel);

            AX_S32 ret = AX_SYS_Link(&m_stModeInfoVdec, &m_stModeInfoIvps);
            if (0 != ret) {
                THROW_AX_EXCEPTION("link VDEC %d %d to IVPS %d %d fail, ret = 0x%x", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                        m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId, ret);
            }

            // IVPS link VO
            ret = AX_SYS_Link(&m_stModeInfoIvps, &m_stModeInfoVo);
            if (0 != ret) {
                THROW_AX_EXCEPTION("link IVPS %d %d to DISP %d %d fail, ret = 0x%x", m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId,
                        m_stModeInfoVo.s32GrpId, m_stModeInfoVo.s32ChnId, ret);
            }
        }

        // 6. file stream start
        if (AX_NVR_CHN_SRC_TYPE::RECORD == m_stAttr.enStreamSrcType) {
            if (!m_stAttr.pPlayback->StartPlay(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.nStartDate, m_stAttr.nStartTime,
                                               m_stAttr.bReverse)) {
                LOG_MM_E(TAG, "[%02d:%02d] start record failed.", m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex);
                break;
            }
        } else {
            if (!m_stAttr.bRecord || m_stAttr.enView == AX_NVR_CHN_VIEW_TYPE::PATROL) {
                /* If record enabled, rtsp had already started within init_rtsp(). */
                if (!GetStream().Start()) {
                    LOG_MM_E(TAG, "[%02d:%02d] start stream(type: %d) failed.", m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex,
                            m_stAttr.enStreamSrcType);
                    GetStream().DeInit();
                    break;
                }
            }
        }

        bRet = AX_TRUE;

    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::StopRtsp(AX_BOOL bForce) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (m_enState != AX_NVR_CHN_STATE::TRANSFER) {
            if (AX_NVR_CHN_STATE::IDLE != m_enState) {
                LOG_MM_E(TAG, "[%02d][state:%d] state invalid.", m_stAttr.nDevID, m_enState);
            }

            bRet = AX_TRUE;
            break;
        }

        if (!m_stAttr.bRecord || bForce) {
            if (!m_objRtspStream.Stop()) {
                LOG_MM_E(TAG, "[%02d] Stream stop failed.", m_stAttr.nDevID);
                break;
            }

            if (bForce && m_stAttr.bRecord && m_stAttr.enView == AX_NVR_CHN_VIEW_TYPE::PREVIEW) {
                if (!m_stAttr.pRecord->Stop(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex)) {
                    LOG_MM_E(TAG, "[%02d:%02d] Stream Record stop failed.", m_stAttr.nDevID, m_stAttr.enIndex);
                }
                m_objRtspStream.UnRegisterObserver(&m_objRecordObs);
            }

            if (!m_objRtspStream.DeInit()) {
                LOG_MM_E(TAG, "[%02d] Stream deinit failed.", m_stAttr.nDevID);
                break;
            }
            this->set_state(AX_NVR_CHN_STATE::IDLE);
        } else {
            LOG_MM_I(TAG, "[%02d:%02d] rtsp record actived. <%d>", m_stAttr.nDevID, m_stAttr.enIndex, m_stAttr.bRecord);
        }

        bRet = AX_TRUE;

    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::StopFile(AX_VOID) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);
    do {
        if (AX_NVR_CHN_STATE::TRANSFER != m_enState) {
            LOG_MM_I(TAG, "[%02d:%02d:][state:%d] state invalid.", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
            bRet = AX_TRUE;
            break;
        }

        if (AX_NVR_CHN_SRC_TYPE::RECORD == m_stAttr.enStreamSrcType) {
            m_stAttr.nStartDate = 0;
            m_stAttr.nStartTime = 0;
        } else if (AX_NVR_CHN_SRC_TYPE::FFMPEG == m_stAttr.enStreamSrcType) {
            if (!m_objFFMpegStream.DeInit()) {
                LOG_MM_E(TAG, "[%02d] Stream deinit failed.", m_stAttr.nDevID);
                break;
            }
        }

        this->set_state(AX_NVR_CHN_STATE::IDLE);
        bRet = AX_TRUE;

    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::StopDisp(AX_VOID) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d][reverse:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState, m_stAttr.bReverse);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (AX_NVR_CHN_STATE::PAUSED == m_enState) {
            if (!this->pause_resume()) {
                break;
            }
            bRet = AX_TRUE;
        }

        if (m_enState != AX_NVR_CHN_STATE::STARTED) {
            LOG_MM_I(TAG, "[%02d][state:%d] state invalid.", m_stAttr.nDevID, m_enState);
            bRet = AX_TRUE;
            break;
        }

        // unlink or unregister
        if (m_stAttr.pDetector && m_stAttr.pDetectObs && AX_NVR_DETECT_SRC_TYPE::NONE != m_stAttr.enDetectSrcType) {
            if (AX_NVR_DETECT_SRC_TYPE::IVPS == m_stAttr.enDetectSrcType) {
                m_objIvps.UnRegisterObserver(m_stAttr.nRegiIvpsDeteChn, m_stAttr.pDetectObs);
            } else if (AX_NVR_DETECT_SRC_TYPE::VDEC == m_stAttr.enDetectSrcType) {
                m_objVdec.UnRegisterObserver(m_stAttr.nRegiVdecDeteChn, m_stAttr.pDetectObs);
            }

            if (nullptr == m_stAttr.pDisplay->GetVoRegionObs()) {
                m_stAttr.pDetector->UnRegisterObserver(m_objIvps.GetGrpId(), &m_objRgnObs);
                m_objRegion.Stop();
                m_objRegion.DeInit();
            } else {
                CVOLayerRegionObserver *pVoObs = const_cast<CVOLayerRegionObserver *>(m_stAttr.pDisplay->GetVoRegionObs());
                if (nullptr != pVoObs) {
                    if (AX_NVR_DETECT_SRC_TYPE::IVPS == m_stAttr.enDetectSrcType) {
                        m_stAttr.pDetector->UnRegisterObserver(m_objIvps.GetGrpId(), pVoObs);
                        pVoObs->UnRegisterSrcGrp(m_objIvps.GetGrpId());
                    } else if (AX_NVR_DETECT_SRC_TYPE::VDEC == m_stAttr.enDetectSrcType) {
                        m_stAttr.pDetector->UnRegisterObserver(m_objVdec.GetGrpId(), pVoObs);
                        pVoObs->UnRegisterSrcGrp(m_objVdec.GetGrpId());
                    }
                }
            }
        }

        if (AX_NVR_CHN_SRC_TYPE::RTSP == m_stAttr.enStreamSrcType) {
            if (!this->GetStream().UnRegisterObserver(&m_objVdec)) {
                LOG_MM_E(TAG, "[%02d] Unlink Stream between VDEC failed.", m_stAttr.nDevID);
                break;
            }
        }

        if (m_stAttr.bReverse && m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD && !m_stAttr.pPlayback->GetAttr().bOnlyIFrameOnReverse) {
            // unlink vdec -> ivps -> venc ->...-> vdec -> ivps -> vo
            StopReversePlaybackLinkages();

            AX_S32 ret = AX_SYS_UnLink(&m_stModeInfoIvps, &m_stModeInfoVo);
            if (0 != ret) {
                THROW_AX_EXCEPTION("unlink IVPS_1 %d %d to DISP %d %d fail, ret = 0x%x", m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId,
                    m_stModeInfoVo.s32GrpId, m_stModeInfoVo.s32ChnId, ret);
            } else {
                LOG_MM_I(TAG, "unlink IVPS_1 %d %d to DISP %d %d OK", m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId,
                    m_stModeInfoVo.s32GrpId, m_stModeInfoVo.s32ChnId);
            }
        } else {
            LOG_MM_I(TAG, "[%02d] UNLink vdec=%d ivps=%d vo=%d", m_stAttr.nDevID, m_objVdec.GetGrpId(), m_objIvps.GetGrpId(), m_stAttr.nVoChannel);

            // vdec unlink ivps
            AX_S32 ret = AX_SYS_UnLink(&m_stModeInfoVdec, &m_stModeInfoIvps);
            if (0 != ret) {
                THROW_AX_EXCEPTION("unlink VDEC %d %d to IVPS %d %d fail, ret = 0x%x", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                        m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId, ret);
            } else {
                LOG_MM_I(TAG, "unlink VDEC %d %d to IVPS %d %d", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                    m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId);
            }

            // ivps unlink vo
            ret = AX_SYS_UnLink(&m_stModeInfoIvps, &m_stModeInfoVo);
            if (0 != ret) {
                THROW_AX_EXCEPTION("unlink IVPS %d %d to DISP %d %d fail, ret = 0x%x", m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId,
                        m_stModeInfoVo.s32GrpId, m_stModeInfoVo.s32ChnId, ret);
            } else {
                LOG_MM_I(TAG, "unlink IVPS %d %d to DISP %d %d", m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId,
                    m_stModeInfoVo.s32GrpId, m_stModeInfoVo.s32ChnId);
            }
        }

        // clean vo buf
        this->clean(m_stAttr.nVoLayer, m_stAttr.nVoChannel);

        // destroy inner modules on REVERSE playback
        if (m_stAttr.bReverse && m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD && !m_stAttr.pPlayback->GetAttr().bOnlyIFrameOnReverse) {
            DeInitReversePlaybackModules();
        }

        // destroy VDEC
        m_objVdec.Stop();
        m_objVdec.DeInit();

        // destroy IVPS
        m_objIvps.Stop();
        m_objIvps.DeInit();

        // unregister stream observer and stop stream
        if (AX_NVR_CHN_SRC_TYPE::FFMPEG == m_stAttr.enStreamSrcType) {
            GetStream().UnRegisterObserver(&m_objVdec);

            if (!m_objFFMpegStream.Stop()) {
                LOG_MM_E(TAG, "[%02d]FFMPeg Stream stop failed.", m_stAttr.nDevID);
                m_objFFMpegStream.DeInit();
                break;
            }
        } else if (AX_NVR_CHN_SRC_TYPE::RECORD == m_stAttr.enStreamSrcType) {
            if (!m_stAttr.pPlayback->UnRegisterObserver(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, &m_objVdec)) {
                LOG_MM_E(TAG, "[%02d] Unlink Record Stream between VDEC failed.", m_stAttr.nDevID);
                break;
            }

            if (!m_stAttr.pPlayback->StopPlay(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex)) {
                LOG_MM_E(TAG, "[%02d]Record Stream stop failed.", m_stAttr.nDevID);
                break;
            }
        }

        this->set_state(AX_NVR_CHN_STATE::TRANSFER);

        bRet = AX_TRUE;

    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::InitReversePlaybackModules(AX_U32 nWidth, AX_U32 nHeight, AX_F32 fFps, AX_U32 nGop, AX_PAYLOAD_TYPE_E enPayload) {
    LOG_MM_I(TAG, "+++");

    AX_S32 nGrp = m_objVdec.GetGrpId() + MAX_DEVICE_PLAYBACK_COUNT;
    // init and start
    if (!this->init_ivps2(nWidth, nHeight, (AX_IVPS_GRP)nGrp)) {
        LOG_MM_E(TAG, "[%02d] Create and Start IVPS_2 failed.", m_stAttr.nDevID);
        return AX_FALSE;
    }

    // create IVPS link info: link to VENC
    m_stModeInfoIvps2 = {AX_ID_IVPS, (AX_S32)(m_objIvps2.GetGrpId()), 0};

    // init and start
    if (!this->init_venc(enPayload, nWidth, nHeight, (AX_U32)fFps, (VENC_GRP)nGrp)) {
        LOG_MM_E(TAG, "[%02d] Create and Start VENC failed.", m_stAttr.nDevID);
        return AX_FALSE;
    }

    if (!this->init_transfer((AX_U32)fFps, nGop, nGrp)) {
        LOG_MM_E(TAG, "[%02d] Create and Start TRANSFER failed.", m_stAttr.nDevID);
        return AX_FALSE;
    } else {
        m_objVenc.RegisterObserver(&m_objStreamTrans);
    }

    m_stModeInfoVenc = {AX_ID_VENC, 0, nGrp};

    if (!this->init_vdec2(enPayload, nWidth, nHeight, {0, 0}, nGrp)) {
        LOG_MM_E(TAG, "[%02d] Create and Start Decoder failed.", m_stAttr.nDevID);
        return AX_FALSE;
    } else {
        m_objStreamTrans.RegisterObserver(&m_objVdec2);
    }

    m_stModeInfoVdec2 = {AX_ID_VDEC, (AX_S32)(m_objVdec2.GetGrpId()), 0};

    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::DeInitReversePlaybackModules() {
    LOG_MM_I(TAG, "+++");

    // destroy vdec2
    m_objVdec2.Stop();
    m_objVdec2.DeInit();

    // destroy venctop
    m_objVenc.Stop();
    m_objVenc.DeInit();

    // destroy ivps2
    m_objIvps2.Stop();
    m_objIvps2.DeInit();

    // destroy stream transfer
    m_objStreamTrans.Stop();
    m_objStreamTrans.DeInit();

    LOG_MM_I(TAG, "---");

    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::StartReversePlaybackLinkages() {
    LOG_MM_I(TAG, "+++");

    // VDEC link IVPS_2
    AX_S32 ret = AX_SYS_Link(&m_stModeInfoVdec, &m_stModeInfoIvps2);
    if (0 != ret) {
        THROW_AX_EXCEPTION("link VDEC %d %d to IVPS_2 %d %d fail, ret = 0x%x", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                m_stModeInfoIvps2.s32GrpId, m_stModeInfoIvps2.s32ChnId, ret);
    } else {
        LOG_MM_W(TAG, "link VDEC %d %d to IVPS_2 %d %d OK", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                m_stModeInfoIvps2.s32GrpId, m_stModeInfoIvps2.s32ChnId);
    }

    // IVPS_2 link VENC
    ret = AX_SYS_Link(&m_stModeInfoIvps2, &m_stModeInfoVenc);
    if (0 != ret) {
        THROW_AX_EXCEPTION("link IVPS2 %d %d to VENC %d %d fail, ret = 0x%x", m_stModeInfoIvps2.s32GrpId, m_stModeInfoIvps2.s32ChnId,
                m_stModeInfoVenc.s32GrpId, m_stModeInfoVenc.s32ChnId, ret);
    } else {
        LOG_MM_W(TAG, "link IVPS2 %d %d to VENC %d %d OK", m_stModeInfoIvps2.s32GrpId, m_stModeInfoIvps2.s32ChnId,
                m_stModeInfoVenc.s32GrpId, m_stModeInfoVenc.s32ChnId);
    }

    // VDEC_2 link IVPS_1
    ret = AX_SYS_Link(&m_stModeInfoVdec2, &m_stModeInfoIvps);
    if (0 != ret) {
        THROW_AX_EXCEPTION("link VDEC_2 %d %d to IVPS_1 %d %d fail, ret = 0x%x", m_stModeInfoVdec2.s32GrpId, m_stModeInfoVdec2.s32ChnId,
                m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId, ret);
    } else {
        LOG_MM_W(TAG, "link VDEC_2 %d %d to IVPS_1 %d %d OK", m_stModeInfoVdec2.s32GrpId, m_stModeInfoVdec2.s32ChnId,
                m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId);
    }

    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::StopReversePlaybackLinkages() {
    LOG_MM_I(TAG, "+++");

    // VDEC unlink IVPS_2
    AX_S32 ret = AX_SYS_UnLink(&m_stModeInfoVdec, &m_stModeInfoIvps2);
    if (0 != ret) {
        THROW_AX_EXCEPTION("unlink VDEC %d %d to IVPS_2 %d %d fail, ret = 0x%x", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                m_stModeInfoIvps2.s32GrpId, m_stModeInfoIvps2.s32ChnId, ret);
    } else {
        LOG_MM_W(TAG, "unlink VDEC %d %d to IVPS_2 %d %d OK", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                m_stModeInfoIvps2.s32GrpId, m_stModeInfoIvps2.s32ChnId);
    }

    // IVPS_2 unlink VENC
    ret = AX_SYS_UnLink(&m_stModeInfoIvps2, &m_stModeInfoVenc);
    if (0 != ret) {
        THROW_AX_EXCEPTION("unlink IVPS2 %d %d to VENC %d %d fail, ret = 0x%x", m_stModeInfoIvps2.s32GrpId, m_stModeInfoIvps2.s32ChnId,
                m_stModeInfoVenc.s32GrpId, m_stModeInfoVenc.s32ChnId, ret);
    } else {
        LOG_MM_W(TAG, "unlink IVPS2 %d %d to VENC %d %d OK", m_stModeInfoIvps2.s32GrpId, m_stModeInfoIvps2.s32ChnId,
                m_stModeInfoVenc.s32GrpId, m_stModeInfoVenc.s32ChnId);
    }

    // VDEC_2 unlink IVPS_1
    ret = AX_SYS_UnLink(&m_stModeInfoVdec2, &m_stModeInfoIvps);
    if (0 != ret) {
        THROW_AX_EXCEPTION("unlink VDEC_2 %d %d to IVPS_1 %d %d fail, ret = 0x%x", m_stModeInfoVdec2.s32GrpId, m_stModeInfoVdec2.s32ChnId,
                m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId, ret);
    } else {
        LOG_MM_W(TAG, "unlink VDEC_2 %d %d to IVPS_1 %d %d OK", m_stModeInfoVdec2.s32GrpId, m_stModeInfoVdec2.s32ChnId,
                m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId);
    }

    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::UpdateRect(const AX_VO_RECT_T &stRect) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_D(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (m_enState != AX_NVR_CHN_STATE::STARTED) {
            LOG_MM_D(TAG, "[%02d][state:%d] state invalid.", m_stAttr.nDevID, m_enState);
            break;
        }
        AX_F32 fFps = 0;
        // AX_U32 u32Width = 0;
        // AX_U32 u32Height = 0;
        if (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD) {
            AXDS_STREAM_INFO_T stStream;
            m_stAttr.pPlayback->GetStreamInfo(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.nStartDate, stStream);
            fFps = stStream.uFrameRate;
        } else {
            const STREAM_INFO_T &streamInfo = this->GetStream().GetStreamInfo();
            fFps = streamInfo.stVideo.nFps;
            // u32Width = streamInfo.stVideo.nFps;
            // u32Height = streamInfo.stVideo.nFps;
        }

        // set vo channel fps
        if (!this->set_fps(m_stAttr.nVoLayer, m_stAttr.nVoChannel, fFps)) {
            break;
        }

        IVPS_CHN_ATTR_T stChnAttr = m_objIvps.GetAttr().stChnAttr[m_stAttr.nLinkIvpsVoooChn];
        LOG_MM_D(TAG, "[%02d]Update Rect %dx%d to %dx%d", m_stAttr.nDevID, stChnAttr.nWidth, stChnAttr.nHeight, stRect.u32Width,
                stRect.u32Height);
        if (stChnAttr.nWidth == stRect.u32Width && stChnAttr.nHeight == stRect.u32Height) {
            bRet = AX_TRUE;
            break;
        }

        stChnAttr.nWidth = stRect.u32Width;
        stChnAttr.nHeight = stRect.u32Height;
        stChnAttr.nStride = ALIGN_UP_16(stChnAttr.nWidth);
        if (!m_objIvps.UpdateChnAttr(m_stAttr.nLinkIvpsVoooChn, stChnAttr)) {
            LOG_MM_E(TAG, "IVPS Update Channel<%d> attr failed", m_stAttr.nLinkIvpsVoooChn);
            break;
        }

        bRet = AX_TRUE;

    } while (0);

    LOG_MM_D(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::UpdateRPatrolRect(const AX_VO_RECT_T &stRect) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (m_enState != AX_NVR_CHN_STATE::STARTED && m_enState != AX_NVR_CHN_STATE::PAUSED) {
            LOG_MM_W(TAG, "[%02d][state:%d] state invalid.", m_stAttr.nDevID, m_enState);
            break;
        }

        AX_U32 nSrcWidth = m_objVdec.GetAttr().nWidth;
        AX_U32 nSrcHeight = m_objVdec.GetAttr().nHeight;

        AX_VDEC_CHN_ATTR_T tVdecChnAttr = m_objVdec.GetAttr().stChnAttr[m_stAttr.nLinkVdecIvpsChn].stAttr;
        AX_U32 nDstWidth = tVdecChnAttr.u32PicWidth;
        AX_U32 nDstHeight = tVdecChnAttr.u32PicHeight;

        if (nSrcWidth < stRect.u32Width || nSrcHeight < stRect.u32Height) { /* Keep vdec out resolution and rely on ivps to do resize */
            /* Update vdec output resolution as input */
            tVdecChnAttr.u32PicWidth = nSrcWidth;
            tVdecChnAttr.u32PicHeight = ALIGN_UP(nSrcHeight, IVPS_FBC_INPUT_HEIGHT_ALIGN);
            tVdecChnAttr.u32FrameStride = ALIGN_UP(tVdecChnAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
            m_objVdec.SetChnAttr(m_stAttr.nLinkVdecIvpsChn, tVdecChnAttr);

            /* Update ivps output resolution */
            IVPS_CHN_ATTR_T stChnAttr = m_objIvps.GetAttr().stChnAttr[m_stAttr.nLinkIvpsVoooChn];
            LOG_MM_I(TAG, "[%02d]Update Rect %dx%d to %dx%d", m_stAttr.nDevID, stChnAttr.nWidth, stChnAttr.nHeight, stRect.u32Width,
                    stRect.u32Height);
            if (stChnAttr.nWidth == stRect.u32Width && stChnAttr.nHeight == stRect.u32Height) {
                bRet = AX_TRUE;
                break;
            }

            stChnAttr.nWidth = stRect.u32Width;
            stChnAttr.nHeight = stRect.u32Height;
            stChnAttr.nStride = ALIGN_UP_16(stChnAttr.nWidth);
            if (!m_objIvps.UpdateChnAttr(m_stAttr.nLinkIvpsVoooChn, stChnAttr)) {
                LOG_MM_E(TAG, "IVPS Update Channel<%d> attr failed", m_stAttr.nLinkIvpsVoooChn);
                break;
            }

            bRet = AX_TRUE;
        } else { /* Rely on vdec to do resize and ivps bypass */
            // Stop rect drawing
            if (m_stAttr.pDetector && m_stAttr.pDetectObs && AX_NVR_DETECT_SRC_TYPE::NONE != m_stAttr.enDetectSrcType) {
                if (nullptr == m_stAttr.pDisplay->GetVoRegionObs()) {
                    m_stAttr.pDetector->UnRegisterObserver(m_objIvps.GetGrpId(), &m_objRgnObs);
                } else {
                    CVOLayerRegionObserver *pVoObs = const_cast<CVOLayerRegionObserver *>(m_stAttr.pDisplay->GetVoRegionObs());
                    if (nullptr != pVoObs) {
                        if (AX_NVR_DETECT_SRC_TYPE::IVPS == m_stAttr.enDetectSrcType) {
                            pVoObs->UnRegisterSrcGrp(m_objIvps.GetGrpId());
                        } else if (AX_NVR_DETECT_SRC_TYPE::VDEC == m_stAttr.enDetectSrcType) {
                            pVoObs->UnRegisterSrcGrp(m_objVdec.GetGrpId());
                        }
                    }
                }
            }

            /* Update vdec output resolution according to display */
            tVdecChnAttr.u32PicWidth = stRect.u32Width;
            tVdecChnAttr.u32PicHeight = ALIGN_UP(stRect.u32Height, IVPS_FBC_INPUT_HEIGHT_ALIGN);
            tVdecChnAttr.u32FrameStride = ALIGN_UP(tVdecChnAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
            m_objVdec.SetChnAttr(m_stAttr.nLinkVdecIvpsChn, tVdecChnAttr);

            /* Update ivps output resolution, usually same as input except when input resolution can not match the display */
            IVPS_CHN_ATTR_T tIvpsChnAttr = m_objIvps.GetAttr().stChnAttr[m_stAttr.nLinkIvpsVoooChn];
            LOG_MM_I(TAG, "[%02d]Update Rect %dx%d to %dx%d", m_stAttr.nDevID, tIvpsChnAttr.nWidth, tIvpsChnAttr.nHeight, stRect.u32Width,
                    stRect.u32Height);

            tIvpsChnAttr.nWidth = stRect.u32Width;
            tIvpsChnAttr.nHeight = stRect.u32Height;
            tIvpsChnAttr.nStride = ALIGN_UP_16(tIvpsChnAttr.nWidth);
            if (!m_objIvps.UpdateChnAttr(m_stAttr.nLinkIvpsVoooChn, tIvpsChnAttr)) {
                LOG_MM_E(TAG, "IVPS Update Channel<%d> attr failed", m_stAttr.nLinkIvpsVoooChn);
                break;
            }

            // Restart rect drawing
            if (m_stAttr.pDetector && m_stAttr.pDetectObs && AX_NVR_DETECT_SRC_TYPE::NONE != m_stAttr.enDetectSrcType) {
                if (AX_NVR_DETECT_SRC_TYPE::IVPS == m_stAttr.enDetectSrcType) {
                    if (nullptr == m_stAttr.pDisplay->GetVoRegionObs()) {
                        m_stAttr.pDetector->RegisterObserver(m_objIvps.GetGrpId(), &m_objRgnObs);
                    } else {
                        CVOLayerRegionObserver *pVoObs = const_cast<CVOLayerRegionObserver *>(m_stAttr.pDisplay->GetVoRegionObs());
                        if (nullptr != pVoObs) {
                            pVoObs->RegisterSrcGrp(m_objIvps.GetGrpId(), m_stAttr.nDevID);
                        }
                    }
                } else if (AX_NVR_DETECT_SRC_TYPE::VDEC == m_stAttr.enDetectSrcType) {
                    if (nullptr == m_stAttr.pDisplay->GetVoRegionObs()) {
                        m_stAttr.pDetector->RegisterObserver(m_objVdec.GetGrpId(), &m_objRgnObs);
                    } else {
                        CVOLayerRegionObserver *pVoObs = const_cast<CVOLayerRegionObserver *>(m_stAttr.pDisplay->GetVoRegionObs());
                        if (nullptr != pVoObs) {
                            pVoObs->RegisterSrcGrp(m_objVdec.GetGrpId(), m_stAttr.nDevID);
                        }
                    }
                }
            }

            bRet = AX_TRUE;
        }
    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::PauseResume(AX_BOOL bForceResume) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);
    bRet = this->pause_resume(bForceResume);
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::Step(AX_BOOL bReverse) {
    AX_BOOL bRet = AX_FALSE;
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);
    do {
        if (AX_NVR_CHN_STATE::PAUSED != m_enState) {
            LOG_MM_W(TAG, "Not in PAUSED state, ignore step operation.");
            bRet = AX_TRUE;
            break;
        }

        if (bReverse != m_stAttr.bReverse) {
            AX_PAYLOAD_TYPE_E enPayload = AX_PAYLOAD_TYPE_E::PT_BUTT;
            AX_F32 fFps = 0;
            AX_U32 nGop = 0;
            AX_U32 nWidth = 0;
            AX_U32 nHeight = 0;

            AXDS_STREAM_INFO_T stStream;
            if (!m_stAttr.pPlayback->GetStreamInfo(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.nStartDate, stStream)) {
                LOG_MM_E(TAG, "[%02d-%02d] Get record info failed.", m_stAttr.nDevID, m_stAttr.enIndex);
                break;
            }

            fFps = stStream.uFrameRate;
            nGop = stStream.uGop;
            enPayload = stStream.uEncodeType;
            nWidth = stStream.uWidth;
            nHeight = stStream.uHeight;

            if (bReverse) { /* Forward => Reverse */
                LOG_MM_W(TAG, "Forward => Reverse START +++");
                /* <------------------ STOP ------------------*/
                AX_S32 ret = AX_SYS_UnLink(&m_stModeInfoVdec, &m_stModeInfoIvps);
                if (0 != ret) {
                    THROW_AX_EXCEPTION("unlink VDEC %d %d to IVPS %d %d fail, ret = 0x%x", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                            m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId, ret);
                }
                m_objVdec.Stop();

                /* <------------------ START ------------------*/
                InitReversePlaybackModules(nWidth, nHeight, fFps, nGop, enPayload);
                StartReversePlaybackLinkages();
                LOG_MM_W(TAG, "Forward => Reverse START ---");
            } else { /* Reverse => Forward */
                LOG_MM_W(TAG, "Reverse => Forward START +++");
                /* <------------------ STOP ------------------*/
                StopReversePlaybackLinkages();
                DeInitReversePlaybackModules();
                m_objVdec.Stop();

                /* <------------------ START ------------------*/
                AX_S32 ret = AX_SYS_Link(&m_stModeInfoVdec, &m_stModeInfoIvps);
                if (0 != ret) {
                    THROW_AX_EXCEPTION("link VDEC %d %d to IVPS %d %d fail, ret = 0x%x", m_stModeInfoVdec.s32GrpId, m_stModeInfoVdec.s32ChnId,
                            m_stModeInfoIvps.s32GrpId, m_stModeInfoIvps.s32ChnId, ret);
                }
                LOG_MM_W(TAG, "Reverse => Forward START ---");
            }

            // ivps reset
            m_objIvps.ResetGrp();
            // vo reset
            AX_VO_ClearChnBuf(m_stAttr.nVoLayer, m_stAttr.nVoChannel, AX_TRUE);
            // vdec start
            m_objVdec.Start();
            // datastream restart
            m_stAttr.bReverse = bReverse;
            m_stAttr.pPlayback->ChangeDirection(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.bReverse);
        }

        AX_S32 ret = AX_VO_StepChn(m_stAttr.nVoLayer, m_stAttr.nVoChannel);
        if (0 != ret) {
            LOG_MM_E(TAG, "AX_VO_StepChn[%d-%d] failed, ret=0x%x", m_stAttr.nVoLayer, m_stAttr.nVoChannel, ret);
            break;
        }

        if (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD) {
            m_stAttr.pPlayback->StepFrame(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex);
        }

        bRet = AX_TRUE;
    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::UpdateFps(AX_F32 fFpsFactor) {
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);
    AX_BOOL bRet = AX_FALSE;

    do {
#if 0
        AX_F32 fFps = 0;
        // AX_U32 u32Width = 0;
        // AX_U32 u32Height = 0;
        if (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD) {
            AXDS_STREAM_INFO_T stStream;
            m_stAttr.pPlayback->GetStreamInfo(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.nStartDate, stStream);
            fFps = stStream.uFrameRate;
        } else {
            const STREAM_INFO_T &streamInfo = this->GetStream().GetStreamInfo();
            fFps = streamInfo.stVideo.nFps;
            // u32Width = streamInfo.stVideo.nFps;
            // u32Height = streamInfo.stVideo.nFps;
        }

        if (!this->set_fps(m_stAttr.nVoLayer, m_stAttr.nVoChannel, fFps, fFpsFactor)) {
            break;
        }
#else
        if (m_stAttr.pPlayback == nullptr) {
            LOG_MM_E(TAG, "[%02d:%02d] Invalid playback handle.", m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex);
            break;
        }

        if (!m_stAttr.pPlayback->ChangeSpeed(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, fFpsFactor)) {
            LOG_MM_E(TAG, "[%02d:%02d] Playback change speed failed.", m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex);
            break;
        }
#endif

        bRet = AX_TRUE;

    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return bRet;
}

AX_BOOL CAXNVRChannel::Crop(const AX_VO_RECT_T &stRect, const AX_IVPS_RECT_T &stCropRect, AX_BOOL bCrop) {
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);

    AX_BOOL bRet = AX_FALSE;

    do {
        if (AX_NVR_CHN_STATE::STARTED != m_enState && AX_NVR_CHN_STATE::PAUSED != m_enState) {
            LOG_MM_I(TAG, "[%02d][state:%d] state invalid.", m_stAttr.nDevID, m_enState);
            bRet = AX_TRUE;
            break;
        }

        if (!m_objIvps.CropResize(bCrop, stCropRect)) {
            LOG_MM_E(TAG, "IVPS CropResize Channel<%d> attr failed", m_stAttr.nLinkIvpsVoooChn);
            break;
        }

        if (AX_NVR_CHN_STATE::PAUSED == m_enState) {
            AX_S32 s32Ret = AX_VO_RefreshChn(m_stAttr.nVoLayer, m_stAttr.nVoChannel);
            if (s32Ret != 0) {
                LOG_MM_E(TAG, "AX_VO_RefreshChn 0x%x ", s32Ret);
                break;
            }
        }

        bRet = AX_TRUE;

    } while (0);

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return AX_TRUE;
}

AX_NVR_CHN_RES_T CAXNVRChannel::GetResolution(AX_VOID) {
    LOG_MM_I(TAG, "[%02d:%02d][state:%d] +++", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    std::lock_guard<std::mutex> lock(mutex_);

    AX_NVR_CHN_RES_T stChnRes;
    // AX_U32 u32Width = 0;
    // AX_U32 u32Height = 0;
    if (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RECORD) {
        AXDS_STREAM_INFO_T stStream;
        m_stAttr.pPlayback->GetStreamInfo(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.nStartDate, stStream);
        stChnRes.w = stStream.uWidth;
        stChnRes.h = stStream.uHeight;
    } else {
    #if 0
        const STREAM_INFO_T &streamInfo = this->GetStream().GetStreamInfo();
        stChnRes.w = streamInfo.stVideo.nWidth;
        stChnRes.h = streamInfo.stVideo.nHeight;
    #else
        stChnRes.w = m_objVdec.GetAttr().stChnAttr[m_stAttr.nLinkVdecIvpsChn].stAttr.u32PicWidth;
        stChnRes.h = m_objVdec.GetAttr().stChnAttr[m_stAttr.nLinkVdecIvpsChn].stAttr.u32PicHeight;
    #endif
    }

    LOG_MM_I(TAG, "[%02d:%02d][state:%d] ---", m_stAttr.nDevID, m_stAttr.enIndex, m_enState);
    return stChnRes;
}

VO_CHN CAXNVRChannel::GetCurrentVoChn(AX_VOID) {
    std::lock_guard<std::mutex> lock(mutex_);
    return m_stAttr.nVoChannel;
}

std::pair<AX_U32, AX_U32> CAXNVRChannel::GetCurrentDateTime(AX_VOID) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (m_stAttr.pPlayback) {
        return m_stAttr.pPlayback->GetCurrentDateTime(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex);
    }

    return make_pair<AX_U32, AX_U32>(0, 0);
}

AX_BOOL CAXNVRChannel::init_rtsp(const std::string &strURL, AX_S32 nCookie) {
    extern int ping4(const char *ip, int timeout);

    AX_BOOL bRet = AX_FALSE;

    do {
        if (m_stAttr.bPing) {
            if (0 != ping4(strURL.c_str(), 2 /* seconds */)) {
                LOG_MM_E(TAG, "network to %s is down, please check network", strURL.c_str());
                break;
            }
        }

        STREAM_ATTR_T stAttr;
        stAttr.strURL = strURL;
        stAttr.nCookie = nCookie;
        if (!m_objRtspStream.Init(stAttr)) {
            LOG_MM_E(TAG, "[%02d]RTSP %s Stream initialize failed.", m_stAttr.nDevID, strURL.c_str());
            break;
        }

#if 0 /* move rtsp start after sdk is ready to make sure the 1st decoded nalu is IDR */
        if (!m_objRtspStream.Start()) {
            LOG_MM_E(TAG, "[%02d]RTSP %s Stream start failed.", m_stAttr.nDevID, strURL.c_str());
            m_objRtspStream.DeInit();
            break;
        }
#endif

        if (m_stAttr.bRecord && m_stAttr.enView == AX_NVR_CHN_VIEW_TYPE::PREVIEW) {
            /* Always start rtsp when record enabled */
            if (!m_objRtspStream.Start()) {
                LOG_MM_E(TAG, "[%02d]RTSP %s Stream start failed.", m_stAttr.nDevID, strURL.c_str());
                m_objRtspStream.DeInit();
                break;
            }

            m_objRecordObs.SetAttr(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, m_stAttr.pRecord);
            m_objRtspStream.RegisterObserver(&m_objRecordObs);

            const STREAM_INFO_T &streamInfo = this->GetStream().GetStreamInfo();
            AXDSF_INIT_ATTR_T tStreamAttr = {1 /* minutes */,
                                             streamInfo.stVideo.enPayload,
                                             (AX_U16)streamInfo.stVideo.nFps,
                                             25,
                                             (AX_U16)streamInfo.stVideo.nWidth,
                                             (AX_U16)streamInfo.stVideo.nHeight};
            if (!m_stAttr.pRecord->Start(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex, tStreamAttr)) {
                m_objRtspStream.UnRegisterObserver(&m_objRecordObs);
                LOG_MM_E(TAG, "[%02d:%02d]Record data stream start failed.", m_stAttr.nDevID, m_stAttr.enIndex);
            }
        }

        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_BOOL CAXNVRChannel::init_ffmpeg(AX_VOID) {
    AX_BOOL bRet = AX_FALSE;
    do {
        STREAM_ATTR_T stAttr;
        stAttr.strURL = "/opt/bin/NVRDemo/road3.264";  // for test
        // stAttr.strURL = "/opt/bin/NVRDemo/DEV_00_STREAM_00.h264"; // for test

        stAttr.nFps = -1;
        if (!m_objFFMpegStream.Init(stAttr)) {
            LOG_MM_E(TAG, "[%02d]FFMPEG %s Stream initialize failed.", m_stAttr.nDevID, stAttr.strURL.c_str());
            break;
        }

        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_BOOL CAXNVRChannel::init_vdec(AX_PAYLOAD_TYPE_E enPayload, AX_U32 nWidth, AX_U32 nHeight, const AX_VO_SIZE_T &voSize,
                                 AX_VDEC_GRP vdGrp /* = INVALID_VDEC_GRP */) {
    AX_NVR_FBC_CONFIG_T fbcCfg = CNVRConfigParser::GetInstance()->GetFBCConfig();

    VDEC_ATTR_T stAttr;
    stAttr.vdGrp = vdGrp;
    stAttr.enCodecType = enPayload;
    stAttr.nWidth = ALIGN_UP_16(nWidth);   /* H264 MB 16x16 */
    stAttr.nHeight = ALIGN_UP_16(nHeight); /* H264 MB 16x16 */
    stAttr.nMaxStreamBufSize = stAttr.nWidth * stAttr.nHeight * 3 / 2;
    stAttr.enDecodeMode =
        (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RTSP) ? AX_VDEC_DISPLAY_MODE_PREVIEW : AX_VDEC_DISPLAY_MODE_PLAYBACK;
    stAttr.bPrivatePool = AX_TRUE;

    if (m_stAttr.enStreamSrcType == AX_NVR_CHN_SRC_TYPE::RTSP) {
        /* 2 frame for vdec to send, too long will cause rtsp drop packet */
        AX_U32 fps = GetStream().GetStreamInfo().stVideo.nFps;
        stAttr.nTimeOut = ((fps > 0) ? (1000 / fps * 2) : 100);
    } else {
        stAttr.nTimeOut = -1;
    }

    for (AX_S32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
        switch (j) {
            case 0:
                /* PLAYBACK PP */
                if (AX_NVR_CHN_VIEW_TYPE::PLAYBACK != m_stAttr.enView) {
                    break;
                }

                m_stAttr.nLinkVdecIvpsChn = j;

                stAttr.stChnAttr[j].bEnable = AX_TRUE;
                stAttr.stChnAttr[j].bLinked = AX_TRUE;
                stAttr.stChnAttr[j].stAttr.u32OutputFifoDepth = (stAttr.stChnAttr[j].bLinked) ? 0 : m_stAttr.nPpDepth[j];
                stAttr.stChnAttr[j].stAttr.u32PicWidth = nWidth;
                stAttr.stChnAttr[j].stAttr.u32PicHeight = nHeight;

                if (fbcCfg.nLv > 0) {
                    stAttr.stChnAttr[j].stAttr.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSY;
                    stAttr.stChnAttr[j].stAttr.stCompressInfo.u32CompressLevel = fbcCfg.nLv;
                    stAttr.stChnAttr[j].stAttr.u32PicHeight = ALIGN_DOWN(stAttr.stChnAttr[j].stAttr.u32PicHeight, 4);
                }

                stAttr.stChnAttr[j].stAttr.u32FrameStride = ALIGN_UP(stAttr.stChnAttr[j].stAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                stAttr.stChnAttr[j].stAttr.enOutputMode = AX_VDEC_OUTPUT_ORIGINAL;
                stAttr.stChnAttr[j].stAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;

                if (stAttr.bPrivatePool) {
                    AX_VDEC_CHN_ATTR_T &stChnAttr = stAttr.stChnAttr[j].stAttr;
                    stChnAttr.u32FrameBufSize = CVDEC::GetBlkSize(stChnAttr.u32PicWidth, stChnAttr.u32PicHeight, stAttr.enCodecType,
                                                                  &stChnAttr.stCompressInfo, stChnAttr.enImgFormat);
                    if (stAttr.stChnAttr[j].bLinked) {
                        stChnAttr.u32FrameBufCnt = m_stAttr.nBackupInDepth + 2;
                    } else {
                        stChnAttr.u32FrameBufCnt = m_stAttr.nPpDepth[j] + m_stAttr.nBackupInDepth;
                    }
                }
                break;
            case 1:
                /* PREVIEW PP */
                if (AX_NVR_CHN_VIEW_TYPE::PLAYBACK == m_stAttr.enView) {
                    break;
                }

                m_stAttr.nLinkVdecIvpsChn = j;

                stAttr.stChnAttr[j].bEnable = AX_TRUE;
                stAttr.stChnAttr[j].bLinked = AX_TRUE;
                stAttr.stChnAttr[j].stAttr.u32OutputFifoDepth = (stAttr.stChnAttr[j].bLinked) ? 0 : m_stAttr.nPpDepth[j];

            #if 0 /* consider zoom ++ */
                if (nWidth > voSize.u32Width && nHeight > voSize.u32Height) {
                    /* TO save BW and VPP, scaler down by VDEC PP directly */
                    stAttr.stChnAttr[j].stAttr.u32PicWidth = voSize.u32Width;
                    stAttr.stChnAttr[j].stAttr.u32PicHeight = voSize.u32Height;
                } else {
                    stAttr.stChnAttr[j].stAttr.u32PicWidth = nWidth;
                    stAttr.stChnAttr[j].stAttr.u32PicHeight = nHeight;
                }
            #else
                stAttr.stChnAttr[j].stAttr.u32PicWidth = nWidth;
                stAttr.stChnAttr[j].stAttr.u32PicHeight = nHeight;
            #endif

                if (fbcCfg.nLv > 0) {
                    stAttr.stChnAttr[j].stAttr.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSY;
                    stAttr.stChnAttr[j].stAttr.stCompressInfo.u32CompressLevel = fbcCfg.nLv;
                    stAttr.stChnAttr[j].stAttr.u32PicHeight = ALIGN_DOWN(stAttr.stChnAttr[j].stAttr.u32PicHeight, 4);
                }

                stAttr.stChnAttr[j].stAttr.u32FrameStride = ALIGN_UP(stAttr.stChnAttr[j].stAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                stAttr.stChnAttr[j].stAttr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                stAttr.stChnAttr[j].stAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;

                if (stAttr.bPrivatePool) {
                    AX_VDEC_CHN_ATTR_T &stChnAttr = stAttr.stChnAttr[j].stAttr;
                    stChnAttr.u32FrameBufSize = CVDEC::GetBlkSize(nWidth, nHeight, stAttr.enCodecType,
                                                                  &stChnAttr.stCompressInfo, stChnAttr.enImgFormat);
                    stChnAttr.u32FrameBufCnt = m_stAttr.nPpDepth[j];
                }
                break;
            case 2:
                /* DETECT PP */
                if (m_stAttr.enDetectSrcType == AX_NVR_DETECT_SRC_TYPE::VDEC) {
                    m_stAttr.nRegiVdecDeteChn = j;

                    stAttr.nFps = GetStream().GetStreamInfo().stVideo.nFps;

                    CONST DETECTOR_ATTR_T &detAttr = m_stAttr.pDetector->GetAttr();
                    stAttr.stChnAttr[j].bEnable = AX_TRUE;
                    stAttr.stChnAttr[j].bLinked = AX_FALSE;
                    stAttr.stChnAttr[j].stAttr.u32OutputFifoDepth = (stAttr.stChnAttr[j].bLinked) ? 0 : m_stAttr.nPpDepth[j];
                    stAttr.stChnAttr[j].stAttr.u32PicWidth = detAttr.nWidth;
                    stAttr.stChnAttr[j].stAttr.u32PicHeight = detAttr.nHeight;
                    stAttr.stChnAttr[j].stAttr.u32FrameStride = ALIGN_UP(stAttr.stChnAttr[j].stAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                    stAttr.stChnAttr[j].stAttr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                    stAttr.stChnAttr[j].stAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                    if (detAttr.nSkipRate > 1) {
                        stAttr.stChnAttr[j].stAttr.stOutputFrmRate.bFrmRateCtrl = AX_TRUE;
                        stAttr.stChnAttr[j].stAttr.stOutputFrmRate.f32DstFrmRate = stAttr.nFps * 1.0 / detAttr.nSkipRate;
                    }

                    if (stAttr.bPrivatePool) {
                        AX_VDEC_CHN_ATTR_T &stChnAttr = stAttr.stChnAttr[j].stAttr;
                        stChnAttr.u32FrameBufSize = CVDEC::GetBlkSize(stChnAttr.u32PicWidth, stChnAttr.u32PicHeight, stAttr.enCodecType,
                                                                      &stChnAttr.stCompressInfo, stChnAttr.enImgFormat);
                        stChnAttr.u32FrameBufCnt = m_stAttr.nPpDepth[j];
                    }
                } else {
                    stAttr.stChnAttr[j].bEnable = AX_FALSE;
                }
                break;
            default:
                stAttr.stChnAttr[j].bEnable = AX_FALSE;
                break;
        }
    }

    AX_BOOL bRet = AX_FALSE;
    do {
        if (!m_objVdec.Init(stAttr)) {
            LOG_MM_E(TAG, "[%02d]VDEC initialize failed.", m_stAttr.nDevID);
            break;
        }

        // if (m_enSrcStreamType == AX_NVR_CHN_SRC_TYPE::RTSP) {
        if (!m_objVdec.Start()) {
            m_objVdec.DeInit();
            LOG_MM_E(TAG, "[%02d]VDEC start failed.", m_stAttr.nDevID);
            break;
        }
        // }

        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_BOOL CAXNVRChannel::init_vdec2(AX_PAYLOAD_TYPE_E enPayload, AX_U32 nWidth, AX_U32 nHeight, const AX_VO_SIZE_T &voSize,
                                 AX_VDEC_GRP vdGrp /* = INVALID_VDEC_GRP */) {
    AX_NVR_FBC_CONFIG_T fbcCfg = CNVRConfigParser::GetInstance()->GetFBCConfig();

    VDEC_ATTR_T stAttr;
    stAttr.vdGrp = vdGrp;
    stAttr.enCodecType = enPayload;
    stAttr.enIPBMode = VIDEO_DEC_MODE_I;
    stAttr.nWidth = ALIGN_UP_16(nWidth);   /* H264 MB 16x16 */
    stAttr.nHeight = ALIGN_UP_16(nHeight); /* H264 MB 16x16 */
    stAttr.nMaxStreamBufSize = stAttr.nWidth * stAttr.nHeight * 3 / 2;
    stAttr.enDecodeMode = AX_VDEC_DISPLAY_MODE_PLAYBACK;
    stAttr.bPrivatePool = AX_TRUE;
    stAttr.nTimeOut = -1;

    for (AX_S32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
        switch (j) {
            case 0:
                /* PLAYBACK PP */
                if (AX_NVR_CHN_VIEW_TYPE::PLAYBACK != m_stAttr.enView) {
                    break;
                }

                m_stAttr.nLinkVdecIvpsChn = j;

                stAttr.stChnAttr[j].bEnable = AX_TRUE;
                stAttr.stChnAttr[j].bLinked = AX_TRUE;
                stAttr.stChnAttr[j].stAttr.u32OutputFifoDepth = (stAttr.stChnAttr[j].bLinked) ? 0 : m_stAttr.nPpDepth[j];
                stAttr.stChnAttr[j].stAttr.u32PicWidth = nWidth;
                stAttr.stChnAttr[j].stAttr.u32PicHeight = nHeight;

                if (fbcCfg.nLv > 0) {
                    stAttr.stChnAttr[j].stAttr.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSY;
                    stAttr.stChnAttr[j].stAttr.stCompressInfo.u32CompressLevel = fbcCfg.nLv;
                    stAttr.stChnAttr[j].stAttr.u32PicHeight = ALIGN_DOWN(stAttr.stChnAttr[j].stAttr.u32PicHeight, 4);
                }

                stAttr.stChnAttr[j].stAttr.u32FrameStride = ALIGN_UP(stAttr.stChnAttr[j].stAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                stAttr.stChnAttr[j].stAttr.enOutputMode = AX_VDEC_OUTPUT_ORIGINAL;
                stAttr.stChnAttr[j].stAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;

                if (stAttr.bPrivatePool) {
                    AX_VDEC_CHN_ATTR_T &stChnAttr = stAttr.stChnAttr[j].stAttr;
                    stChnAttr.u32FrameBufSize = CVDEC::GetBlkSize(stChnAttr.u32PicWidth, stChnAttr.u32PicHeight, stAttr.enCodecType,
                                                                  &stChnAttr.stCompressInfo, stChnAttr.enImgFormat);
                    if (stAttr.stChnAttr[j].bLinked) {
                        stChnAttr.u32FrameBufCnt = m_stAttr.nBackupInDepth + 2;
                    } else {
                        stChnAttr.u32FrameBufCnt = m_stAttr.nPpDepth[j] + m_stAttr.nBackupInDepth;
                    }
                }
                break;
            default:
                stAttr.stChnAttr[j].bEnable = AX_FALSE;
                break;
        }
    }

    AX_BOOL bRet = AX_FALSE;
    do {
        if (!m_objVdec2.Init(stAttr)) {
            LOG_MM_E(TAG, "[%02d]VDEC initialize failed.", m_stAttr.nDevID);
            break;
        }

        if (!m_objVdec2.Start()) {
            m_objVdec2.DeInit();
            LOG_MM_E(TAG, "[%02d]VDEC start failed.", m_stAttr.nDevID);
            break;
        }

        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_BOOL CAXNVRChannel::init_ivps(AX_U32 nSrcW, AX_U32 nSrcH, const AX_VO_RECT_T &voChnWin, AX_IVPS_GRP ivGrp) {
    AX_BOOL bRet = AX_FALSE;
    do {
        AX_U32 nChnNum = 0;

        IVPS_ATTR_T stAttr;
        stAttr.nWidth = nSrcW;
        stAttr.nHeight = nSrcH;

        stAttr.stChnAttr[nChnNum].enEngine = AX_IVPS_ENGINE_VPP;
        stAttr.stChnAttr[nChnNum].nWidth = voChnWin.u32Width;
        stAttr.stChnAttr[nChnNum].nHeight = voChnWin.u32Height;
        stAttr.stChnAttr[nChnNum].nStride = ALIGN_UP_16(stAttr.stChnAttr[nChnNum].nWidth);
        stAttr.stChnAttr[nChnNum].bLinked = AX_TRUE;
        stAttr.stChnAttr[nChnNum].nFifoDepth = m_stAttr.nFifoDepthForVo;
        stAttr.stChnAttr[nChnNum].stPoolAttr.ePoolSrc = POOL_SOURCE_PRIVATE;
        stAttr.stChnAttr[nChnNum].stPoolAttr.nFrmBufNum = m_stAttr.nFrmBufNum;

        m_stAttr.nLinkIvpsVoooChn = nChnNum++;

        if (AX_NVR_DETECT_SRC_TYPE::IVPS == m_stAttr.enDetectSrcType) {
            CONST DETECTOR_ATTR_T &detAttr = m_stAttr.pDetector->GetAttr();
            stAttr.stChnAttr[nChnNum].enEngine = AX_IVPS_ENGINE_VPP;
            stAttr.stChnAttr[nChnNum].nWidth = detAttr.nWidth;
            stAttr.stChnAttr[nChnNum].nHeight = detAttr.nHeight;
            stAttr.stChnAttr[nChnNum].nStride = ALIGN_UP_16(stAttr.stChnAttr[nChnNum].nWidth);
            stAttr.stChnAttr[nChnNum].bLinked = AX_FALSE;
            stAttr.stChnAttr[nChnNum].nFifoDepth = m_stAttr.nFifoDepthForDetect;
            stAttr.stChnAttr[nChnNum].stPoolAttr.ePoolSrc = POOL_SOURCE_PRIVATE;
            stAttr.stChnAttr[nChnNum].stPoolAttr.nFrmBufNum = m_stAttr.nFrmBufNum;
            if (detAttr.nSkipRate > 1) {
                stAttr.stChnAttr[nChnNum].stFRC.fSrcFrameRate = m_objRtspStream.GetStreamInfo().stVideo.nFps;
                stAttr.stChnAttr[nChnNum].stFRC.fDstFrameRate = stAttr.stChnAttr[nChnNum].stFRC.fSrcFrameRate / detAttr.nSkipRate;
            }

            m_stAttr.nRegiIvpsDeteChn = nChnNum;

            nChnNum++;
        }

        stAttr.nGrpId = ivGrp;
        stAttr.nChnNum = nChnNum;

        if (AX_NVR_CHN_VIEW_TYPE::PLAYBACK == m_stAttr.enView) {
            stAttr.nBackupInDepth = m_stAttr.nBackupInDepth;
        }

        if (!m_objIvps.Init(stAttr)) {
            LOG_MM_E(TAG, "[%02d]IVPS initialize failed.", m_stAttr.nDevID);
            break;
        }

        // if (m_enSrcStreamType == AX_NVR_CHN_SRC_TYPE::RTSP) {
        if (!m_objIvps.Start()) {
            LOG_MM_E(TAG, "[%02d]IVPS start failed.", m_stAttr.nDevID);
            m_objIvps.DeInit();
            break;
        }
        // }
        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_BOOL CAXNVRChannel::init_ivps2(AX_U32 nSrcW, AX_U32 nSrcH, AX_IVPS_GRP ivGrp) {
    AX_BOOL bRet = AX_FALSE;
    do {
        IVPS_ATTR_T stAttr;
        stAttr.nWidth = nSrcW;
        stAttr.nHeight = nSrcH;

        stAttr.stChnAttr[0].enEngine = AX_IVPS_ENGINE_VPP;
        stAttr.stChnAttr[0].nWidth = nSrcW;
        stAttr.stChnAttr[0].nHeight = nSrcH;
        stAttr.stChnAttr[0].nStride = ALIGN_UP_16(stAttr.stChnAttr[0].nWidth);
        stAttr.stChnAttr[0].bLinked = AX_TRUE;
        stAttr.stChnAttr[0].nFifoDepth = m_stAttr.nFifoDepthForVo;
        stAttr.stChnAttr[0].stPoolAttr.ePoolSrc = POOL_SOURCE_PRIVATE;
        stAttr.stChnAttr[0].stPoolAttr.nFrmBufNum = m_stAttr.nFrmBufNum;

        stAttr.nGrpId = ivGrp;
        stAttr.nChnNum = 1;
        stAttr.nBackupInDepth = m_stAttr.nBackupInDepth;

        AX_NVR_FBC_CONFIG_T fbcCfg = CNVRConfigParser::GetInstance()->GetFBCConfig();
        if (fbcCfg.nLv > 0) {
            stAttr.stChnAttr[0].nStride = ALIGN_UP(stAttr.stChnAttr[0].nWidth, IVPS_FBC_STRIDE_ALIGN);
        }

        if (!m_objIvps2.Init(stAttr)) {
            LOG_MM_E(TAG, "[%02d]IVPS initialize failed.", m_stAttr.nDevID);
            break;
        }

        if (!m_objIvps2.Start()) {
            LOG_MM_E(TAG, "[%02d]IVPS start failed.", m_stAttr.nDevID);
            m_objIvps2.DeInit();
            break;
        }

        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_BOOL CAXNVRChannel::init_transfer(AX_U32 nFps, AX_U32 nGop, VENC_GRP veGrp /*= INVALID_VENC_GRP*/) {
    if (!m_objStreamTrans.Init(veGrp, nFps, nGop)) {
        return AX_FALSE;
    }

    if (!m_objStreamTrans.Start()) {
        LOG_MM_E(TAG, "Start stream pingpong transfer failed.");
        m_objStreamTrans.DeInit();
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::init_venc(AX_PAYLOAD_TYPE_E enPayload, AX_U32 nWidth, AX_U32 nHeight, AX_U32 nFPS, VENC_GRP veGrp /*= INVALID_VENC_GRP*/) {
    AX_BOOL bRet = AX_FALSE;
    do {
        VENC_ATTR_T tVencAttr;

        tVencAttr.nChannel = veGrp;
        tVencAttr.ePayloadType = enPayload;
        tVencAttr.eRcType = (PT_H264 == enPayload ? AX_VENC_RC_MODE_H264CBR : AX_VENC_RC_MODE_H265CBR);
        tVencAttr.nGOP = 1;
        tVencAttr.fFramerate = nFPS;
        tVencAttr.nWidth = nWidth;
        tVencAttr.nHeight = nHeight;
        tVencAttr.nMaxWidth = nWidth;
        tVencAttr.nMaxHeight = nHeight;
        tVencAttr.nBufSize = 2 * 1024 * 1024;
        tVencAttr.nInFifoDepth = 2;
        tVencAttr.nOutFifoDepth = 2;
        tVencAttr.eMemSource = AX_MEMORY_SOURCE_CMM;
        tVencAttr.eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        tVencAttr.nBitrate = 8192;
        tVencAttr.bFBC = AX_FALSE;
        tVencAttr.bLink = AX_TRUE;
        tVencAttr.nGdrNum = 0;

        /* H264 RC config */
        tVencAttr.arrRcConfig[0].ePayloadType = PT_H264;
        tVencAttr.arrRcConfig[0].stRCInfo[0].eRcType = AX_VENC_RC_MODE_H264CBR;
        tVencAttr.arrRcConfig[0].stRCInfo[0].nMinQp = 0;
        tVencAttr.arrRcConfig[0].stRCInfo[0].nMaxQp = 51;
        tVencAttr.arrRcConfig[0].stRCInfo[0].nMinIQp = 0;
        tVencAttr.arrRcConfig[0].stRCInfo[0].nMaxIQp = 51;
        tVencAttr.arrRcConfig[0].stRCInfo[0].nMinIProp = 10;
        tVencAttr.arrRcConfig[0].stRCInfo[0].nMaxIProp = 40;
        tVencAttr.arrRcConfig[0].stRCInfo[0].nIntraQpDelta = -2;
        tVencAttr.arrRcConfig[0].stRCInfo[0].nBitrate = 4096;
        tVencAttr.arrRcConfig[0].stRCInfo[1].eRcType = AX_VENC_RC_MODE_H264VBR;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nMinQp = 0;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nMaxQp = 51;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nMinIQp = 0;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nMaxIQp = 51;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nMinIProp = 10;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nMaxIProp = 40;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nFixQp = 25;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nIntraQpDelta = -2;
        tVencAttr.arrRcConfig[0].stRCInfo[1].nBitrate = 4096;
        /* H265 RC config */
        tVencAttr.arrRcConfig[1].ePayloadType = PT_H265;
        tVencAttr.arrRcConfig[1].stRCInfo[0].eRcType = AX_VENC_RC_MODE_H265CBR;
        tVencAttr.arrRcConfig[1].stRCInfo[0].nMinQp = 0;
        tVencAttr.arrRcConfig[1].stRCInfo[0].nMaxQp = 51;
        tVencAttr.arrRcConfig[1].stRCInfo[0].nMinIQp = 0;
        tVencAttr.arrRcConfig[1].stRCInfo[0].nMaxIQp = 51;
        tVencAttr.arrRcConfig[1].stRCInfo[0].nMinIProp = 10;
        tVencAttr.arrRcConfig[1].stRCInfo[0].nMaxIProp = 40;
        tVencAttr.arrRcConfig[1].stRCInfo[0].nIntraQpDelta = -2;
        tVencAttr.arrRcConfig[1].stRCInfo[0].nBitrate = 4096;
        tVencAttr.arrRcConfig[1].stRCInfo[1].eRcType = AX_VENC_RC_MODE_H265VBR;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nMinQp = 0;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nMaxQp = 51;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nMinIQp = 0;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nMaxIQp = 51;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nMinIProp = 10;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nMaxIProp = 40;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nFixQp = 25;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nIntraQpDelta = -2;
        tVencAttr.arrRcConfig[1].stRCInfo[1].nBitrate = 4096;

        bRet = m_objVenc.Init(tVencAttr);
        if (!bRet) {
            LOG_MM_E(TAG, "[%02d] VENC initialize failed.", m_stAttr.nDevID);
            return AX_FALSE;
        }

        bRet = m_objVenc.Start();
        if (!bRet) {
            LOG_MM_E(TAG, "[%02d] VENC start failed.", m_stAttr.nDevID);
            m_objVenc.DeInit();
        }

        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_BOOL CAXNVRChannel::init_region(AX_S32 nGrp, AX_U32 nSrcW, AX_U32 nSrcH, const AX_VO_RECT_T &voChnWin) {
    AX_BOOL bRet = AX_FALSE;
    do {
        NVR_REGION_ATTR_T rgnAttr;
        rgnAttr.nGroup = nGrp;
        rgnAttr.nChn = m_stAttr.nLinkIvpsVoooChn;
        rgnAttr.nWidth = voChnWin.u32Width;
        rgnAttr.nHeight = voChnWin.u32Height;
        rgnAttr.nSrcW = nSrcW;
        rgnAttr.nSrcH = nSrcH;

        if (m_stAttr.pDisplay->GetFBPaint() == nullptr) {
            rgnAttr.enType = AX_NVR_RGN_TYPE::IVPS;
        } else {
            rgnAttr.enType = AX_NVR_RGN_TYPE::VOFB;
            AX_NVR_RECT_T rect;
            if (m_stAttr.pDisplay->GetChannelRect(m_stAttr.nVoChannel, rect)) {
                rgnAttr.nW = rect.w;
                rgnAttr.nH = rect.h;
            }
            rgnAttr.pVo = const_cast<CVO *>(m_stAttr.pDisplay->GetVo());
            rgnAttr.pFb = const_cast<CFramebufferPaint *>(m_stAttr.pDisplay->GetFBPaint());
            rgnAttr.nVoLayer = m_stAttr.nVoLayer;
            rgnAttr.nVoChannel = m_stAttr.nVoChannel;
        }

        if (!m_objRegion.Init(rgnAttr)) {
            break;
        }

        if (!m_objRegion.Start()) {
            m_objRegion.DeInit();
            break;
        }
        bRet = AX_TRUE;
    } while (0);
    return bRet;
}

CStream0 &CAXNVRChannel::GetStream(AX_VOID) {
    switch (m_stAttr.enStreamSrcType) {
        case AX_NVR_CHN_SRC_TYPE::RTSP:
            return m_objRtspStream;
        case AX_NVR_CHN_SRC_TYPE::FFMPEG:
            return m_objFFMpegStream;
        case AX_NVR_CHN_SRC_TYPE::RECORD:
            return m_objFFMpegStream;
        default:
            break;
    }
    return m_objRtspStream;
}

const AX_NVR_CHN_STATE& CAXNVRChannel::GetCurrentState(AX_VOID) {
    std::lock_guard<std::mutex> lock(mutex_);
    return m_enState;
}

AX_VOID CAXNVRChannel::set_state(AX_NVR_CHN_STATE enStatus) {
    LOG_MM_D(TAG, "[%02d]change state %d to %d", m_stAttr.nDevID, m_enState, enStatus);
    m_enState = enStatus;
}

AX_BOOL CAXNVRChannel::set_fps(VO_LAYER nVoLayer, VO_CHN nVoChannel, AX_F32 fFps, AX_F32 fFactor /*= 1.0f*/) {
    if (m_stAttr.enView == AX_NVR_CHN_VIEW_TYPE::PLAYBACK) {
        AX_S32 ret = AX_VO_SetChnFrameRate(nVoLayer, nVoChannel, fFps * fFactor);
        if (0 != ret) {
            LOG_MM_E(TAG, "AX_VO_SetChnFrameRate[%d-%d] fps=%f, factor=%f failed, ret=0x%x", nVoLayer, nVoChannel, fFps, fFactor, ret);
            return AX_FALSE;
        }
    }
    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::clean(VO_LAYER nVoLayer, VO_CHN nVoChannel) {
    AX_S32 ret = AX_VO_ClearChnBuf(nVoLayer, nVoChannel, AX_TRUE);
    if (0 != ret) {
        LOG_MM_E(TAG, "AX_VO_ClearChnBuf[%d-%d] failed, ret=0x%x", nVoLayer, nVoChannel, ret);
        return AX_FALSE;
    }

    m_stAttr.reset_vo();
    return AX_TRUE;
}

AX_BOOL CAXNVRChannel::pause_resume(AX_BOOL bForceResume) {
    AX_BOOL bRet = AX_FALSE;
    do {
        AX_S32 ret = -1;
        if (AX_NVR_CHN_STATE::STARTED == m_enState) {
            if (!bForceResume) {
                if (AX_NVR_CHN_SRC_TYPE::RECORD == m_stAttr.enStreamSrcType) {
                    m_stAttr.pPlayback->PausePlay(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex);
                    /* TODO: Wait for vo to consume the buffered frames */
                    std::this_thread::sleep_for(std::chrono::milliseconds(400));
                }

                ret = AX_VO_PauseChn(m_stAttr.nVoLayer, m_stAttr.nVoChannel);
                if (0 != ret) {
                    LOG_MM_E(TAG, "AX_VO_PauseChn[%d-%d] failed, ret=0x%x", m_stAttr.nVoLayer, m_stAttr.nVoChannel, ret);
                    break;
                }

                this->set_state(AX_NVR_CHN_STATE::PAUSED);
            }
        } else if (AX_NVR_CHN_STATE::PAUSED == m_enState) {
            ret = AX_VO_ResumeChn(m_stAttr.nVoLayer, m_stAttr.nVoChannel);
            if (0 != ret) {
                LOG_MM_E(TAG, "AX_VO_ResumeChn[%d-%d] failed, ret=0x%x", m_stAttr.nVoLayer, m_stAttr.nVoChannel, ret);
                break;
            }

            if (AX_NVR_CHN_SRC_TYPE::RECORD == m_stAttr.enStreamSrcType) {
                m_stAttr.pPlayback->ResumePlay(m_stAttr.nDevID, (AX_U8)m_stAttr.enIndex);
            }

            this->set_state(AX_NVR_CHN_STATE::STARTED);
        } else {
            LOG_MM_W(TAG, "[%02d][state:%d] state invalid.", m_stAttr.nDevID, m_enState);
            break;
        }

        bRet = AX_TRUE;
    } while (0);
    return bRet;
}
