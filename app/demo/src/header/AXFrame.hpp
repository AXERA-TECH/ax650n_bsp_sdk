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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <AXException.hpp>
#include <fstream>
#include <mutex>
#include <string>
#include "AppLogApi.h"
#if defined(__RECORD_VB_TIMESTAMP__)
#include "TimestampHelper.hpp"
#endif
#include "ax_sys_api.h"

typedef enum axAPP_FRAME_TYPE {
    AX_APP_FRAME_TYPE_VIDEO,
    AX_APP_FRAME_TYPE_AUDIO,
    AX_APP_FRAME_TYPE_BUTT
} AX_APP_FRAME_TYPE_E;

class CAXFrame;
class IFrameRelease {
public:
    virtual ~IFrameRelease(AX_VOID) = default;
    virtual AX_VOID VideoFrameRelease(CAXFrame *pFrame) {};
    virtual AX_VOID AudioFrameRelease(CAXFrame *pFrame) {};
};

class CAXFrame final {
public:
    AX_S32 nGrp{-1};
    AX_S32 nChn{-1};
    AX_APP_FRAME_TYPE_E eFrameType{AX_APP_FRAME_TYPE_VIDEO};
    union {
        AX_VIDEO_FRAME_INFO_T stVFrame;
        AX_AUDIO_FRAME_INFO_T stAFrame;
    } stFrame;
    IFrameRelease *pFrameRelease{nullptr};
    AX_VOID *pUserDefine{nullptr};
    AX_BOOL bMultiplex{AX_FALSE};
    AX_U32 nFrmRefCnt{0};
    std::mutex mtxFrmRefCnt;

    CAXFrame(AX_VOID) {
        memset(&stFrame, 0, sizeof(stFrame));
    }
    ~CAXFrame(AX_VOID) = default;

private:
    AX_VOID __DeepCopy(const CAXFrame &rhs) noexcept {
        nGrp = rhs.nGrp;
        nChn = rhs.nChn;
        stFrame = rhs.stFrame;
        pFrameRelease = rhs.pFrameRelease;
        pUserDefine = rhs.pUserDefine;
        bMultiplex = rhs.bMultiplex;
        nFrmRefCnt = rhs.nFrmRefCnt;
    }

public:
    CAXFrame(const CAXFrame &rhs) {
        __DeepCopy(rhs);
    }

    CAXFrame &operator=(const CAXFrame &rhs) {
        if (this != &rhs) {
            __DeepCopy(rhs);
        }

        return *this;
    }

    /* c++11 rule of five: to delcare move statements obviously */
    CAXFrame(CAXFrame &&rhs) = default;
    CAXFrame &operator=(CAXFrame &&rhs) = default;

    AX_VOID FreeMem(AX_VOID) {
        if (pFrameRelease) {
            if (AX_APP_FRAME_TYPE_AUDIO == eFrameType) {
                pFrameRelease->AudioFrameRelease(this);
            }
            else {
                pFrameRelease->VideoFrameRelease(this);
            }
        }
    }

    AX_VOID IncRef(AX_VOID) const {
        if (AX_APP_FRAME_TYPE_AUDIO == eFrameType) {
            auto &m = stFrame.stAFrame.stAFrame.u32BlkId;
            if (AX_INVALID_BLOCKID != m) {
                AX_S32 ret = AX_POOL_IncreaseRefCnt(m);
                if (0 != ret) {
                    THROW_AX_EXCEPTION("AX_POOL_IncreaseRefCnt(blk 0x%x) fail, ret = 0x%x", m, ret);
                }
            }
        }
        else {
            for (auto &m : stFrame.stVFrame.stVFrame.u32BlkId) {
                if (AX_INVALID_BLOCKID != m) {
                    AX_S32 ret = AX_POOL_IncreaseRefCnt(m);
                    if (0 != ret) {
                        THROW_AX_EXCEPTION("AX_POOL_IncreaseRefCnt(blk 0x%x) fail, ret = 0x%x", m, ret);
                    }
                }
            }
        }
    }

    AX_VOID DecRef(AX_VOID) const {
        if (AX_APP_FRAME_TYPE_AUDIO == eFrameType) {
            auto &m = stFrame.stAFrame.stAFrame.u32BlkId;
            if (AX_INVALID_BLOCKID != m) {
                AX_S32 ret = AX_POOL_DecreaseRefCnt(m);
                if (0 != ret) {
                    THROW_AX_EXCEPTION("AX_POOL_DecreaseRefCnt(blk 0x%x) fail, ret = 0x%x", m, ret);
                }
            }
        }
        else {
            for (auto &m : stFrame.stVFrame.stVFrame.u32BlkId) {
                if (AX_INVALID_BLOCKID != m) {
#if defined(__RECORD_VB_TIMESTAMP__)
                    CTimestampHelper::RecordTimestamp(stFrame.stVFrame.stVFrame, nGrp, nChn, TIMESTAMP_VB_RELEASE);
#endif

                    AX_S32 ret = AX_POOL_DecreaseRefCnt(m);
                    if (0 != ret) {
                        THROW_AX_EXCEPTION("AX_POOL_DecreaseRefCnt(blk 0x%x) fail, ret = 0x%x", m, ret);
                    }
                }
            }
        }
    }

