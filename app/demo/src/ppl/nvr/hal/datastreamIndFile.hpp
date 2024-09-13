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
#include "datastreamfile.hpp"
#include "AppLog.hpp"
#include <vector>
#include <map>

#define AX_DSIF_FILE_MAGIC 0x46495841 // "AXIF" = AXera Index File
#define AX_DSIF_VERSION 0x00000001

typedef enum {
    AXIF_OPEN_FOR_READ = 0,
    AXIF_OPEN_FOR_WRITE,
    AXIF_OPEN_FOR_BOTH
} AXIF_OPEN_FLAG_E;

typedef struct AXIF_FRAME_LOCATION {
    std::string strDataFile; /* 全局定位帧所在的数据文件。# 全局定位指当天特定通道所有视频文件中的定位 */
    AX_S32 nFileIndex {-1}; /* 全局定位帧所在的数据文件的序号 */
    AX_S32 nGlobalFrameIndex {-1}; /* 全局定位帧索引 */
    AX_S32 nFrameIndexWithinFile {-1}; /* 全局定位帧在当前文件中的位置 */
    AX_S32 nFileStartFrmIndex {-1}; /* 当前文件首帧在全局定位中的位置 */
    AX_S32 nFileEndFrmIndex {-1}; /* 当前文件最后一帧在全局定位中的位置 */

    AX_VOID Print() {
        LOG_MM_W("DSIF", "strDataFile=%s(nFileIndex %d, nGlobalFrameIndex %d, nFrameIndexWithinFile %d, nFileStartFrmIndex %d, nFileEndFrmIndex %d)", strDataFile.c_str(), nFileIndex, nGlobalFrameIndex, nFrameIndexWithinFile, nFileStartFrmIndex, nFileEndFrmIndex);
    }
} AXIF_FRAME_LOCATION_T;

typedef struct AXIF_FRAME_RELOCATION_INFO {
    AX_S32 nFileIndex {-1}; /* 全局定位帧所在的数据文件的序号 */
    AX_S32 nFrameIndexWithinFile {-1}; /* 全局定位帧在当前文件中的位置 */

    AX_VOID Print() {
        LOG_MM_W("DSIF", "nFileIndex %d, nFrameIndexWithinFile %d", nFileIndex, nFrameIndexWithinFile);
    }
} AXIF_FRAME_RELOCATION_INFO_T;

/* Index File Header */
typedef struct AXIF_FILE_HEADER {
    AX_U32 uMagic;  // Magic number
    AX_U32 uVersion;  // Version number
    AX_U32 uTotalSize;  //索引文件的总长度
    AX_U32 uFileCount;  //视频文件个数
    AX_U32 uTotalFrameCount; // 视频总帧数
    AX_U32 uTotalIFrameCount; // 视频总I帧数
    AX_U64 uTotalTime; //视频总时长，单位微秒
    AXIF_FILE_HEADER() {
        memset(this, 0, sizeof(AXIF_FILE_HEADER));
        uMagic = AX_DSIF_FILE_MAGIC;
        uVersion = AX_DSIF_VERSION;
    }
} AXIF_FILE_HEADER_T;

/* Index File Info */
typedef struct AXIF_FILE_INFO {
    AX_U32  uSize; // 实际存储的文件信息大小，去掉了pIFrameOffsetStart指针大小，包括了I帧偏移数据
    AX_CHAR szFilePath[128];
    AX_U32 uFileSize;
    AX_U32 uFrameCount;  //帧的总数
    AX_PAYLOAD_TYPE_E uEncodeType;
    AX_U16 uFrameRate;
    AX_U16 uGop;
    AX_U32 uWidth;
    AX_U32 uHeight;
    AXDS_DATETIME_T tStartTime;
    AXDS_DATETIME_T tEndTime;
    AX_U32 uIFrameCount;
    AX_U32* pIFrameOffsetStart;  //记录I帧的偏移，长度不定

    AX_VOID Print() {
        LOG_MM_E("PRINT", "File(%s) => uFrameCount %d", szFilePath, uFrameCount);
    }
} AXIF_FILE_INFO_T;

typedef struct AXIF_FILE_INFO_EX {
    AXIF_FILE_INFO_T tInfo;
    AX_U8* pIFrmOffsetBuf {nullptr};
} AXIF_FILE_INFO_EX_T;

class CDSIFIterator;
class CDSIterator;

