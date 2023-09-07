/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "VideoDecoder.hpp"
#include <errno.h>
#include <string.h>
#include <algorithm>
#include <set>
#include <unordered_map>
#include "AppLogApi.h"
#include "ax_buffer_tool.h"
#include "make_unique.hpp"

using namespace std;
#define VDEC "VDEC"
#define PTS_TIME_BASE (1000000)

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

struct pair_hash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
    }
};

struct pair_equal {
    template <typename T1, typename T2>
    bool operator()(const std::pair<T1, T2>& a, const std::pair<T1, T2>& b) const {
        return a.first == b.first && a.second == b.second;
    }
};

AX_VOID CVideoDecoder::RecvThread(AX_VOID* pArg) {
    LOG_M_D(VDEC, "%s: +++", __func__);

    typedef struct VDEC_CHN_INFO_S {
        AX_U32 nFps = {0};
        AX_U64 nSeqNum = {0};
        AX_U64 nPTS = {0};
#ifdef __DUMP_VDEC_FRAME__
        ofstream ofs;
#endif
        VDEC_CHN_INFO_S(AX_VOID) = default;
        VDEC_CHN_INFO_S(AX_U32 fps, AX_U64 seq, AX_U64 pts) {
            nFps = fps;
            nSeqNum = seq;
            nPTS = pts;
        }

#ifdef __DUMP_VDEC_FRAME__
        /* c++11 rule of five: if dtor is delcared, other 4 need to be decared too */
        VDEC_CHN_INFO_S(const VDEC_CHN_INFO_S&) = default;
        VDEC_CHN_INFO_S(VDEC_CHN_INFO_S&&) = default;
        VDEC_CHN_INFO_S& operator=(const VDEC_CHN_INFO_S&) = default;
        VDEC_CHN_INFO_S& operator=(VDEC_CHN_INFO_S&&) = default;
        ~VDEC_CHN_INFO_S(AX_VOID) {
            if (ofs.is_open()) {
                ofs.close();
            }
        }
#endif
    } VDEC_CHN_INFO_T;

    unordered_map<pair<AX_VDEC_GRP, AX_VDEC_CHN>, VDEC_CHN_INFO_T, pair_hash, pair_equal> mapChnInfo;
    std::set<AX_VDEC_GRP> setGrps;
    const AX_VDEC_GRP VDEC_GRP_NUM = (const AX_VDEC_GRP)m_arrGrpInfo.size();
    for (AX_VDEC_GRP i = 0; i < VDEC_GRP_NUM; ++i) {
        if (!m_arrGrpInfo[i].bActive) {
            continue;
        }

        setGrps.insert(i);

        for (AX_VDEC_CHN j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
            if (!m_arrGrpInfo[i].stAttr.bChnEnable[j]) {
                continue;
            }

            mapChnInfo[{i, j}] = {m_arrGrpInfo[i].stAttr.nFps, 0, 0};
        }
    }

    AX_VDEC_GRP_SET_INFO_T stGrpSet;
    constexpr AX_S32 TIMEOUT = -1; /* fixme: AX650SW-2164 */
    AX_S32 ret;
    while (m_DecodeThread.IsRunning()) {
        if (0 == setGrps.size()) {
            break;
        }

        ret = AX_VDEC_SelectGrp(&stGrpSet, 1000);
        if (0 != ret) {
            if (AX_ERR_VDEC_FLOW_END == ret) {
                LOG_M_W(VDEC, "AX_VDEC_SelectGrp() recv flow end");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else {
                if (AX_ERR_VDEC_TIMED_OUT != ret) {
                    LOG_M_E(VDEC, "AX_VDEC_SelectGrp() fail, ret = 0x%x", ret);
                } else {
                    LOG_M_I(VDEC, "AX_VDEC_SelectGrp() timeout");
                }
            }
            continue;
        }

        std::lock_guard<std::mutex> stopLck(m_mtxStop);
        if (!m_DecodeThread.IsRunning()) {
            break;
        }

        if (0 == stGrpSet.u32GrpCount) {
            /* how could this happen, just make sure */
            LOG_M_E(VDEC, "AX_VDEC_SelectGrp() success, but return vdGrp count is 0");
            break;
        }

        for (AX_U32 i = 0; i < stGrpSet.u32GrpCount; ++i) {
            for (AX_U32 j = 0; j < stGrpSet.stChnSet[i].u32ChnCount; ++j) {
                AX_VDEC_GRP vdGrp = stGrpSet.stChnSet[i].VdGrp;
                AX_VDEC_CHN vdChn = stGrpSet.stChnSet[i].VdChn[j];
                if (vdGrp < 0 || vdChn < 0) {
                    /* how could this happen, just make sure */
                    LOG_M_E(VDEC, "AX_VDEC_SelectGrp() success, but return invalid vdGrp %d or vdChn %d", vdGrp, vdChn);
                    return;
                }

                VDEC_CHN_INFO_T& tChnInfo = mapChnInfo[{vdGrp, vdChn}];
                if (1 == ++tChnInfo.nSeqNum || 0 == tChnInfo.nFps) {
                    /* 1st frame or unknown fps */
                    AX_SYS_GetCurPTS(&tChnInfo.nPTS);
                } else {
                    tChnInfo.nPTS += (PTS_TIME_BASE / tChnInfo.nFps);
                }

                AX_VIDEO_FRAME_INFO_T stVFrame;
                ret = AX_VDEC_GetChnFrame(vdGrp, vdChn, &stVFrame, TIMEOUT);
                if (0 != ret) {
                    if (AX_ERR_VDEC_FLOW_END == ret || AX_ERR_VDEC_UNEXIST == ret) {
                        setGrps.erase(vdGrp);
                        LOG_M_C(VDEC, "vdGrp %2d vdChn %d decode flow end, left %2d", vdGrp, vdChn, setGrps.size());
                        continue;
                    } else if (AX_ERR_VDEC_STRM_ERROR == ret) {
                        LOG_M_N(VDEC, "AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d): stream is undecodeable", vdGrp, vdChn);
                        continue;
                    } else {
                        LOG_M_E(VDEC, "AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d, timeout %d) fail, ret = 0x%x", vdGrp, vdChn, TIMEOUT, ret);
                        return;
                    }
                }

                /* SDK only return 0, needs to release */
                if (AX_INVALID_BLOCKID == stVFrame.stVFrame.u32BlkId[0]) {
                    LOG_M_E(VDEC, "AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d) recv invalid frame blk id", vdGrp, vdChn);
                    return;
                }

                if (0 == stVFrame.stVFrame.u32Width || 0 == stVFrame.stVFrame.u32Height ||
                    m_arrGrpInfo[vdGrp].stAttr.stChnAttr[vdChn].enImgFormat != stVFrame.stVFrame.enImgFormat) {
                    LOG_M_W(VDEC, "AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d) recv invalid frame %dx%d, pxl fmt %d, blk 0x%x", vdGrp, vdChn,
                            stVFrame.stVFrame.u32Width, stVFrame.stVFrame.u32Height, stVFrame.stVFrame.enImgFormat,
                            stVFrame.stVFrame.u32BlkId[0]);

                    /* if valid blk id, should release */
                    ret = AX_VDEC_ReleaseChnFrame(vdGrp, vdChn, &stVFrame);
                    if (0 != ret) {
                        LOG_M_E(VDEC, "AX_VDEC_ReleaseChnFrame(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
                        return;
                    }

                    continue;
                }

#if defined(__RECORD_VB_TIMESTAMP__)
                if (!CHECK_VALID_PRIVATE_FRAME_DATA(stVFrame.stVFrame.u64PrivateData)) {
                    LOG_M_E(VDEC,
                            "vdGrp %d vdChn %d frame %lld: invalid private data 0xll%X, pts %lld phy 0x%llx %dx%d stride %d blkId 0x%x",
                            vdGrp, vdChn, stVFrame.stVFrame.u64SeqNum, stVFrame.stVFrame.u64PrivateData, stVFrame.stVFrame.u64PTS,
                            stVFrame.stVFrame.u64PhyAddr[0], stVFrame.stVFrame.u32Width, stVFrame.stVFrame.u32Height,
                            stVFrame.stVFrame.u32PicStride[0], stVFrame.stVFrame.u32BlkId[0]);
                } else {
                    AX_U64 nTimeStamp = m_arrTimestampHelper[vdGrp].LoadTimeStamp(stVFrame.stVFrame.u64PrivateData);
                    if (INVALID_TIMESTAMP == nTimeStamp) {
                        LOG_M_E(VDEC, "invalid vdGrp %d private frame timestamp <0xll%X - %lld>", vdGrp, stVFrame.stVFrame.u64PrivateData,
                                nTimeStamp);
                        nTimeStamp = 0x1; /* just set an invalid timestamp to disable sys set a new one */
                    } else {
                        LOG_M_D(VDEC, "[vdGrp %02d, idx %02d] <-- private frame id = 0x%llX, timestamp = %lld", vdGrp,
                                FRAME_PRIVATE_APPID(stVFrame.stVFrame.u64PrivateData), stVFrame.stVFrame.u64PrivateData, nTimeStamp);
                    }

                    CTimestampHelper::ClearTimestamps(stVFrame.stVFrame);
                    CTimestampHelper::RecordTimestamp(stVFrame.stVFrame, vdGrp, vdChn, TIMESTAMP_VDEC_SEND, nTimeStamp);
                    CTimestampHelper::RecordTimestamp(stVFrame.stVFrame, vdGrp, vdChn, TIMESTAMP_VDEC_RECV);
                }
#endif

                CAXFrame axFrame;
                axFrame.nGrp = (AX_S32)vdGrp;
                axFrame.nChn = (AX_S32)vdChn;
                axFrame.stFrame.stVFrame = stVFrame;
                axFrame.stFrame.stVFrame.stVFrame.u64PTS = tChnInfo.nPTS;

                LOG_M_N(VDEC, "decoded vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx %dx%d stride %d blkId 0x%x size %d", axFrame.nGrp,
                        axFrame.nChn, axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
                        axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32Width,
                        axFrame.stFrame.stVFrame.stVFrame.u32Height, axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0],
                        axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0], axFrame.stFrame.stVFrame.stVFrame.u32FrameSize);

#ifdef __DUMP_VDEC_FRAME__
                if (!tChnInfo.ofs.is_open()) {
                    AX_CHAR szFile[64];
                    sprintf(szFile, "./dump_%dx%d_grp%d_chn%d.nv12.img", stVFrame.stVFrame.u32PicStride[0], stVFrame.stVFrame.u32Height,
                            vdGrp, vdChn);
                    tChnInfo.ofs.open(szFile, ofstream::out | ofstream::binary | ofstream::trunc);
                }

                if (tChnInfo.ofs.is_open()) {
                    tChnInfo.ofs.write((const char*)axFrame.stFrame.stVFrame.stVFrame.u64VirAddr[0],
                                       axFrame.stFrame.stVFrame.stVFrame.u32FrameSize);
                }
#endif
                Notify(axFrame);

                ret = AX_VDEC_ReleaseChnFrame(vdGrp, vdChn, &stVFrame);
                LOG_M_D(VDEC, "release vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx %dx%d stride %d, blkId 0x%x", axFrame.nGrp,
                        axFrame.nChn, axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
                        axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32Width,
                        axFrame.stFrame.stVFrame.stVFrame.u32Height, axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0],
                        axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0]);
                if (0 != ret) {
                    LOG_M_E(VDEC, "AX_VDEC_ReleaseChnFrame(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
                    return;
                }
            }  // end for (AX_U32 j = 0; j < stGrpSet.stChnSet[i].u32ChnCount; ++j)
        }      // end for (AX_U32 i = 0; i < stGrpSet.u32GrpCount; ++i)
    }

    LOG_M_C(VDEC, ">>>>>>>>>>>>>>> decode thread quited <<<<<<<<<<<<<<<");
}

