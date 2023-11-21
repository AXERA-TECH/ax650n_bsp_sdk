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
#include "haltype.hpp"
#include "AXThread.hpp"
#include "AXEvent.hpp"
#include "istream.hpp"

#define AX_DS_INVALID_HANDLE (-1)
#define MAX_WRITE_CACHE_SIZE (1 * 1024 * 1024) // (4 * 1024 * 1024)  4MB caused THP?
#define MAX_SPS_CACHE_SIZE (1024)
#define MAX_FRAME_BUFF_SIZE (2654280) /* 4096 * 2160 * 3 / 2 * 0.1 * 2 */
#define AX_DSF_FILE_MAGIC 0x53445841 // "AXDS" = AXera Data Stream
#define AX_DSF_FRAME_MAGIC 0x48465841 // "AXFH" = AXera Frame Header
#define AX_DSF_VERSION 0x00000001


typedef enum {
    AXDS_FRM_TYPE_VIDEO_NON = 0x0,
    AXDS_FRM_TYPE_VIDEO_SPS = NALU_TYPE_SPS,
    AXDS_FRM_TYPE_VIDEO_PPS = NALU_TYPE_PPS,
    AXDS_FRM_TYPE_VIDEO_VPS = NALU_TYPE_VPS,
    AXDS_FRM_TYPE_VIDEO_IDR = NALU_TYPE_IDR,
    AXDS_FRM_TYPE_VIDEO_OTHER = NALU_TYPE_OTH, /* P/B frames */
    AXDS_FRM_TYPE_AUDIO = 0x11,
    AXDS_FRM_TYPE_MAX
} AXDS_FRM_TYPE_E;

typedef enum {
    AX_DSF_OPEN_FOR_READ = 0,
    AX_DSF_OPEN_FOR_WRITE,
    AX_DSF_OPEN_FOR_BOTH
} AX_DSF_OPEN_FLAG_E;

typedef struct BUFFER_S {
    AX_U8 *pBuf;
    AX_U32 nCapacity;
    AX_U32 nSize;
    explicit BUFFER_S(AX_U32 nCap = 0x100000) {
        pBuf = (AX_U8 *)malloc(sizeof(AX_U8) * nCap);
        nCapacity = nCap;
        nSize = 0;
    }

    ~BUFFER_S(AX_VOID) {
        free(pBuf);
    }
} AXDS_BUFFER_T, *AXDS_BUFFER_PTR;

typedef struct AXDS_FRAME_OFFSET {
    AX_U32 nCount;
    AX_U32* pOffsetStart;

    AXDS_FRAME_OFFSET() {
        nCount = 0;
        pOffsetStart = nullptr;
    }
} AXDS_FRAME_OFFSET_T;

typedef struct AXDS_DATETIME {
    AX_U32 uSec;
    AX_U32 uUsec;

    AX_VOID Fill() {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uSec = ts.tv_sec;
        uUsec = ts.tv_nsec / 1000000;
    }

    AX_U64 Value() {
        return (AX_U64)uSec * 1000 + uUsec;
    }

    AX_U32 TickFrom(AXDS_DATETIME& tFrom) {
        return (AX_U64)(uSec - tFrom.uSec) * 1000 + (uUsec - tFrom.uUsec);
    }

    bool operator ==(const AXDS_DATETIME& other) const {
        return (AX_U64)uSec * 1000 + uUsec == (AX_U64)other.uSec * 1000 + other.uUsec;
    }
    bool operator !=(const AXDS_DATETIME& other) const {
        return uSec != other.uSec || uUsec != other.uUsec;
    }
    bool operator <(const AXDS_DATETIME& other) const {
        return (AX_U64)uSec * 1000 + uUsec < (AX_U64)other.uSec * 1000 + other.uUsec;
    }
    bool operator <=(const AXDS_DATETIME& other) const {
        return (AX_U64)uSec * 1000 + uUsec <= (AX_U64)other.uSec * 1000 + other.uUsec;
    }
    bool operator >(const AXDS_DATETIME& other) const {
        return (AX_U64)uSec * 1000 + uUsec > (AX_U64)other.uSec * 1000 + other.uUsec;
    }
    bool operator >=(const AXDS_DATETIME& other) const {
        return (AX_U64)uSec * 1000 + uUsec >= (AX_U64)other.uSec * 1000 + other.uUsec;
    }

} AXDS_DATETIME_T;