class CDataStreamIndFile {
    friend class CDSIFIterator;
    friend class CDSIterator;
public:
    CDataStreamIndFile() = default;

    virtual ~CDataStreamIndFile() {
        DeInit();
    }

    AX_BOOL Init(const AX_CHAR* pFilePath, AXIF_OPEN_FLAG_E eOpenFlag, AX_S32 nDate = 0, AX_S32 nTime = 0, AX_BOOL bGopMode = AX_FALSE);
    AX_BOOL DeInit();

    /* Create or Update data stream file info */
    AX_BOOL CreateAndSave(AXIF_FILE_INFO_T& tIFFileInfo);

    AXIF_FILE_HEADER_T GetFileHeader() { return m_tFileHeader; };
    AX_S32 GetInfoCount() { return m_tFileHeader.uFileCount; };
    static std::string FormatFileName(AX_U8 nStreamID);
    /* In GOP mode, only iterator one GOP, will not switch to next index file */
    AX_BOOL IsGopMode() { return m_bGopMode; };

    AXIF_FILE_INFO_EX_T FindInfo(AX_S32 nInfoIndex, AX_BOOL bFillOffset = AX_FALSE);

    /* Interfaces for iterator file info */
    CDSIFIterator info_begin();
    CDSIFIterator info_end();
    /* Interfaces for iterator file info reversely */
    CDSIFIterator info_rbegin();
    CDSIFIterator info_rend();
    /* Interfaces for iterator frames */
    CDSIterator begin();
    CDSIterator end();
    /* Interfaces for iterator frames reversely */
    CDSIterator rbegin();
    CDSIterator rend();
    /* Interfaces for iterator GOP frames */
    CDSIterator gop_begin(AX_S32 nFileIndex, AX_S32 nGopStartIndex);
    CDSIterator gop_end(AX_S32 nFileIndex, AX_S32 nGopEndIndex);

protected:
    AX_BOOL IsOpened() { return m_hFD != AX_DS_INVALID_HANDLE ? AX_TRUE : AX_FALSE; };
    AX_BOOL Load(AXIF_OPEN_FLAG_E eOpenFlag);
    AX_BOOL OpenFile(const AX_CHAR* pFilePath, AX_BOOL bFileExists, AXIF_OPEN_FLAG_E eOpenFlag);
    AX_VOID CloseFile();
    AX_BOOL WriteFileHeader(AXIF_FILE_HEADER_T& tIFFileHeader);
    AX_BOOL WriteFileInfo(AXIF_FILE_INFO_T& tIFFileInfo);

private:
    /* 通过全局定位帧索引获取数据文件路径以及该帧在当前文件中的索引 */
    AX_BOOL FindFrameLocationByGlobalIndex(AX_S32 nGlobalFrmIndex, AXIF_FRAME_LOCATION_T& tLocation);
    AX_BOOL FindFrameLocationByFrmIndexWithinFile(AX_S32 nFileIndex, AX_S32 nFrmIndex, AXIF_FRAME_LOCATION_T& tLocation);
    /* 通过秒定位数据文件路径以及帧索引 */
    AX_BOOL FindFrameLocationByTime(time_t nTargetSeconds, AXIF_FRAME_LOCATION_T& tLocation, AX_BOOL bIFrameOnly = AX_FALSE, AX_BOOL bReverse = AX_FALSE);

private:
    AX_CHAR m_szFilePath[276];
    AX_S32 m_nStartDate {-1};
    AX_S32 m_nStartTime {-1};
    AX_BOOL m_bGopMode {AX_FALSE};
    AX_S32 m_hFD {AX_DS_INVALID_HANDLE};
    AXIF_FILE_HEADER_T m_tFileHeader;
    AX_U8* m_pIFrmOffsetBuf {nullptr};
};


