#include "DspStage.h"
#include "AXLockQ.hpp"
#include "ElapsedTimer.hpp"
#include "ax_dsp_api.h"
#define DSP "dsp"
#define SAMPLE_PHY_MEM_ALIGN_SIZE (128)
#define DSP_GRP_COUNT (2)

CDspStage::CDspStage(DSP_ATTR_S &tDspAttr) : CAXStage(DSP), m_tDspAttr(tDspAttr) {
}

AX_BOOL CDspStage::Init() {
    AX_S32 ret = -1;
    m_arrFrameQ = new (std::nothrow) CAXLockQ<CAXFrame *>[DSP_GRP_COUNT];
    if (!m_arrFrameQ) {
        LOG_MM_E(DSP, "alloc queue fail");
        return AX_FALSE;
    } else {
        for (AX_U32 i = 0; i < DSP_GRP_COUNT; ++i) {
            m_arrFrameQ[i].SetCapacity(10);
        }
    }

    do {
        ret = AX_DSP_PowerOn((AX_DSP_ID_E)m_tDspAttr.nDspId);
        if (ret != AX_DSP_SUCCESS) {
            LOG_MM_E(DSP, "AX DSP Poweron error %x\n", ret);
            break;
        }
        ret = AX_DSP_LoadBin((AX_DSP_ID_E)m_tDspAttr.nDspId, m_tDspAttr.strSramPath.data(), AX_DSP_MEM_TYPE_SRAM);
        if (ret != AX_DSP_SUCCESS) {
            LOG_MM_E(DSP, "AX DSP LoadBin error %x\n", ret);
            break;
        }
        ret = AX_DSP_LoadBin((AX_DSP_ID_E)m_tDspAttr.nDspId, m_tDspAttr.strItcmPath.data(), AX_DSP_MEM_TYPE_ITCM);
        if (ret != AX_DSP_SUCCESS) {
            LOG_MM_E(DSP, "AX DSP LoadBin error %x\n", ret);
            break;
        }
        ret = AX_DSP_EnableCore((AX_DSP_ID_E)m_tDspAttr.nDspId);
        if (ret != AX_DSP_SUCCESS) {
            LOG_MM_E(DSP, "AX DSP Enable Core error %x\n", ret);
            break;
        }

        ret = AX_DSP_CV_Init(m_tDspAttr.nDspId);
        AX_U32 nSrcSize = m_tDspAttr.nDstWidth * m_tDspAttr.nDstHeight * 3 / 2;
        for (AX_U32 i = 0; i < m_tDspAttr.nDeepCnt; i++) {
            DSP_DST_S tDst;
            ret = AX_SYS_MemAlloc((AX_U64 *)&tDst.u64PhyAddr, (AX_VOID **)&tDst.u64VirAddr, nSrcSize, SAMPLE_PHY_MEM_ALIGN_SIZE, (const AX_S8*)"app_dsp_img");
            if (0 != ret) {
                LOG_MM_E(DSP, "AX_SYS_MemAlloc size:%lld Failed.", tDst.u64VirAddr, nSrcSize);
                AX_SYS_MemFree(tDst.u64PhyAddr, (AX_VOID *)tDst.u64VirAddr);
                break;
            } else {
                tDst.bUsable = AX_TRUE;
                m_listDst.push_back(tDst);
            }
        }
        m_DspParam.src_width = m_tDspAttr.nSrcWidth;
        m_DspParam.src_height = m_tDspAttr.nSrcHeight;
        m_DspParam.src_stride = m_tDspAttr.nSrcWidth;
        m_DspParam.dst_width = m_tDspAttr.nDstWidth;
        m_DspParam.dst_height = m_tDspAttr.nDstHeight;
        m_DspParam.dst_stride = m_tDspAttr.nDstWidth;
        m_DspParam.inter_type = 0;

        m_initState = AX_TRUE;
    } while (0);

    if (AX_FALSE == m_initState) {
        delete[] m_arrFrameQ;
        m_arrFrameQ = nullptr;
    }

    return m_initState;
}
AX_BOOL CDspStage::DeInit() {
    LOG_MM_D(DSP, "+++");
    AX_DSP_DisableCore((AX_DSP_ID_E)m_tDspAttr.nDspId);
    AX_DSP_PowerOff((AX_DSP_ID_E)m_tDspAttr.nDspId);

    for (auto dst : m_listDst) {
        AX_SYS_MemFree(dst.u64PhyAddr, (AX_VOID *)dst.u64VirAddr);
    }

    if (m_arrFrameQ) {
        delete[] m_arrFrameQ;
        m_arrFrameQ = nullptr;
    }

    LOG_MM_D(DSP, "---");

    return AX_TRUE;
}
AX_BOOL CDspStage::Start(STAGE_START_PARAM_PTR pStartParams) {
    LOG_MM_D(DSP, "+++");
    if (m_initState) {
        if (!m_DspThread.Start(std::bind(&CDspStage::ProcessFrame, this, std::placeholders::_1), nullptr, "AppDetect")) {
            LOG_MM_E(DSP, "create detect thread fail");
            return AX_FALSE;
        }

        LOG_MM_I(DSP, "---");
    }
    return AX_TRUE;
}

AX_BOOL CDspStage::Stop() {
    if (m_DspThread.IsRunning()) {
        m_DspThread.Stop();
        m_DspThread.Join();
    }
    return AX_TRUE;
}