typedef struct AXDS_VIDEO_INFO {
    AXDS_DATETIME_T tStart;
    AXDS_DATETIME_T tEnd;
    AX_PAYLOAD_TYPE_E uEncodeType {PT_H264};
    AXDS_VIDEO_INFO() {};
    AXDS_VIDEO_INFO(AXDS_DATETIME_T start, AXDS_DATETIME_T end) {
        tStart = start;
        tEnd = end;
    }
} AXDS_VIDEO_INFO_T;

/* File Header */
typedef struct AXDS_FILE_HEADER {
    AX_U32 uMagic;  // Magic number
    AX_U32 uVersion;  // Version number
    AX_U64 uFileSize;  // data文件总大小
    AX_U32 uDataOffset;  // 首个FrameHeader开始位置在文件中偏移
    AX_U32 uTailOffset;  // Tail开始位置在文件中偏移
    AX_U32 uFrameCount;  // 帧的总数
    AX_PAYLOAD_TYPE_E uEncodeType;  // PT_H264, PT_H265
    AX_U16 uFrameRate;
    AX_U16 uGop;
    AX_U16 uWidth;
    AX_U16 uHeight;
    AXDS_DATETIME_T tStartTime;
    AXDS_DATETIME_T tEndTime;

    AXDS_FILE_HEADER () {
        memset(this, 0, sizeof(AXDS_FILE_HEADER));
        uMagic = AX_DSF_FILE_MAGIC;
        uVersion = AX_DSF_VERSION;
        uDataOffset = sizeof(AXDS_FILE_HEADER);
        uTailOffset = sizeof(AXDS_FILE_HEADER);
        uEncodeType = PT_H264;
        uFrameRate = 25;
        uGop = 25;
        uWidth = 1920;
        uHeight = 1080;
    }
} AXDS_FILE_HEADER_T;

/* Frame Header */
typedef struct AXDS_FRAME_HEADER {
    AX_U32 uMagic;  // Magic number
    AX_U32 uHeaderSize;
    AX_U32 uFrameSize; // 帧数据+帧头的总size
    AXDS_DATETIME_T tTimeStamp;
    AX_U8  uFrameType;   // 0x02: 视频SPS帧; 0x03: 视频PPS帧; 0x04: 视频VPS帧; 0x05: 视频I帧；0x06：视频P/B帧；0x11：音频帧
    AX_U8  uFrameSubType; // reserved
    AX_U16 uExtDataLen;   // 扩展信息暂时不用，长度填0
    AX_U8  uExtInfo[8]; //用户存放I帧的扩展信息或者音频帧的扩展信息，暂时不用

    AX_BOOL IsValidFrmType() {
        if (0x2 != uFrameType && 0x3 != uFrameType && 0x4 != uFrameType && 0x5 != uFrameType && 0x6 != uFrameType && 0x11 != uFrameType) {
            return AX_FALSE;
        }

        return AX_TRUE;
    }

    /* 是否为SPS/PPS/VPS? */
    AX_BOOL IsParamSet() {
        return (0x02 == uFrameType || 0x03 == uFrameType || 0x04 == uFrameType) ? AX_TRUE : AX_FALSE;
    }

    STREAM_NALU_TYPE_E GetNaluType() {
        switch (uFrameType) {
            case 0x2: return NALU_TYPE_SPS;
            case 0x3: return NALU_TYPE_PPS;
            case 0x4: return NALU_TYPE_VPS;
            case 0x5: return NALU_TYPE_IDR;
            case 0x6: return NALU_TYPE_OTH;
            default: return NALU_TYPE_NON;
        }
    }
} AXDS_FRAME_HEADER_T;