class CDSIFIterator : public std::iterator<std::random_access_iterator_tag, AXIF_FILE_INFO_T> {
    friend class CDataStreamIndFile;

protected:
    CDataStreamIndFile* m_pDSIF {nullptr};
    AXIF_FILE_INFO_EX_T value;
    AX_S32 m_nInfoIndex {-1};
    AX_S32 m_nInfoCount {-1};
    AX_BOOL m_bReverse {AX_FALSE};
    constexpr static AXIF_FILE_INFO_EX_T END_VALUE {0, {0}, 0, 0, (AX_PAYLOAD_TYPE_E)0, 0, 0, 0, 0, {0, 0}, {0, 0}, 0, nullptr, nullptr};
    constexpr static AX_S32 BEGIN {0};
    constexpr static AX_S32 END {-1};
    constexpr static AX_S32 RBEGIN {0x7FFFFFFF};
    constexpr static AX_S32 REND {-1};

public:
    CDSIFIterator(CDataStreamIndFile* pIFInstance, AX_S32 nInfoIndex) : m_pDSIF(pIFInstance) {
        if (!m_pDSIF || !m_pDSIF->IsOpened()) {
            return;
        }

        AX_U32 nInfoCount = m_pDSIF->GetFileHeader().uFileCount;
        if (BEGIN ==nInfoIndex) {
            m_bReverse = AX_FALSE;
            value = GetValue(0);
            m_nInfoIndex = nInfoIndex;
            m_nInfoCount = nInfoCount;
        } else if (RBEGIN ==nInfoIndex) {
            m_bReverse = AX_TRUE;
            AX_U32 nLastFileIndex = nInfoCount - 1;
            value = GetValue(nLastFileIndex);
            m_nInfoIndex = nLastFileIndex;
            m_nInfoCount = nInfoCount;
        } else {
            value = CDSIFIterator::END_VALUE;
        }
    };

    CDSIFIterator(const CDSIFIterator& it) {
        value = it.value;
        m_nInfoIndex = it.m_nInfoIndex;
        m_nInfoCount = it.m_nInfoCount;
        m_pDSIF = it.m_pDSIF;
        m_bReverse = it.m_bReverse;
    };

    CDSIFIterator() {
        value = CDSIFIterator::END_VALUE;
        m_nInfoIndex = 0;
        m_nInfoCount = 0;
        m_pDSIF = nullptr;
        m_bReverse = AX_FALSE;
    };

    // Assignment operator
    CDSIFIterator& operator=(const CDSIFIterator& src) {
        value = src.value;
        m_nInfoIndex = src.m_nInfoIndex;
        m_nInfoCount = src.m_nInfoCount;
        m_pDSIF = src.m_pDSIF;
        m_bReverse = src.m_bReverse;
        return *this;
    }

    // Dereference an iterator
    AXIF_FILE_INFO_EX_T& operator*() {
        if (-1 == m_nInfoIndex || m_nInfoIndex == m_nInfoCount) {
            throw std::logic_error("Cannot dereference an end iterator.");
        }
        return value;
    }

    // Prefix increment operator
    CDSIFIterator& operator++() {
        if (-1 == m_nInfoIndex || m_nInfoIndex == m_nInfoCount) {
            throw std::logic_error("Cannot increment an end iterator.");
        }

        value = GetValue(Advance());

        return *this;
    }

    // Postfix increment operator
    CDSIFIterator operator++(int) {
        if (-1 == m_nInfoIndex || m_nInfoIndex == m_nInfoCount) {
            throw std::logic_error("Cannot increment an end iterator.");
        }

        auto temp = *this;
        value = GetValue(Advance());
        return temp;
    }

    CDSIFIterator operator+(AX_S32 nDistance) {
        if (-1 == m_nInfoIndex || m_nInfoIndex == m_nInfoCount) {
            throw std::logic_error("Cannot increment an end iterator.");
        }

        while (nDistance-- > 0) {
            (*this)++;
        }

        return *this;
    }

    CDSIFIterator Relocate(AX_S32 nFileIndex) {
        m_nInfoIndex = nFileIndex;
        value = GetValue(m_nInfoIndex);
        return *this;
    }

    AX_S32 Advance() {
        if (m_bReverse) {
            return --m_nInfoIndex;
        } else {
            return ++m_nInfoIndex;
        }
    }

    AXIF_FILE_INFO_EX_T GetValue(AX_S32 nIndex) {
        if (-1 != nIndex) {
            return m_pDSIF->FindInfo(nIndex, m_bReverse ? AX_TRUE : AX_FALSE);
        } else {
            return m_pDSIF->FindInfo(m_nInfoIndex, m_bReverse ? AX_TRUE : AX_FALSE);
        }
    }