AX_VOID CDspStage::ProcessFrame(AX_VOID *pArg) {
    LOG_MM_D(DSP, "+++");
    AX_U32 nGrp = m_tDspAttr.nGrp;
    AX_S32 ret = -1;

    DSP_DST_S *dst = nullptr;
    while (m_DspThread.IsRunning()) {
        CAXFrame *pAxInFrame = nullptr;
        for (AX_U32 index = 0; index < m_listDst.size(); index++) {
            if (m_listDst[index].bUsable && m_arrFrameQ[nGrp].Pop(pAxInFrame, 0)) {
                m_listDst[index].bUsable = AX_FALSE;
                dst = &m_listDst[index];
                break;
            }
        }
        if (pAxInFrame) {
            AX_MEM_INFO_T inBuf[3];
            AX_MEM_INFO_T outBuf[3];
            inBuf[0].u64PhyAddr = pAxInFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0];
            inBuf[1].u64PhyAddr = pAxInFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0] +
                                  pAxInFrame->stFrame.stVFrame.stVFrame.u32PicStride[0] * pAxInFrame->stFrame.stVFrame.stVFrame.u32Height;
            inBuf[2].u64PhyAddr = 0;

            outBuf[0].u64PhyAddr = dst->u64PhyAddr;
            outBuf[1].u64PhyAddr = dst->u64PhyAddr + m_DspParam.dst_stride * m_DspParam.dst_height;
            outBuf[2].u64PhyAddr = 0;
            ret = AX_DSP_CV_ResizeAll(m_tDspAttr.nDspId, inBuf, outBuf, &m_DspParam);
            if (AX_DSP_SUCCESS != ret) {
                LOG_MM_E(DSP, "AX_DSP_CV_ResizeAll failed ret:%d", ret);
                dst->bUsable = AX_TRUE;
            } else {
                CAXFrame *pAxOutFrame = new (std::nothrow) CAXFrame();
                memcpy(&pAxOutFrame->stFrame.stVFrame.stVFrame, &pAxInFrame->stFrame.stVFrame.stVFrame, sizeof(AX_VIDEO_FRAME_T));
                if (!pAxOutFrame) {
                    LOG_M_E(DSP, "alloc MediaFrame instance fail");
                    dst->bUsable = AX_TRUE;
                    continue;
                }
                pAxOutFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0] = dst->u64PhyAddr;
                pAxOutFrame->stFrame.stVFrame.stVFrame.u64VirAddr[0] = dst->u64VirAddr;
                pAxOutFrame->stFrame.stVFrame.stVFrame.u32Width = m_DspParam.dst_width;
                pAxOutFrame->stFrame.stVFrame.stVFrame.u32Height = m_DspParam.dst_height;
                pAxOutFrame->stFrame.stVFrame.stVFrame.u32PicStride[0] = m_DspParam.dst_width;
                pAxOutFrame->pFrameRelease = this;
                NotifyAll(m_tDspAttr.nGrp, 0, pAxOutFrame);
            }
            pAxInFrame->FreeMem();
        } else {
            CElapsedTimer::GetInstance()->mSleep(10);
        }
    }
}

AX_BOOL CDspStage::SendFrame(AX_U32 nGrp, CAXFrame *pAxFrame) {
    if (!m_initState) {
        pAxFrame->FreeMem();
        LOG_MM_W(DSP, "nGrp:%d,m_dspAttr.grp:%d, unusable.", nGrp, m_tDspAttr.nGrp);
        return AX_TRUE;
    } else {
        if (!m_arrFrameQ[m_tDspAttr.nGrp].Push(pAxFrame)) {
            LOG_MM_W(DSP, "push grp[%d] frame %lld to q fail.GetCapacity:%d", m_tDspAttr.nGrp, pAxFrame->stFrame.stVFrame.stVFrame.u64SeqNum,
                     m_arrFrameQ[m_tDspAttr.nGrp].GetCapacity());
            pAxFrame->FreeMem();
            CElapsedTimer::GetInstance()->mSleep(1);
        }
    }
    return AX_TRUE;
}

AX_VOID CDspStage::RegObserver(AX_S32 nChannel, IObserver *pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nGroup = m_tDspAttr.nGrp;
        tTransAttr.nChannel = 0;
        tTransAttr.bLink = AX_FALSE;
        tTransAttr.nWidth = m_DspParam.dst_width;
        tTransAttr.nHeight = m_DspParam.dst_height;
        tTransAttr.fSrcFramerate = m_tDspAttr.fSrcFramerate;
        tTransAttr.nSnsSrc = m_tDspAttr.nGrp;
        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_DETECT, m_tDspAttr.nGrp, 0, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CDspStage::NotifyAll(AX_U32 nGrp, AX_U32 nType, AX_VOID *pAxFrame) {
    if (nullptr == pAxFrame) {
        return;
    }

    for (std::vector<IObserver *>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_DSP, nGrp, nType, pAxFrame);
    }
}

AX_VOID CDspStage::VideoFrameRelease(CAXFrame *pAxFrame) {
    if (pAxFrame) {
        for (auto &dst : m_listDst) {
            if (dst.u64PhyAddr == pAxFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0]) {
                dst.bUsable = AX_TRUE;
            }
        }
        delete pAxFrame;
        pAxFrame = nullptr;
    }
}