/* File Tail */
typedef struct AXDS_FILE_TAIL {
    AX_U32 uMagic;  // Magic number
    AX_U32 uFrameCount;  //帧的总数
    AX_U32* pBufFrameOffset; //帧的位置在文件中的偏移

    AXDS_FILE_TAIL() {
        memset(this, 0, sizeof(AXDS_FILE_TAIL));
        uMagic = AX_DSF_FILE_MAGIC;
    }
} AXDS_FILE_TAIL_T;

typedef struct AXDSF_INIT_ATTR {
    AX_U16 uPeriod; // File duration, unit: minute
    AX_PAYLOAD_TYPE_E uEncodeType;
    AX_U16 uFrameRate;
    AX_U16 uGop;
    AX_U16 uWidth;
    AX_U16 uHeight;
} AXDSF_INIT_ATTR_T;

class CDSFIterator;
class CDataStreamFile {
public:
    CDataStreamFile(AXDSF_INIT_ATTR_T& tInitAttr);
    CDataStreamFile();
    virtual ~CDataStreamFile();

    AX_BOOL Init();
    AX_BOOL DeInit();
    AX_S32  Open(const AX_CHAR* pszFile, AX_DSF_OPEN_FLAG_E eOpenFlag = AX_DSF_OPEN_FOR_WRITE);
    AX_BOOL Close();
    AX_BOOL WriteFrame(const AX_VOID* pData, AX_U32 nSize, AXDS_FRAME_HEADER_T& tFrameHeader);
    AX_VOID Flush();
    AX_BOOL IsOpened();
    const AXDS_FILE_HEADER_T& GetFileHeader();
    const AX_CHAR* GetFilePath();
    const AXDS_FRAME_OFFSET_T& GetIFrameOffsetInfo();

    /* Interfaces for iterator frames */
    CDSFIterator frm_begin();
    CDSFIterator frm_end();

    AXDS_FRAME_HEADER_T* FindFrame(AX_S32 nFrmIndex);
    AXDS_FRAME_HEADER_T* FindFrameByOffset(AX_U32 nFrmOffset);
    AX_S32 FindFrmIndexByTime(AX_S32 nSeconds, AX_BOOL bOnlyIFrame = AX_FALSE);

protected:
    AX_VOID SwapBuf();
    AX_BOOL AllocFileTailBuf();
    AX_BOOL FreeFileTailBuf();
    AX_BOOL StartWriteThread();
    AX_BOOL StopWriteThread();
    AX_BOOL WriteFileHeader();
    AX_BOOL UpdateFileHeader();
    AX_BOOL WriteFileTail();
    AX_BOOL FillFrameHeader(AX_U32 nSize, AXDS_FRAME_HEADER_T& tHeader);

    AX_VOID WriteThread(AX_VOID* pArg);

protected:
    AXDS_BUFFER_T m_tBufCache {MAX_WRITE_CACHE_SIZE};
    AXDS_BUFFER_T m_tBufWrite {MAX_WRITE_CACHE_SIZE};
    AXDS_FRAME_OFFSET_T m_tFrameOffsetInfo;
    AXDS_FRAME_OFFSET_T m_tIFrameOffsetInfo;
    AXDS_BUFFER_T m_tSPSBufCache {MAX_SPS_CACHE_SIZE};

    AX_CHAR m_szFilePath[128];
    AXDSF_INIT_ATTR_T m_tInitAttr;
    AXDS_FILE_HEADER_T m_tFileHeader;
    AXDS_FILE_TAIL_T m_tFileTail;

    AX_S32    m_hFD {AX_DS_INVALID_HANDLE};
    CAXThread m_threadWrite;

    CAXEvent  m_hWriteStartEvent;
    CAXEvent  m_hWriteCompleteEvent;

    std::mutex m_mtxWrite;
    std::mutex m_mtxWriting;