    // Comparisons
    bool operator==(const CDSIFIterator& iter)const { return strcmp(value.tInfo.szFilePath, iter.value.tInfo.szFilePath) == 0; }
    bool operator!=(const CDSIFIterator& iter)const { return strcmp(value.tInfo.szFilePath, iter.value.tInfo.szFilePath) != 0; }
    bool operator<(const CDSIFIterator& iter) const { return value.tInfo.tStartTime < iter.value.tInfo.tStartTime; }
    bool operator>(const CDSIFIterator& iter) const { return value.tInfo.tStartTime > iter.value.tInfo.tStartTime; }
    bool operator<=(const CDSIFIterator& iter) const { return value.tInfo.tStartTime <= iter.value.tInfo.tStartTime; }
    bool operator>=(const CDSIFIterator& iter) const { return value.tInfo.tStartTime >= iter.value.tInfo.tStartTime; }
};


class CDSIterator : public std::iterator<std::random_access_iterator_tag, AXDS_FRAME_HEADER_T*> {
    friend class CDataStreamFile;
    friend class CDataStreamIndFile;

protected:
    CDataStreamIndFile* m_pDSIF {nullptr};
    CDataStreamFile* m_pDSF {nullptr};
    CDSIFIterator m_itDSIF;
    CDSIFIterator m_itEndDSIF;
    CDSFIterator m_itDSF;
    AX_BOOL m_bReverse {AX_FALSE};
    AX_BOOL m_bOnlyIFrame {AX_FALSE};
    AXDS_FRAME_HEADER_T* value {nullptr};

    AX_S32 m_nCurrFrameIndex {-1};
    AX_S32 m_nCurrFileIndex {-1};
    AX_S32 m_nTotalFrameCount {-1};
    AX_S32 m_nCurrFileEndInd {-1};

    AXIF_FRAME_RELOCATION_INFO_T m_tRelocateInfo;

    AX_S32 m_nCurrFileIFrameIndex {-1};

    constexpr static AXDS_FRAME_HEADER_T* END_VALUE = nullptr;
    constexpr static AX_S32 BEGIN {0};
    constexpr static AX_S32 END {-1};
    constexpr static AX_S32 RBEGIN {0x7FFFFFFF};
    constexpr static AX_S32 REND {-2};

public:
    CDSIterator(CDataStreamIndFile* pIFInstance, AX_S32 nIndex, AX_BOOL bOnlyIFrame = AX_FALSE) : m_pDSIF(pIFInstance) {
        if (!pIFInstance || !pIFInstance->IsOpened()) {
            value = CDSIterator::END_VALUE;
            return;
        }

        m_bOnlyIFrame = bOnlyIFrame;
        if (BEGIN == nIndex) {
            m_bReverse = AX_FALSE;

            m_nTotalFrameCount = m_pDSIF->GetFileHeader().uTotalFrameCount;
            m_itDSIF = m_pDSIF->info_begin();
            m_itEndDSIF = m_pDSIF->info_end();

            m_pDSF = new CDataStreamFile();
            if (!OpenDataFile((*m_itDSIF).tInfo.szFilePath)) {
                LOG_MM_E("DS_ITER", "Open data file %s failed.", (*m_itDSIF).tInfo.szFilePath);
                value = CDSIterator::END_VALUE;
                return;
            }

            m_itDSF = m_pDSF->frm_begin();
            if (m_itDSF == m_pDSF->frm_end()) {
                value = CDSIterator::END_VALUE;
                return;
            }

            value = *m_itDSF;
            m_nCurrFrameIndex = 0;
            m_nCurrFileIndex = 0;

            AXDS_FILE_HEADER_T tDataFileHeader = m_pDSF->GetFileHeader();
            m_nCurrFileEndInd = tDataFileHeader.uFrameCount - 1;
        } else if (END == nIndex) {
            value = CDSIterator::END_VALUE;
        } else if (RBEGIN == nIndex) {
            m_bReverse = AX_TRUE;

            if (m_bOnlyIFrame) {
                m_nTotalFrameCount = m_pDSIF->GetFileHeader().uTotalIFrameCount;
                m_itDSIF = m_pDSIF->info_rbegin();
                m_itEndDSIF = m_pDSIF->info_rend();
                /* Locate to the first file with frame data (In case space is full, empty file without data would be created) */
                while (m_itDSIF != m_itEndDSIF && (*m_itDSIF).tInfo.uIFrameCount == 0) {
                    m_itDSIF++;
                }

                if (m_itDSIF == m_itEndDSIF) {
                    value = CDSIterator::END_VALUE;
                }

                m_pDSF = new CDataStreamFile();
                if (!OpenDataFile((*m_itDSIF).tInfo.szFilePath)) {
                    LOG_MM_E("DS_RITER", "Open data file %s failed.", (*m_itDSIF).tInfo.szFilePath);
                    value = CDSIterator::END_VALUE;
                    return;
                }

                m_nCurrFrameIndex = 0;
                m_nCurrFileIndex = m_pDSIF->GetFileHeader().uFileCount - 1;
                m_nCurrFileIFrameIndex = (*m_itDSIF).tInfo.uIFrameCount - 1;
                AX_U32 nIFrmOffset = *((AX_U32*)((*m_itDSIF).pIFrmOffsetBuf) + m_nCurrFileIFrameIndex);
                value = m_pDSF->FindFrameByOffset(nIFrmOffset);

                m_nCurrFileEndInd = (*m_itDSIF).tInfo.uIFrameCount - 1;
            } else {
                m_nTotalFrameCount = m_pDSIF->GetFileHeader().uTotalFrameCount;
                m_itDSIF = m_pDSIF->info_rbegin();
                m_itEndDSIF = m_pDSIF->info_rend();
                /* Locate to the first file with frame data (In case space is full, empty file without data would be created) */
                while (m_itDSIF != m_itEndDSIF && (*m_itDSIF).tInfo.uFrameCount == 0) {
                    m_itDSIF++;
                }

                if (m_itDSIF == m_itEndDSIF) {
                    value = CDSIterator::END_VALUE;
                }

                m_pDSF = new CDataStreamFile();
                if (!OpenDataFile((*m_itDSIF).tInfo.szFilePath)) {
                    LOG_MM_E("DS_ITER", "Open data file %s failed.", (*m_itDSIF).tInfo.szFilePath);
                    value = CDSIterator::END_VALUE;
                    return;
                }

                m_itDSF = m_pDSF->frm_rbegin();
                if (m_itDSF == m_pDSF->frm_rend()) {
                    value = CDSIterator::END_VALUE;
                    return;
                }

                value = *m_itDSF;
                m_nCurrFrameIndex = 0;
                m_nCurrFileIndex = m_pDSIF->GetFileHeader().uFileCount - 1;

                AXDS_FILE_HEADER_T tDataFileHeader = m_pDSF->GetFileHeader();
                m_nCurrFileEndInd = tDataFileHeader.uFrameCount - 1;
            }
        } else if (REND == nIndex) {
            value = CDSIterator::END_VALUE;
        }
    };