    AX_U32 IncFrmRef(AX_VOID) {
        std::lock_guard<std::mutex> lck(mtxFrmRefCnt);
        nFrmRefCnt++;
        return nFrmRefCnt;
    }

    AX_U32 DecFrmRef(AX_VOID) {
        std::lock_guard<std::mutex> lck(mtxFrmRefCnt);
        if (nFrmRefCnt > 0) {
            nFrmRefCnt--;
        }

        return nFrmRefCnt;
    }

    AX_U32 GetFrameSize(AX_VOID) const {
        if (AX_APP_FRAME_TYPE_AUDIO == eFrameType) {
            return stFrame.stAFrame.stAFrame.u32Len;
        }
        else {
            AX_U32 nBpp = 0;

            AX_U32 nStride = stFrame.stVFrame.stVFrame.u32PicStride[0];
            if (0 == nStride) {
                nStride = stFrame.stVFrame.stVFrame.u32Width;
            }
            switch (stFrame.stVFrame.stVFrame.enImgFormat) {
                case AX_FORMAT_YUV420_PLANAR:
                case AX_FORMAT_YUV420_PLANAR_VU:
                case AX_FORMAT_YUV420_SEMIPLANAR:
                case AX_FORMAT_YUV420_SEMIPLANAR_VU:
                    nBpp = 12;
                    break;
                case AX_FORMAT_YUV422_INTERLEAVED_YUVY:
                case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
                case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
                case AX_FORMAT_YUV422_INTERLEAVED_VYUY:
                case AX_FORMAT_ARGB1555:
                    nBpp = 16;
                    break;
                case AX_FORMAT_RGB888:
                case AX_FORMAT_BGR888:
                    nBpp = 24;
                    break;
                case AX_FORMAT_ARGB8888:
                    nBpp = 32;
                    break;
                default:
                    nBpp = 0;
                    break;
            }

            return nStride * stFrame.stVFrame.stVFrame.u32Height * nBpp / 8;
        }
    }

    AX_BOOL LoadFile(const char *pFile) {
        if (!pFile) {
            return AX_FALSE;
        }

        if (0 == stFrame.stVFrame.stVFrame.u32Width || 0 == stFrame.stVFrame.stVFrame.u32Height || 0 == stFrame.stVFrame.stVFrame.u32FrameSize ||
            0 == stFrame.stVFrame.stVFrame.u64VirAddr[0]) {
            return AX_FALSE;
        }

        std::ifstream ifs(pFile, std::ifstream::in | std::ifstream::binary);
        if (!ifs) {
            return AX_FALSE;
        }

        ifs.seekg(0, ifs.end);
        AX_U32 nFileSize = ifs.tellg();
        ifs.seekg(0, ifs.beg);

        if (nFileSize != stFrame.stVFrame.stVFrame.u32FrameSize) {
            ifs.close();
            return AX_FALSE;
        }

        ifs.read((char *)stFrame.stVFrame.stVFrame.u64VirAddr[0], nFileSize);
        ifs.close();

        return AX_TRUE;
    }

    /**
     * @brief dump image
     *
     * @param pPath:
     *           - if pPath is directory path, image will be dumped as file name: dump_1920x1080_frame1.img
     *           - if pPath is file path, image will be dumped as file path specified
     * @return AX_BOOL
     */
    AX_BOOL SaveFile(const char *pPath) const {
        if (!pPath) {
            return AX_FALSE;
        }

        if (0 == stFrame.stVFrame.stVFrame.u32Width || 0 == stFrame.stVFrame.stVFrame.u32Height || 0 == stFrame.stVFrame.stVFrame.u64VirAddr[0]) {
            return AX_FALSE;
        }

        AX_U32 nFrameSize = stFrame.stVFrame.stVFrame.u32FrameSize;
        if (0 == nFrameSize) {
            nFrameSize = GetFrameSize();
        }

        std::string strPath{pPath};
        struct stat st;
        if (0 == ::stat(pPath, &st) && S_ISDIR(st.st_mode)) {
            AX_CHAR szName[64];
            sprintf(szName, "dump_%dx%d_grp%d_chn%d_frame%lld.img", stFrame.stVFrame.stVFrame.u32PicStride[0], stFrame.stVFrame.stVFrame.u32Height, nGrp,
                    nChn, stFrame.stVFrame.stVFrame.u64SeqNum);
            if (strPath[strPath.length() - 1] != '/') {
                strPath += "/";
            }

            strPath += szName;
        }

        std::ofstream ofs(strPath.c_str(), std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
        if (!ofs) {
            return AX_FALSE;
        }

        ofs.write((const char *)stFrame.stVFrame.stVFrame.u64VirAddr[0], nFrameSize);
        ofs.close();

        return AX_TRUE;
    }
};