    AX_U32    m_nEstimatedMaxFrameCount {0};
    AX_U32    m_nTotalCount {0};
    AX_U32    m_nGOPCalulated {0};
    AX_S32    m_nSPSOffset {-1}; /* if not -1, relocate the next I frame offset here */
    AX_BOOL   m_bClosing {AX_FALSE};
    AX_BOOL   m_bGopRetrieved {AX_FALSE};

    AX_U8 m_arrFrameData[MAX_FRAME_BUFF_SIZE];
};

class CDSFIterator : public std::iterator<std::random_access_iterator_tag, AXDS_FRAME_HEADER_T*> {
    friend class CDataStreamFile;

protected:
    CDataStreamFile* m_pDataFile;
    AXDS_FRAME_HEADER_T* value;
    AX_S32 nCurrFrameIndex;
    AX_S32 nFrameCount;
    constexpr static AXDS_FRAME_HEADER_T* END_VALUE = nullptr;
    constexpr static AX_S32 BEGIN {0};
    constexpr static AX_S32 END {-1};

public:
    CDSFIterator(CDataStreamFile* pDSFInstance, AX_S32 nIndex) : m_pDataFile {pDSFInstance} {
        if (!m_pDataFile || !m_pDataFile->IsOpened()) {
            return;
        }

        value = m_pDataFile->FindFrame(nIndex);
        nCurrFrameIndex = nIndex;
        nFrameCount = m_pDataFile->GetFileHeader().uFrameCount;
    }

    CDSFIterator(const CDSFIterator& it) {
        value = it.value;
        nCurrFrameIndex = it.nCurrFrameIndex;
        nFrameCount = it.nFrameCount;
        m_pDataFile = it.m_pDataFile;
    }

    CDSFIterator() {
        value = END_VALUE;
        nCurrFrameIndex = 0;
        nFrameCount = 0;
        m_pDataFile = nullptr;
    }

    // Assignment operator
    CDSFIterator& operator=(const CDSFIterator& src) {
        value = src.value;
        nCurrFrameIndex = src.nCurrFrameIndex;
        nFrameCount = src.nFrameCount;
        m_pDataFile = src.m_pDataFile;

        return *this;
    }

    // Dereference an iterator
    AXDS_FRAME_HEADER_T* operator*() {
        if (-1 == nCurrFrameIndex || nCurrFrameIndex == nFrameCount) {
            throw std::logic_error("Cannot dereference an end iterator 222.");
        }
        return value;
    }

    // Prefix increment operator
    CDSFIterator& operator++() {
        if (-1 == nCurrFrameIndex || nCurrFrameIndex == nFrameCount) {
            throw std::logic_error("Cannot increment an end iterator.");
        }

        value = m_pDataFile->FindFrame(++nCurrFrameIndex);
        return *this;
    }

    // Postfix increment operator
    CDSFIterator operator++(int) {
        if (-1 == nCurrFrameIndex || nCurrFrameIndex == nFrameCount) {
            throw std::logic_error("Cannot increment an end iterator");
        }
        auto temp = *this;
        value = m_pDataFile->FindFrame(++nCurrFrameIndex);
        return temp;
    }

    CDSFIterator Relocate(AX_S32 nNewFrmIndex) {
        nCurrFrameIndex = nNewFrmIndex;
        value = m_pDataFile->FindFrame(nCurrFrameIndex);
        return *this;
    }

    // Comparisons
    bool operator==(const CDSFIterator& iter) const { return value == iter.value; }
    bool operator!=(const CDSFIterator& iter) const { return value != iter.value; }
    bool operator<(const CDSFIterator& iter) const { return value->tTimeStamp < iter.value->tTimeStamp; }
    bool operator>(const CDSFIterator& iter) const { return value->tTimeStamp > iter.value->tTimeStamp; }
    bool operator<=(const CDSFIterator& iter) const { return value->tTimeStamp <= iter.value->tTimeStamp; }
    bool operator>=(const CDSFIterator& iter) const { return value->tTimeStamp >= iter.value->tTimeStamp; }
};