    CDSIterator(const CDSIterator& it) {
        value = it.value;
        m_nCurrFrameIndex = it.m_nCurrFrameIndex;
        m_nCurrFileIndex = it.m_nCurrFileIndex;
        m_nCurrFileIFrameIndex = it.m_nCurrFileIFrameIndex;
        m_nTotalFrameCount = it.m_nTotalFrameCount;
        m_pDSIF = it.m_pDSIF;
        m_pDSF = it.m_pDSF;
        m_itDSIF = it.m_itDSIF;
        m_itEndDSIF = it.m_itEndDSIF;
        m_itDSF = it.m_itDSF;
        m_nCurrFileEndInd = it.m_nCurrFileEndInd;
        m_tRelocateInfo = it.m_tRelocateInfo;
        m_bReverse = it.m_bReverse;
        m_bOnlyIFrame = it.m_bOnlyIFrame;
    };

    AX_VOID Destroy() {
        if (m_pDSIF) {
            delete m_pDSIF;
            m_pDSIF = nullptr;
        }

        if (m_pDSF) {
            CloseDataFile();

            delete m_pDSF;
            m_pDSF = nullptr;
        }
    }

    // Assignment operator
    CDSIterator& operator=(const CDSIterator& src) {
        value = src.value;
        m_nCurrFrameIndex = src.m_nCurrFrameIndex;
        m_nCurrFileIndex = src.m_nCurrFileIndex;
        m_nCurrFileIFrameIndex = src.m_nCurrFileIFrameIndex;
        m_nTotalFrameCount = src.m_nTotalFrameCount;
        m_pDSIF = src.m_pDSIF;
        m_pDSF = src.m_pDSF;
        m_itDSIF = src.m_itDSIF;
        m_itEndDSIF = src.m_itEndDSIF;
        m_itDSF = src.m_itDSF;
        m_nCurrFileEndInd = src.m_nCurrFileEndInd;
        m_tRelocateInfo = src.m_tRelocateInfo;
        m_bReverse = src.m_bReverse;
        m_bOnlyIFrame = src.m_bOnlyIFrame;

        return *this;
    }