AX_U32 CVideoDecoder::GetBlkSize(AX_U32 nW, AX_U32 nH, AX_U32 nAlign, AX_PAYLOAD_TYPE_E eType,
                                 AX_FRAME_COMPRESS_INFO_T* pstCompressInfo /* = nullptr*/,
                                 AX_IMG_FORMAT_E ePxlFmt /* = AX_FORMAT_YUV420_SEMIPLANAR*/) {
    return AX_VDEC_GetPicBufferSize(nW, nH, ePxlFmt, pstCompressInfo, eType);
}

AX_BOOL CVideoDecoder::Init(const std::vector<VDEC_GRP_ATTR_T>& v) {
    LOG_M_D(VDEC, "%s: +++", __func__);

    if (0 == count_if(v.begin(), v.end(), [](const VDEC_GRP_ATTR_T& m) { return m.bEnable ? true : false; })) {
        LOG_M_E(VDEC, "%s: 0 enable vdec grp", __func__);
        return AX_FALSE;
    }

    const AX_U32 VDEC_GRP_NUM = v.size();
    m_arrGrpInfo.resize(VDEC_GRP_NUM);
    m_arrCacheBuf.resize(VDEC_GRP_NUM);
#if defined(__RECORD_VB_TIMESTAMP__)
    m_arrTimestampHelper.resize(VDEC_GRP_NUM);
#endif

    for (AX_VDEC_GRP vdGrp = 0; vdGrp < (AX_VDEC_GRP)VDEC_GRP_NUM; ++vdGrp) {
        VDEC_GRP_INFO_T& stGrpInfo = m_arrGrpInfo[vdGrp];
        stGrpInfo.stAttr = v[vdGrp];

        if (!stGrpInfo.stAttr.bEnable) {
            continue;
        }

        if (!CreateDecoder(vdGrp, stGrpInfo)) {
            DeInit();
            return AX_FALSE;
        } else {
            stGrpInfo.bActive = AX_TRUE;

            for (AX_VDEC_CHN i = 0; i < MAX_VDEC_CHN_NUM; ++i) {
                if (stGrpInfo.stAttr.bChnEnable[i]) {
                    if (0 == stGrpInfo.stAttr.stChnAttr[i].u32OutputFifoDepth) {
                        stGrpInfo.bLinked = AX_TRUE;
                        break;
                    }
                }
            }
        }

        if (AX_VDEC_INPUT_MODE_STREAM == stGrpInfo.stAttr.enInputMode) {
            m_arrCacheBuf[vdGrp] = make_unique<CStreamCacheBuf>(stGrpInfo.stAttr.nMaxStreamBufSize);
            if (!m_arrCacheBuf[vdGrp]) {
                DeInit();
                return AX_FALSE;
            }
        }
    }

    LOG_M_D(VDEC, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::DeInit(AX_VOID) {
    LOG_M_D(VDEC, "%s: +++", __func__);

    if (m_DecodeThread.IsRunning()) {
        LOG_M_E(VDEC, "%s: decode thread is running", __func__);
        return AX_FALSE;
    }

    AX_S32 ret;
    const AX_U32 VDEC_GRP_NUM = m_arrGrpInfo.size();
    for (AX_VDEC_GRP vdGrp = 0; vdGrp < (AX_VDEC_GRP)VDEC_GRP_NUM; ++vdGrp) {
        if (!m_arrGrpInfo[vdGrp].stAttr.bEnable) {
            continue;
        }

        if (m_arrGrpInfo[vdGrp].bActive) {
            // LOG_M_W(VDEC, "AX_VDEC_DestroyGrp(vdGrp %d) +++", vdGrp);
            for (AX_U32 i = 0; i < 3; ++i) {
                ret = AX_VDEC_DestroyGrp(vdGrp);
                if (0 == ret) {
                    break;
                } else {
                    LOG_M_W(VDEC, "AX_VDEC_DestroyGrp(vdGrp %d) fail %d times", vdGrp, i + 1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            // LOG_M_W(VDEC, "AX_VDEC_DestroyGrp(vdGrp %d) ---, ret = 0x%x", vdGrp, ret);
            if (0 != ret) {
                LOG_M_E(VDEC, "AX_VDEC_DestroyGrp(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
            }

            m_arrGrpInfo[vdGrp].bActive = AX_FALSE;
            LOG_M_C(VDEC, "vdGrp %2d is destoryed", vdGrp);
        }
    }

    LOG_M_D(VDEC, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::GetGrpAttr(AX_VDEC_GRP vdGrp, VDEC_GRP_ATTR_T& stGrpAttr) const {
    if (vdGrp >= (AX_VDEC_GRP)m_arrGrpInfo.size()) {
        LOG_M_E(VDEC, "%s: invalid vdGrp %d", __func__, vdGrp);
        return AX_FALSE;
    }

    stGrpAttr = m_arrGrpInfo[vdGrp].stAttr;
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::GetAllGrpAttr(std::vector<VDEC_GRP_ATTR_T>& vecGrpAttr) {
    for (auto& tGrpInfo : m_arrGrpInfo) {
        vecGrpAttr.push_back(tGrpInfo.stAttr);
    }
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::Start(AX_VOID) {
    LOG_M_D(VDEC, "%s: +++", __func__);

    AX_S32 ret;

    /* as for link mode, set output fifo depth to 0, then App will not launch decode thread */
    AX_BOOL bRcvThread = {AX_FALSE};
    for (auto&& m : m_arrGrpInfo) {
        if (m.bActive) {
            for (auto&& n : m.stAttr.stChnAttr) {
                if (n.u32OutputFifoDepth > 0) {
                    bRcvThread = AX_TRUE;
                    break;
                }
            }

            if (bRcvThread) {
                break;
            }
        }
    }

    if (bRcvThread) {
        if (!m_DecodeThread.Start([this](AX_VOID* pArg) -> AX_VOID { RecvThread(pArg); }, this, "AppDecode", SCHED_FIFO, 99)) {
            LOG_M_E(VDEC, "%s: start receive thread fail", __func__);
            return AX_FALSE;
        }
    }

    const AX_U32 VDEC_GRP_NUM = m_arrGrpInfo.size();
    for (AX_VDEC_GRP vdGrp = 0; vdGrp < (AX_VDEC_GRP)VDEC_GRP_NUM; ++vdGrp) {
        if (!m_arrGrpInfo[vdGrp].stAttr.bEnable) {
            continue;
        }

        AX_VDEC_RECV_PIC_PARAM_T stRecvParam;
        memset(&stRecvParam, 0, sizeof(stRecvParam));
        stRecvParam.s32RecvPicNum = -1;
        ret = AX_VDEC_StartRecvStream(vdGrp, &stRecvParam);
        if (0 != ret) {
            LOG_M_E(VDEC, "AX_VDEC_StartRecvStream(vdGrp %d) fail, ret = 0x%x", vdGrp);
            Stop();
            return AX_FALSE;
        }

        m_arrGrpInfo[vdGrp].bStarted = AX_TRUE;

#if defined(__RECORD_VB_TIMESTAMP__)
        m_arrTimestampHelper[vdGrp].Reset();
#endif
    }

    LOG_M_D(VDEC, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::Stop(AX_VOID) {
    LOG_M_D(VDEC, "%s: +++", __func__);

    AX_S32 ret;

    {
        std::lock_guard<std::mutex> lckStop(m_mtxStop);

        const AX_U32 VDEC_GRP_NUM = m_arrGrpInfo.size();
        for (AX_VDEC_GRP vdGrp = 0; vdGrp < (AX_VDEC_GRP)VDEC_GRP_NUM; ++vdGrp) {
            if (!m_arrGrpInfo[vdGrp].bActive || !m_arrGrpInfo[vdGrp].stAttr.bEnable) {
                continue;
            }

            AX_VDEC_GRP_STATUS_T vdGrpStatus;
            memset(&vdGrpStatus, 0, sizeof(vdGrpStatus));
            ret = AX_VDEC_QueryStatus(vdGrp, &vdGrpStatus);
            if (0 != ret) {
                LOG_M_E(VDEC, "%s: AX_VDEC_QueryStatus(vdGrp %d) fail, ret = 0x%x", __func__, vdGrp, ret);
            } else {
                if (vdGrpStatus.bStartRecvStream) {
                    LOG_M_C(VDEC, "stop vdGrp %d +++", vdGrp);

                    ret = AX_VDEC_StopRecvStream(vdGrp);
                    if (0 != ret) {
                        LOG_M_E(VDEC, "%s: AX_VDEC_StopRecvStream(vdGrp %d) fail, ret = 0x%x", __func__, vdGrp, ret);
                        return AX_FALSE;
                    }

                    m_arrGrpInfo[vdGrp].bStarted = AX_FALSE;

                    ret = AX_VDEC_ResetGrp(vdGrp);
                    if (0 != ret) {
                        LOG_M_E(VDEC, "%s: AX_VDEC_ResetGrp(vdGrp %d) fail, ret = 0x%x", __func__, vdGrp, ret);
                        return AX_FALSE;
                    }

#if 0
                    AX_VDEC_RECV_PIC_PARAM_T stRecvParam;
                    memset(&stRecvParam, 0, sizeof(stRecvParam));
                    stRecvParam.s32RecvPicNum = -1;
                    ret = AX_VDEC_StartRecvStream(vdGrp, &stRecvParam);
                    if (0 != ret) {
                        LOG_M_E(VDEC, "%s: AX_VDEC_StartRecvStream(vdGrp %d) fail, ret = 0x%x", __func__, vdGrp, ret);
                        return AX_FALSE;
                    }

                    AX_VDEC_STREAM_T eof;
                    memset(&eof, 0, sizeof(eof));
                    eof.bEndOfFrame = AX_TRUE;
                    eof.bEndOfStream = AX_TRUE;
                    ret = AX_VDEC_SendStream(vdGrp, &eof, -1);
                    if (0 != ret) {
                        LOG_M_E(VDEC, "%s: AX_VDEC_SendStream(vdGrp %d eof) fail, ret = 0x%x", __func__, vdGrp, ret);
                        return AX_FALSE;
                    }

                    ret = AX_VDEC_StopRecvStream(vdGrp);
                    if (0 != ret) {
                        LOG_M_E(VDEC, "%s: AX_VDEC_StopRecvStream(vdGrp %d) fail, ret = 0x%x", __func__, vdGrp, ret);
                        return AX_FALSE;
                    }
#endif

                    LOG_M_C(VDEC, "stop vdGrp %d ---", vdGrp);
                }
            }
        }

        m_DecodeThread.Stop();
    }

    m_DecodeThread.Join();

    LOG_M_D(VDEC, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::RegObserver(AX_VDEC_GRP vdGrp, IObserver* pObs) {
    if (!pObs) {
        LOG_M_E(VDEC, "%s observer is nil", __func__);
        return AX_FALSE;
    }

    if (m_DecodeThread.IsRunning()) {
        LOG_M_E(VDEC, "%s is not allowed after decode thread is running", __func__);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxObs);
    auto kv = m_mapObs.find(vdGrp);
    if (kv == m_mapObs.end()) {
        list<IObserver*> lstObs;
        lstObs.push_back(pObs);
        m_mapObs[vdGrp] = lstObs;
    } else {
        for (auto& m : kv->second) {
            if (m == pObs) {
                LOG_M_W(VDEC, "%s: observer %p is already registed to vdGrp %d", __func__, pObs, vdGrp);
                return AX_TRUE;
            }
        }

        kv->second.push_back(pObs);
    }

    LOG_M_I(VDEC, "%s: register observer %p to vdGrp %d ok", __func__, pObs, vdGrp);
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::UnRegObserver(AX_VDEC_GRP vdGrp, IObserver* pObs) {
    if (!pObs) {
        LOG_M_E(VDEC, "%s observer is nil", __func__);
        return AX_FALSE;
    }

    if (m_DecodeThread.IsRunning()) {
        LOG_M_E(VDEC, "%s is not allowed after decode thread is running", __func__);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxObs);
    auto kv = m_mapObs.find(vdGrp);
    if (kv != m_mapObs.end()) {
        for (auto& m : kv->second) {
            if (m == pObs) {
                kv->second.remove(pObs);
                LOG_M_I(VDEC, "%s: unregist observer %p from vdGrp %d ok", __func__, pObs, vdGrp);
                return AX_TRUE;
            }
        }
    }

    LOG_M_E(VDEC, "%s: observer %p is not registed to vdGrp %d", __func__, pObs, vdGrp);
    return AX_FALSE;
}

AX_BOOL CVideoDecoder::UnRegAllObservers(AX_VOID) {
    std::lock_guard<std::mutex> lck(m_mtxObs);
    m_mapObs.clear();
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::CreateDecoder(AX_VDEC_GRP vdGrp, const VDEC_GRP_INFO_T& stGrpInfo) {
    AX_VDEC_GRP_ATTR_T stGrpAttr;
    memset(&stGrpAttr, 0, sizeof(stGrpAttr));
    stGrpAttr.enCodecType = stGrpInfo.stAttr.enCodecType;
    stGrpAttr.enInputMode = stGrpInfo.stAttr.enInputMode;
    stGrpAttr.u32MaxPicWidth = ALIGN_UP(stGrpInfo.stAttr.nMaxWidth, 16);   /* H264 MB 16x16 */
    stGrpAttr.u32MaxPicHeight = ALIGN_UP(stGrpInfo.stAttr.nMaxHeight, 16); /* H264 MB 16x16 */
    stGrpAttr.u32StreamBufSize = stGrpInfo.stAttr.nMaxStreamBufSize;

    stGrpAttr.bSdkAutoFramePool = stGrpInfo.stAttr.bPrivatePool;

    /* let SDK copy input stream buffer */
    stGrpAttr.bSkipSdkStreamPool = AX_FALSE;

    LOG_M_C(VDEC, "create vdGrp %d: codec %d, %dx%d, input mode %d, stream buf size 0x%x, private pool %d", vdGrp, stGrpAttr.enCodecType,
            stGrpAttr.u32MaxPicWidth, stGrpAttr.u32MaxPicHeight, stGrpAttr.enInputMode, stGrpAttr.u32StreamBufSize,
            stGrpAttr.bSdkAutoFramePool);

    AX_S32 ret = AX_VDEC_CreateGrp(vdGrp, &stGrpAttr);
    if (0 != ret) {
        LOG_M_E(VDEC, "AX_VDEC_CreateGrp(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
        return AX_FALSE;
    }

    try {
        AX_VDEC_GRP_PARAM_T stGrpParam;
        memset(&stGrpParam, 0, sizeof(stGrpParam));
        /*
            tick: 6298
            In order to decrease VB num, config VDEC to decode order (no reorder)
            Make sure video stream has no B frames
        */
        stGrpParam.stVdecVideoParam.enOutputOrder = AX_VDEC_OUTPUT_ORDER_DEC;
        // stGrpParam.bFrameRateCtrl = stGrpInfo.stAttr.bFramerateCtrl;
        // if (stGrpParam.bFrameRateCtrl && stGrpInfo.stAttr.nFps > 0) {
        //     stGrpParam.f32FPS = stGrpInfo.stAttr.nFps;
        // }

#ifdef __VDEC_PP_FRAME_CTRL__
        stGrpParam.f32SrcFrmRate = stGrpInfo.stAttr.nFps;
#endif
        ret = AX_VDEC_SetGrpParam(vdGrp, &stGrpParam);
        if (0 != ret) {
            LOG_M_E(VDEC, "AX_VDEC_SetGrpParam(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
            throw 1;
        }

        ret = AX_VDEC_SetDisplayMode(vdGrp, stGrpInfo.stAttr.eDecodeMode);
        if (0 != ret) {
            LOG_M_E(VDEC, "AX_VDEC_SetGrpParam(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
            throw 1;
        }

        for (AX_VDEC_CHN vdChn = 0; vdChn < MAX_VDEC_CHN_NUM; ++vdChn) {
            if (stGrpInfo.stAttr.bChnEnable[vdChn]) {
                const AX_VDEC_CHN_ATTR_T& stChnAttr = stGrpInfo.stAttr.stChnAttr[vdChn];

                LOG_M_C(VDEC, "enable vdGrp %d vdChn %d: %dx%d stride %d padding %d, fifo depth %d, mode %d, compress %d lv %d, fps = %f",
                        vdGrp, vdChn, stChnAttr.u32PicWidth, stChnAttr.u32PicHeight, stChnAttr.u32FrameStride, stChnAttr.u32FramePadding,
                        stChnAttr.u32OutputFifoDepth, stChnAttr.enOutputMode, stChnAttr.stCompressInfo.enCompressMode,
                        stChnAttr.stCompressInfo.u32CompressLevel, stChnAttr.stOutputFrmRate.f32DstFrmRate);

                ret = AX_VDEC_SetChnAttr(vdGrp, vdChn, &stChnAttr);
                if (0 != ret) {
                    LOG_M_E(VDEC, "AX_VDEC_SetChnAttr(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
                    throw 1;
                }

                ret = AX_VDEC_EnableChn(vdGrp, vdChn);
                if (0 != ret) {
                    LOG_M_E(VDEC, "AX_VDEC_EnableChn(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
                    throw 1;
                }
            } else {
                ret = AX_VDEC_DisableChn(vdGrp, vdChn);
                if (0 != ret) {
                    LOG_M_E(VDEC, "AX_VDEC_DisableChn(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
                    throw 1;
                }
            }
        }

    } catch (...) {
        AX_VDEC_DestroyGrp(vdGrp);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVideoDecoder::OnRecvVideoData(AX_S32 nCookie, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) {
    AX_VDEC_GRP vdGrp = nCookie;
#ifdef __DUMP_VDEC_NALU__
    AX_CHAR szFile[64];
    sprintf(szFile, "./vdGrp%d_%08d_len%d.nalu", vdGrp, ++m_arrGrpInfo[vdGrp].nNaluCount, nLen);
    ofstream ofs(szFile, ofstream::out | ofstream::binary | ofstream::trunc);
    if (ofs.is_open()) {
        ofs.write((const char*)pData, nLen);
        ofs.close();
    }
#endif

    if (m_arrCacheBuf[vdGrp]) {
        if (nLen > m_arrCacheBuf[vdGrp]->GetCapacity()) {
            return Send(vdGrp, pData, nLen, nPTS);
        }

        if (m_arrCacheBuf[vdGrp]->Insert(pData, nLen)) {
            return AX_TRUE;
        } else {
            AX_U32 nCacheSize;
            const AX_U8* pCacheBuf = m_arrCacheBuf[vdGrp]->GetCacheBuf(nCacheSize);
            if (!Send(vdGrp, pCacheBuf, nCacheSize, 0)) {
                return AX_FALSE;
            }

            return m_arrCacheBuf[vdGrp]->Insert(pData, nLen);
        }
    } else {
        return Send(vdGrp, pData, nLen, nPTS);
    }
}

AX_BOOL CVideoDecoder::OnRecvAudioData(AX_S32 nCookie, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) {
    return AX_TRUE;
}

AX_BOOL CVideoDecoder::Send(AX_VDEC_GRP vdGrp, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) {
    if (!m_arrGrpInfo[vdGrp].bStarted) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return AX_TRUE;
    }

    AX_VDEC_STREAM_T stStream;
    memset(&stStream, 0, sizeof(stStream));
    stStream.u64PTS = nPTS;
    stStream.pu8Addr = (AX_U8*)pData;
    stStream.u32StreamPackLen = nLen;
    if (AX_VDEC_INPUT_MODE_FRAME == m_arrGrpInfo[vdGrp].stAttr.enInputMode) {
        /* indicate one frame is ended */
        stStream.bEndOfFrame = AX_TRUE;
    }

#if defined(__RECORD_VB_TIMESTAMP__)
    /* private data is used to debug VB */
    AX_U64 nTimestamp;
    stStream.u64PrivateData = m_arrTimestampHelper[vdGrp].MakePrivateData(vdGrp, nTimestamp);
    if (!CHECK_VALID_PRIVATE_FRAME_DATA(stStream.u64PrivateData)) {
        LOG_M_E(VDEC, "[vdGrp %02d, idx %02d] --> private frame id = 0x%llX, timestamp = %lld", vdGrp,
                FRAME_PRIVATE_APPID(stStream.u64PrivateData), stStream.u64PrivateData, nTimestamp);
    }
#endif

    /**
     * @brief  Use VDEC to flow control (timeout: -1)
     *
     *   - file: VDEC is under AX_VDEC_DISPLAY_MODE_PLAYBACK mode:
     *           if out fifo is full, VDEC stop decoding util out fifo has space.
     *           Meanwhile AX_VDEC_SendStream(-1) will block when both in & out fifo are full
     *
     *   - rtsp: VDEC is under AX_VDEC_DISPLAY_MODE_PREVIEW mode:
     *           if out fifo is full, VDEC continue to decode and abandon the new decoded frame
     *           AX_VDEC_SendStream(-1) will block only when in fifo is full
     */

    // LOG_M_W(VDEC, "AX_VDEC_SendStream(vdGrp %d len %d) +++", vdGrp, stStream.u32StreamPackLen);
    AX_S32 ret = AX_VDEC_SendStream(vdGrp, &stStream, -1);
    // LOG_M_W(VDEC, "AX_VDEC_SendStream(vdGrp %d len %d) ---, ret = 0x%x", vdGrp, stStream.u32StreamPackLen, ret);
    if (0 != ret) {
        if (AX_ERR_VDEC_NEED_REALLOC_BUF == ret) {
            /* refer to jira4806:
                  If stream changed, app is in charge of destory and recreate */
            ret = AX_VDEC_StopRecvStream(vdGrp);
            if (0 != ret) {
                LOG_M_E(VDEC, "AX_VDEC_StopRecvStream(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
                return AX_FALSE;
            }

            m_arrGrpInfo[vdGrp].bStarted = AX_FALSE;

            ret = AX_VDEC_ResetGrp(vdGrp);
            if (0 != ret) {
                LOG_M_E(VDEC, "AX_VDEC_ResetGrp(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
                return AX_FALSE;
            }

#if 0
            /* because of private pool, first destory consumer */
            ret = AX_VDEC_DestroyGrp(vdGrp);
            if (0 != ret) {
                LOG_M_E(VDEC, "AX_VDEC_DestroyGrp(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
                return AX_FALSE;
            }

            m_arrGrpInfo[vdGrp].bActive = AX_FALSE;
#endif

            LOG_M_W(VDEC, "stream %d is changed, vdGrp %d is destoryed", vdGrp, vdGrp);

        } else {
            LOG_M_E(VDEC, "AX_VDEC_SendStream(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
        }

        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVideoDecoder::Notify(const CAXFrame& axFrame) {
    std::lock_guard<std::mutex> lck(m_mtxObs);
    if (m_mapObs.end() != m_mapObs.find(axFrame.nGrp)) {
        for (auto& pObs : m_mapObs[axFrame.nGrp]) {
            if (pObs) {
                if (!pObs->OnRecvData(E_OBS_TARGET_TYPE_VDEC, axFrame.nGrp, axFrame.nChn, (AX_VOID*)&axFrame)) {
                    LOG_M_E(VDEC, "%s: OnRecvData(vdGrp %d, vdChn %d) fail", __func__, axFrame.nGrp, axFrame.nChn);
                }
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CVideoDecoder::AttachPool(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, AX_POOL pool) {
    if (AX_INVALID_POOLID == pool) {
        LOG_M_E(VDEC, "AttachPool: invalid pool id %d", pool);
        return AX_FALSE;
    }

    AX_S32 ret = AX_VDEC_AttachPool(vdGrp, vdChn, pool);
    if (0 != ret) {
        LOG_M_E(VDEC, "AX_VDEC_AttachPool(vdGrp %d, vdChn %d) pool %d fail, ret = 0x%x", vdGrp, vdChn, pool, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVideoDecoder::DetachPool(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn) {
    AX_S32 ret = AX_VDEC_DetachPool(vdGrp, vdChn);
    if (0 != ret) {
        LOG_M_E(VDEC, "AX_VDEC_DetachPool(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVideoDecoder::SetChnAttr(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, const AX_VDEC_CHN_ATTR_T& stChnAttr) {
    AX_S32 ret = AX_VDEC_SetChnAttr(vdGrp, vdChn, &stChnAttr);
    if (0 != ret) {
        LOG_M_E(VDEC, "AX_VDEC_SetChnAttr(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}