    // Dereference an iterator
    AXDS_FRAME_HEADER_T* operator*() {
        if (IsEnd()) {
            throw std::logic_error("Cannot dereference an end iterator.");
        }
        return value;
    }

    // Prefix increment operator
    CDSIterator& operator++() {
        if (IsEnd()) {
            throw std::logic_error("Cannot increment an end iterator.");
        }

        if (m_nCurrFrameIndex++ == m_nCurrFileEndInd) {
            if (m_pDSIF->IsGopMode()) {
                value = CDSIterator::END_VALUE;
                return *this;
            }

            while (1) { /* Untill find the next data file with frames */
                CloseDataFile();
                m_itDSIF++;
                if (m_bReverse) {
                    m_nCurrFileIndex--;
                } else {
                    m_nCurrFileIndex++;
                }

                if (m_itDSIF == m_itEndDSIF) {
                    value = CDSIterator::END_VALUE;
                    return *this;
                }

                if (!OpenDataFile((*m_itDSIF).tInfo.szFilePath)) {
                    LOG_MM_E("DS_ITER", "Open data file %s failed.", (*m_itDSIF).tInfo.szFilePath);
                    value = CDSIterator::END_VALUE;
                    return *this;
                }

                if (m_pDSF->GetFileHeader().uFrameCount == 0) {
                    continue;
                }

                break;
            }

            if (!m_bReverse) {
                m_itDSF = m_pDSF->frm_begin();
                m_nCurrFileEndInd = m_nCurrFrameIndex + m_pDSF->GetFileHeader().uFrameCount - 1;
            } else {
                m_nCurrFileIFrameIndex = (*m_itDSIF).tInfo.uIFrameCount - 1;
                if (m_bOnlyIFrame) {
                    m_nCurrFileEndInd = m_nCurrFrameIndex + (*m_itDSIF).tInfo.uIFrameCount - 1;
                } else {
                    m_itDSF = m_pDSF->frm_rbegin();
                    m_nCurrFileEndInd = m_nCurrFrameIndex + m_pDSF->GetFileHeader().uFrameCount - 1;
                }
            }

            GetValue();
        } else {
            if (!m_bReverse) {
                m_itDSF++;
            } else {
                if (m_bOnlyIFrame) {
                    m_nCurrFileIFrameIndex--;
                } else {
                    m_itDSF++;
                }
            }

            GetValue();
        }

        return *this;
    }

    // Postfix increment operator
    CDSIterator operator++(int) {
        if (IsEnd()) {
            throw std::logic_error("Cannot increment an end iterator.");
        }

        auto temp = *this;
        if (m_nCurrFrameIndex++ == m_nCurrFileEndInd) {
            while (1) { /* Untill find the next data file with frames */
                CloseDataFile();

                m_itDSIF++;

                if (m_bReverse) {
                    m_nCurrFileIndex--;
                } else {
                    m_nCurrFileIndex++;
                }

                if (m_itDSIF == m_itEndDSIF) {
                    value = CDSIterator::END_VALUE;
                    return *this;
                }

                if (!OpenDataFile((*m_itDSIF).tInfo.szFilePath)) {
                    value = CDSIterator::END_VALUE;
                    return *this;
                }

                if (m_pDSF->GetFileHeader().uFrameCount == 0) {
                    continue;
                }

                break;
            }

            if (!m_bReverse) {
                m_itDSF = m_pDSF->frm_begin();
                m_nCurrFileEndInd = m_nCurrFrameIndex + m_pDSF->GetFileHeader().uFrameCount - 1;
            } else {
                m_nCurrFileIFrameIndex = (*m_itDSIF).tInfo.uIFrameCount - 1;
                if (m_bOnlyIFrame) {
                    m_nCurrFileEndInd = m_nCurrFrameIndex + (*m_itDSIF).tInfo.uIFrameCount - 1;
                } else {
                    m_itDSF = m_pDSF->frm_rbegin();
                    m_nCurrFileEndInd = m_nCurrFrameIndex + m_pDSF->GetFileHeader().uFrameCount - 1;
                }
            }

            GetValue();
        } else {
            if (!m_bReverse) {
                m_itDSF++;
            } else {
                if (m_bOnlyIFrame) {
                    m_nCurrFileIFrameIndex--;
                } else {
                    m_itDSF++;
                }
            }

            GetValue();
        }

        return temp;
    }

    CDSIterator operator+(AX_S32 nDistance) {
        if (IsEnd()) {
            throw std::logic_error("Cannot increment an end iterator.");
        }

        while (nDistance-- > 0) {
            (*this)++;
        }

        return *this;
    }

    AXIF_FRAME_RELOCATION_INFO_T& GetRelocatedInfo() {
        return m_tRelocateInfo;
    }

    CDSIterator Relocate(AXIF_FRAME_LOCATION_T& tLocation) {
        if (strcmp(tLocation.strDataFile.c_str(), m_pDSF->GetFilePath()) != 0) {
            m_itDSIF.Relocate(tLocation.nFileIndex);

            CloseDataFile();
            OpenDataFile(tLocation.strDataFile);

            if (!m_bReverse) {
                /* Use interface FindFrameByOffset to locate frame instead of iterator when play reversely */
                m_itDSF = m_pDSF->frm_begin();
            }
        }

        if (value == CDSIterator::END_VALUE) {
            return *this;
        }

        if (m_bReverse) {
            m_nCurrFrameIndex = tLocation.nGlobalFrameIndex;
            m_nCurrFileEndInd = tLocation.nFileEndFrmIndex;
            m_nCurrFileIFrameIndex = tLocation.nFrameIndexWithinFile;

            AX_U32 nIFrmOffset = *((AX_U32*)((*m_itDSIF).pIFrmOffsetBuf) + m_nCurrFileIFrameIndex);
            value = m_pDSF->FindFrameByOffset(nIFrmOffset);

            m_tRelocateInfo.nFrameIndexWithinFile = m_pDSF->FindFrmIndexByOffset(nIFrmOffset);
            m_tRelocateInfo.nFileIndex = tLocation.nFileIndex;
            m_nCurrFileIndex = tLocation.nFileIndex;
        } else {
            m_nCurrFrameIndex = tLocation.nGlobalFrameIndex;
            m_nCurrFileEndInd = tLocation.nFileEndFrmIndex;
            m_itDSF.Relocate(tLocation.nFrameIndexWithinFile);
            value = *m_itDSF;
        }

        return *this;
    }

    AX_BOOL GetValue() {
        if (m_bReverse) {
            if (m_bOnlyIFrame) {
                AX_U32 nIFrmOffset = *((AX_U32*)((*m_itDSIF).pIFrmOffsetBuf) + m_nCurrFileIFrameIndex);
                value = m_pDSF->FindFrameByOffset(nIFrmOffset);

                m_tRelocateInfo.nFrameIndexWithinFile = m_pDSF->FindFrmIndexByOffset(nIFrmOffset);
                m_tRelocateInfo.nFileIndex = m_nCurrFileIndex;
            } else {
                value = *m_itDSF;
            }
        } else {
            value = *m_itDSF;
        }

        return AX_TRUE;
    }

    AX_VOID Terminate() {
        CloseDataFile();
    }

    AX_BOOL IsEnd() {
        if (-1 == m_nCurrFrameIndex || m_nCurrFrameIndex == m_nTotalFrameCount) {
            return AX_TRUE;
        }

        return AX_FALSE;
    }

    // Comparisons
    bool operator==(const CDSIterator& iter) const { return value == iter.value; }
    bool operator!=(const CDSIterator& iter) const { return value != iter.value; }
    bool operator<(const CDSIterator& iter) const { return value->tTimeStamp < iter.value->tTimeStamp; }
    bool operator>(const CDSIterator& iter) const { return value->tTimeStamp > iter.value->tTimeStamp; }
    bool operator<=(const CDSIterator& iter) const { return value->tTimeStamp <= iter.value->tTimeStamp; }
    bool operator>=(const CDSIterator& iter) const { return value->tTimeStamp >= iter.value->tTimeStamp; }

protected:
    AX_BOOL OpenDataFile(std::string strDataFile) {
        if (!m_pDSF || !m_pDSF->Open(strDataFile.c_str(), AX_DSF_OPEN_FOR_READ)) {
            return AX_FALSE;
        }

        return AX_TRUE;
    }

    AX_VOID CloseDataFile() {
        if (m_pDSF && m_pDSF->IsOpened()) {
            m_pDSF->Close();
        }
    }
};
