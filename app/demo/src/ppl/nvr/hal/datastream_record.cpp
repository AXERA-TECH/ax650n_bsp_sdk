/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "datastream_record.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "NVRConfigParser.h"
#include "fs.hpp"
#include <fcntl.h>
#include <unistd.h>

#define TAG "DATA_STREAM_R"
#define DS_CHN_RESERVE_SPACE_THRESHOLD (0.73) /* gop * avg p frame size = 25 * 30 / 1024 */
#define DS_MAX_SUPPORTED_DEVICE_COUNT (128)
#define DS_MAX_SUPPORTED_STREAM_COUNT (2)

using namespace std;


AX_BOOL CDataStreamRecord::Init(const AXDS_RECORD_INIT_ATTR_T& tAttr) {
    if (!CheckInitAttr(tAttr)) {
        return AX_FALSE;
    }

    m_tInitAttr = tAttr;

    LOG_M_I(TAG, "Input Parameter:");
    LOG_M_I(TAG, "  Save path: %s", m_tInitAttr.szParentDir);
    LOG_M_I(TAG, "  Max device count: %d", m_tInitAttr.uMaxDevCnt);
    LOG_M_I(TAG, "  Sub stream count: %d", m_tInitAttr.uStreamCnt);
    LOG_M_I(TAG, "  Max device space: %d", m_tInitAttr.uMaxDevSpace);
    LOG_M_I(TAG, "  Max file period: %d", m_tInitAttr.uMaxFilePeriod);
    LOG_M_I(TAG, "  Generate index file on close: %s", m_tInitAttr.bGenIFOnClose ? "YES" : "NO");

    if (!Prepare()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::DeInit() {
    if (!m_tConfig.bSaveDisk) {
        if (m_threadStreamMonitor.IsRunning()) {
            m_threadStreamMonitor.Stop();
            m_threadStreamMonitor.Join();
        }
    }

    StopAll();

    if (m_tConfig.bSaveDisk) {
        AX_U8 nMaxChn = m_tInitAttr.uMaxDevCnt;
        for (AX_U8 i = 0; i < nMaxChn; i++) {
            for (AX_U8 j = 0; j < m_tInitAttr.uStreamCnt; j++) {
                if (m_mapDev2Mutex[i][j]) {
                    delete m_mapDev2Mutex[i][j];
                    m_mapDev2Mutex[i][j] = nullptr;
                }
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::Start(AX_U8 nDeviceID, AX_U8 nStreamID, AXDSF_INIT_ATTR_T& tInitAttr) {
    LOG_MM_I(TAG, "[%d][%d] +++", nDeviceID, nStreamID);

    if (!CheckStream(nDeviceID, nStreamID)) {
        LOG_M_E(TAG, "Can not open device stream(%d, %d).", nDeviceID, nStreamID);
        return AX_FALSE;
    }

    if (m_tConfig.bSaveDisk) {
        m_mapDev2DSF[nDeviceID][nStreamID] = new CDataStreamFile(tInitAttr);
        if (m_tInitAttr.bGenIFOnClose) {
            /* Create datastream index file instance */
            m_mapDev2DSIF[nDeviceID][nStreamID] = new CDataStreamIndFile();

            /* If index file exists, open file and load header info, if not, initailize header info */
            AX_CHAR szFilePath[300] = {0};
            AX_CHAR szDateBuf[16] = {0};
            CElapsedTimer::GetLocalDate(szDateBuf, 16, '-');
            sprintf(szFilePath, "%s/%s/%s", m_mapDev2Dir[nDeviceID].c_str(), szDateBuf, CDataStreamIndFile::FormatFileName(nStreamID).c_str());
            m_mapDev2DSIF[nDeviceID][nStreamID]->Init(szFilePath, AXIF_OPEN_FOR_WRITE);
        }

        m_mapDev2FileInfo[nDeviceID][nStreamID].nFD = CreateDataFile(nDeviceID, nStreamID);
        if (-1 == m_mapDev2FileInfo[nDeviceID][nStreamID].nFD) {
            LOG_M_E(TAG, "[%d][%d] Create data file failed.", nDeviceID, nStreamID);
        } else {
            LOG_MM_I(TAG, "[%d][%d] Create data file successfully.", nDeviceID, nStreamID);

            m_mapDev2FileInfo[nDeviceID][nStreamID].nStartSecond = CElapsedTimer::GetTickCount() / 1000;
            m_mapDev2FileInfo[nDeviceID][nStreamID].nDay = CElapsedTimer::GetCurrDay();
        }

        m_mapDev2Status[nDeviceID][nStreamID] = AX_DS_STREAM_STATUS_STARTED;
    } else {
        m_mapDev2MonitorInfo[nDeviceID][nStreamID].nDevice = nDeviceID;
        m_mapDev2MonitorInfo[nDeviceID][nStreamID].nStream = nStreamID;
        m_mapDev2MonitorInfo[nDeviceID][nStreamID].eStatus = AX_DS_STREAM_STATUS_STARTED;
    }

    LOG_MM_I(TAG, "[%d][%d] ---", nDeviceID, nStreamID);
    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::Stop(AX_U8 nDeviceID, AX_U8 nStreamID) {
    if (!CheckStream(nDeviceID, nStreamID, m_tConfig.bSaveDisk ? AX_TRUE : AX_FALSE)) {
        return AX_FALSE;
    }

    if (m_tConfig.bSaveDisk) {
        CloseDataFile(nDeviceID, nStreamID);

        if (m_mapDev2DSF[nDeviceID][nStreamID]) {
            delete m_mapDev2DSF[nDeviceID][nStreamID];
            m_mapDev2DSF[nDeviceID][nStreamID] = nullptr;
        }

        if (m_mapDev2DSIF[nDeviceID][nStreamID]) {
            m_mapDev2DSIF[nDeviceID][nStreamID]->DeInit();
            delete m_mapDev2DSIF[nDeviceID][nStreamID];
            m_mapDev2DSIF[nDeviceID][nStreamID] = nullptr;
        }

        m_mapDev2Status[nDeviceID][nStreamID] = AX_DS_STREAM_STATUS_STOPPED;
    } else {
        m_mapDev2MonitorInfo[nDeviceID][nStreamID].eStatus = AX_DS_STREAM_STATUS_STOPPED;
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::StopAll() {
    for (AX_U8 nDeviceID = 0; nDeviceID < m_tInitAttr.uMaxDevCnt; ++nDeviceID) {
        for (AX_U8 nStreamID = 0; nStreamID < m_tInitAttr.uStreamCnt; ++nStreamID) {
            Stop(nDeviceID, nStreamID);
        }
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::Save(AX_U8 nDeviceID, AX_U8 nStreamID, AXDS_FRAME_HEADER_T& tFrameHeader, const AX_U8* pData, AX_U32 nSize) {
    if (!CheckStream(nDeviceID, nStreamID, m_tConfig.bSaveDisk ? AX_TRUE : AX_FALSE)) {
        if (AX_DS_STREAM_STATUS_STARTED == m_mapDev2Status[nDeviceID][nStreamID]) {
            LOG_MM_E(TAG, "Device stream(%d, %d) is not opened.", nDeviceID, nStreamID);
        }

        return AX_FALSE;
    }

    if (nullptr == pData || 0 == nSize) {
        LOG_MM_E(TAG, "Invalid data: data=%p, size=%d", pData, nSize);
        return AX_FALSE;
    }

    if (!tFrameHeader.IsValidFrmType()) {
        return AX_FALSE;
    }

    if (m_tConfig.bSaveDisk) {
        if (m_mapDev2DiskFullFlg[nDeviceID]) {
            return AX_FALSE;
        }

        std::lock_guard<std::mutex> lck(*m_mapDev2Mutex[nDeviceID][nStreamID]);

        m_nLastFrameType = tFrameHeader.uFrameType;

        /* Ensure that the frame header and data would be within the same file */
        AX_U32 nRealWriteSize = nSize + sizeof(AXDS_FRAME_HEADER_T);
        SwitchDestination(nDeviceID, nStreamID, nRealWriteSize, tFrameHeader.uFrameType);

        AX_S32 hFD = m_mapDev2FileInfo[nDeviceID][nStreamID].nFD;
        if (AX_DS_INVALID_HANDLE == hFD) {
            LOG_MM_E(TAG, "[%d][%d] Data file handle is invalid.", nDeviceID, nStreamID);
            return AX_FALSE;
        }

        if (!m_mapDev2DiskFullFlg[nDeviceID]) {
            if (!m_mapDev2DSF[nDeviceID][nStreamID]->WriteFrame(pData, nSize, tFrameHeader)) {
                LOG_MM_E(TAG, "[%d][%d] Save data failed.", nDeviceID, nStreamID);
                return AX_FALSE;
            }
        }

        m_mapDev2TotalSize[nDeviceID] += nRealWriteSize;
    } else {
        m_mapDev2MonitorInfo[nDeviceID][nStreamID].nSize += nSize;
        if (AXDS_FRM_TYPE_VIDEO_IDR == tFrameHeader.uFrameType
            || AXDS_FRM_TYPE_VIDEO_OTHER == tFrameHeader.uFrameType) {
            m_mapDev2MonitorInfo[nDeviceID][nStreamID].nSeq += 1;
        }

    }

    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::CheckInitAttr(const AXDS_RECORD_INIT_ATTR_T& tAttr) {
    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::Prepare() {
    m_tConfig = CNVRConfigParser::GetInstance()->GetDataStreamConfig();
    if (m_tConfig.bSaveDisk) {
        /* Create directory of device */
        AX_CHAR szChnPath[300] = {0};
        AX_U8 nMaxDevCnt = m_tInitAttr.uMaxDevCnt;
        for (AX_U8 i = 0; i < nMaxDevCnt; i++) {
            sprintf(szChnPath, "%s/DEV_%02d", m_tInitAttr.szParentDir[0], i + 1);
            if (!CDiskHelper::CreateDir(szChnPath, AX_FALSE)) {
                return AX_FALSE;
            } else {
                LOG_MM_I(TAG, "Directory %s created.", szChnPath);

                m_mapDev2Dir[i] = string(szChnPath);
                m_mapDev2FileInfo[i].resize(m_tInitAttr.uStreamCnt);
                m_mapDev2Status[i].resize(m_tInitAttr.uStreamCnt, AX_DS_STREAM_STATUS_INIT);
                m_mapDev2DSF[i].resize(m_tInitAttr.uStreamCnt);
                m_mapDev2DSIF[i].resize(m_tInitAttr.uStreamCnt);
                m_mapDev2DiskFullFlg[i] = AX_FALSE;
                for (AX_U8 j = 0; j < m_tInitAttr.uStreamCnt; j++) {
                    m_mapDev2Mutex[i].push_back(new std::mutex());
                }
            }
        }

        /* Calculate existing device stream consumed spaces */
        for (AX_U8 i = 0; i < nMaxDevCnt; i++) {
            std::deque<DISK_FILE_INFO_T> listAllDirs = CDiskHelper::TraverseDirs(m_mapDev2Dir[i].c_str());
            for (AX_U8 j = 0; j < listAllDirs.size(); j++) {
                m_mapDev2TotalSize[i] += CDiskHelper::GetDirSize(listAllDirs.at(j).path.c_str());
            }
        }
    } else {
        AX_U8 nMaxDevCnt = m_tInitAttr.uMaxDevCnt;
        for (AX_U8 i = 0; i < nMaxDevCnt; i++) {
            m_mapDev2MonitorInfo[i].resize(m_tInitAttr.uStreamCnt);
        }

        if (!m_threadStreamMonitor.Start([this](AX_VOID *pArg) -> AX_VOID { ThreadStreamMonitor(pArg); }, this, "DS_R_MONITOR")) {
            LOG_MM_E(TAG, "Create stream monitor thread fail");
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_VOID CDataStreamRecord::SwitchDestination(AX_U8 nDeviceID, AX_U8 nStreamID, AX_U32 nSize, AX_U8 nFrameType) {
    /* Switch for certain type of frames */
    if (!IsTypeForSwitch(nFrameType)) {
        LOG_MM_D(TAG, "Ignore none SPS/PPS/VPS/I frames.");
        return;
    }

    /* Switch once reaches max size of device consumable spaces */
    AX_U64 nChnTotalBytes = m_mapDev2TotalSize[nDeviceID] + nSize;
    if (((nChnTotalBytes + nSize) / 1048576.0) >= (m_tInitAttr.uMaxDevSpace - DS_CHN_RESERVE_SPACE_THRESHOLD)) {
        QFutureWatcher<AX_BOOL> watcherRemoveThread;
        QFutureInterface<AX_BOOL> interface;
        watcherRemoveThread.setFuture(interface.future());
        interface.reportStarted();

        REMOVE_THREAD_PARAM_T tRemoveParams = {nDeviceID, nStreamID, 1};
        std::thread thread([this, interface, tRemoveParams]() mutable {
            AX_U8 nDeviceID = tRemoveParams.nDeviceID;
            AX_U8 nRemoveDays = tRemoveParams.nRemoveDays;

            std::deque<DISK_FILE_INFO_T> listAllDirs = CDiskHelper::TraverseDirs(m_mapDev2Dir[nDeviceID].c_str());
            if (listAllDirs.size() == 0) {
                LOG_MM_E(TAG, "No directory under %s exists, quit thread.", m_mapDev2Dir[nDeviceID].c_str());

                if (interface.isRunning()) {
                    interface.reportFinished();
                }
                return;
            }

            if (listAllDirs.size() == 1) {
                LOG_MM_W(TAG, "[%d] Only one directory exists under %s and could not be removed.", nDeviceID, m_mapDev2Dir[nDeviceID].c_str());
                m_mapDev2DiskFullFlg[nDeviceID] = AX_TRUE;

                if (interface.isRunning()) {
                    interface.reportFinished();
                }
            } else {
                /* Removing old data won't bother frame saving, make watcher finished and continue saving frames immediately */
                if (interface.isRunning()) {
                    interface.reportFinished();
                }

                for (AX_U8 i = 0; i < nRemoveDays; i++) {
                    string strDir = listAllDirs.at(i).path;

                    AX_U64 nRemovedSize = 0;
                    std::deque<DISK_FILE_INFO_T> listAllFiles = CDiskHelper::TraverseFiles(strDir.c_str());
                    for (auto &m : listAllFiles) {
                        nRemovedSize += m.size;
                    }

                    CDiskHelper::RemoveDir(strDir.c_str());
                    m_mapDev2TotalSize[nDeviceID] -= nRemovedSize;
                    LOG_M_C(TAG, "[%d] Delete obsoleted directory %s, extra %d MB can be reused.", nDeviceID, strDir.c_str(), nRemovedSize >> 20);
                }
                LOG_MM_I(TAG, "[%d] Old directories for %d days(s) removed, quit thread.", nDeviceID, nRemoveDays);
            }
        });

        thread.detach();

        /* Waiting thread to determine whether could start saving frames */
        watcherRemoveThread.waitForFinished();

        m_mapDev2FileInfo[nDeviceID][nStreamID].nFD = SwitchFile(nDeviceID, nStreamID);
        if (AX_DS_INVALID_HANDLE == m_mapDev2FileInfo[nDeviceID][nStreamID].nFD) {
            LOG_MM_E(TAG, "[%d][%d] Create data file for duration limit failed.", nDeviceID, nStreamID);
            return;
        } else {
            LOG_MM_I(TAG, "[%d][%d] Create data file for duration limit successfully.", nDeviceID, nStreamID);
            m_mapDev2FileInfo[nDeviceID][nStreamID].nStartSecond = CElapsedTimer::GetTickCount() / 1000;
            m_mapDev2FileInfo[nDeviceID][nStreamID].nDay = CElapsedTimer::GetCurrDay();
        }
    }

    /* Create file on app started */
    if (AX_DS_INVALID_HANDLE == m_mapDev2FileInfo[nDeviceID][nStreamID].nFD) {
        m_mapDev2FileInfo[nDeviceID][nStreamID].nFD = CreateDataFile(nDeviceID, nStreamID);
        if (AX_DS_INVALID_HANDLE == m_mapDev2FileInfo[nDeviceID][nStreamID].nFD) {
            LOG_MM_E(TAG, "[%d][%d] Create data file failed.", nDeviceID, nStreamID);
            return;
        } else {
            LOG_MM_I(TAG, "[%d][%d] Create data file successfully.");
            m_mapDev2FileInfo[nDeviceID][nStreamID].nStartSecond = CElapsedTimer::GetTickCount() / 1000;
            m_mapDev2FileInfo[nDeviceID][nStreamID].nDay = CElapsedTimer::GetCurrDay();
        }
    }

    /* Switch for a new day */
    if (CElapsedTimer::GetCurrDay() != m_mapDev2FileInfo[nDeviceID][nStreamID].nDay) {
        m_mapDev2FileInfo[nDeviceID][nStreamID].nFD = SwitchFile(nDeviceID, nStreamID);
        if (AX_DS_INVALID_HANDLE == m_mapDev2FileInfo[nDeviceID][nStreamID].nFD) {
            LOG_MM_E(TAG, "[%d][%d] Create data file for new day comes failed.", nDeviceID, nStreamID);
            return;
        } else {
            LOG_MM_I(TAG, "[%d][%d] Create data file for new day comes successfully.");
            m_mapDev2FileInfo[nDeviceID][nStreamID].nStartSecond = CElapsedTimer::GetTickCount() / 1000;
            m_mapDev2FileInfo[nDeviceID][nStreamID].nDay = CElapsedTimer::GetCurrDay();
        }
    }

    /* Switch for duration limit */
    if (CElapsedTimer::GetTickCount() / 1000 > m_mapDev2FileInfo[nDeviceID][nStreamID].nStartSecond + m_tInitAttr.uMaxFilePeriod * 60) {
        LOG_M_C(TAG, "[%d][%d] Start switch for device stream duration limit.", nDeviceID, nStreamID);
        m_mapDev2FileInfo[nDeviceID][nStreamID].nFD = SwitchFile(nDeviceID, nStreamID);
        if (AX_DS_INVALID_HANDLE == m_mapDev2FileInfo[nDeviceID][nStreamID].nFD) {
            LOG_MM_E(TAG, "[%d][%d] Create data file for duration limit failed.", nDeviceID, nStreamID);
            return;
        } else {
            LOG_MM_I(TAG, "[%d][%d] Create data file for duration limit successfully.", nDeviceID, nStreamID);
            m_mapDev2FileInfo[nDeviceID][nStreamID].nStartSecond = CElapsedTimer::GetTickCount() / 1000;
            m_mapDev2FileInfo[nDeviceID][nStreamID].nDay = CElapsedTimer::GetCurrDay();
        }
    }
}

AX_S32 CDataStreamRecord::CreateDataFile(AX_U8 nDeviceID, AX_U8 nStreamID) {
    /* Data file parent directory format: </XXX/DEV_XX/YYYY-MM-DD> */
    AX_CHAR szDateDir[300] = {0};
    AX_CHAR szDateBuf[16] = {0};
    CElapsedTimer::GetLocalDate(szDateBuf, 16, '-');
    sprintf(szDateDir, "%s/DEV_%02d/%s", m_tInitAttr.szParentDir[0], nDeviceID + 1, szDateBuf);

    if (CDiskHelper::CreateDir(szDateDir, AX_FALSE)) {
        LOG_MM_I(TAG, "[%d][%d] Create date(%s) directory successfully.", nDeviceID, nStreamID, szDateDir);

        AX_CHAR szFilePath[300] = {0};
        AX_CHAR szTimeBuf[16] = {0};
        CElapsedTimer::GetLocalTime(szTimeBuf, 16, '-', AX_FALSE);
        sprintf(szFilePath, "%s/%s/%s%s_%s.dat", m_mapDev2Dir[nDeviceID].c_str(), szDateBuf, 0 == nStreamID ? "main" : "sub", (0 == nStreamID ? "" : to_string(nStreamID).c_str()), szTimeBuf);

        AX_S32 nFD = m_mapDev2DSF[nDeviceID][nStreamID]->Open(szFilePath);
        if (AX_DS_INVALID_HANDLE == nFD) {
            LOG_MM_E(TAG, "[%d][%d] Open data file: %s failed.", nDeviceID, nStreamID, szFilePath);
        } else {
            LOG_MM_I(TAG, "[%d][%d] Open data file: %s successfully.", nDeviceID, nStreamID, szFilePath);
        }

        return nFD;
    } else {
        LOG_MM_E(TAG, "[%d][%d] Create date(%s) directory failed.", nDeviceID, nStreamID, szDateDir);
    }

    return AX_DS_INVALID_HANDLE;
}

AX_BOOL CDataStreamRecord::CloseDataFile(AX_U8 nDeviceID, AX_U8 nStreamID) {
    AX_BOOL bRet = m_mapDev2DSF[nDeviceID][nStreamID]->Close();

    if (m_tInitAttr.bGenIFOnClose) {
        AXDS_FILE_HEADER_T tDSHeader = m_mapDev2DSF[nDeviceID][nStreamID]->GetFileHeader();
        AXDS_FRAME_OFFSET_T tIFrameOffsetInfo = m_mapDev2DSF[nDeviceID][nStreamID]->GetIFrameOffsetInfo();

        AXIF_FILE_INFO_T tInfo;
        memset(tInfo.szFilePath, 0, 128);
        strncpy(tInfo.szFilePath, m_mapDev2DSF[nDeviceID][nStreamID]->GetFilePath(), 127);
        tInfo.uFileSize = tDSHeader.uFileSize;
        tInfo.uFrameCount = tDSHeader.uFrameCount;
        tInfo.uEncodeType = tDSHeader.uEncodeType;
        tInfo.uFrameRate = tDSHeader.uFrameRate;
        tInfo.uGop = tDSHeader.uGop;
        tInfo.uWidth = tDSHeader.uWidth;
        tInfo.uHeight = tDSHeader.uHeight;
        tInfo.tStartTime = tDSHeader.tStartTime;
        tInfo.tEndTime = tDSHeader.tEndTime;

        tInfo.uIFrameCount = tIFrameOffsetInfo.nCount;
        tInfo.pIFrameOffsetStart = tIFrameOffsetInfo.pOffsetStart;
        tInfo.uSize = sizeof(AXIF_FILE_INFO_T) - sizeof(AX_U32*)/* Remove ptr size of pIFrameOffsetStart */ + tInfo.uIFrameCount * sizeof(AX_U32) /* Add I frame offset buffer size */;

        m_mapDev2DSIF[nDeviceID][nStreamID]->CreateAndSave(tInfo);
    }

    return bRet;
}

AX_S32 CDataStreamRecord::SwitchFile(AX_U8 nDeviceID, AX_U8 nStreamID) {
    LOG_MM_I(TAG, "[%d][%d] +++", nDeviceID, nStreamID);
    AX_S32 nOldFD = m_mapDev2FileInfo[nDeviceID][nStreamID].nFD;
    if (AX_DS_INVALID_HANDLE == nOldFD) {
        return AX_DS_INVALID_HANDLE;
    }

    CloseDataFile(nDeviceID, nStreamID);

    LOG_MM_I(TAG, "[%d][%d] ---", nDeviceID, nStreamID);
    return CreateDataFile(nDeviceID, nStreamID);
}

AX_BOOL CDataStreamRecord::CheckStream(AX_U8 nDeviceID, AX_U8 nStreamID, AX_BOOL bCheckOpen /*= AX_FALSE*/) {
    if (nDeviceID >= m_tInitAttr.uMaxDevCnt || nStreamID >= m_tInitAttr.uStreamCnt) {
        LOG_MM_E(TAG, "[%d][%d] Check stream failed", nDeviceID, nStreamID);
        return AX_FALSE;
    }

    if (bCheckOpen) {
        return (m_mapDev2DSF[nDeviceID][nStreamID] == nullptr || AX_DS_INVALID_HANDLE == m_mapDev2FileInfo[nDeviceID][nStreamID].nFD) ? AX_FALSE : AX_TRUE;
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::CheckFrameType(AX_U8 nFrameType) {
    if (nFrameType >= AXDS_FRM_TYPE_MAX) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamRecord::IsTypeForSwitch(AX_U8 nFrameType) {
    AXDS_FRM_TYPE_E eFrmType = (AXDS_FRM_TYPE_E)nFrameType;
    if (AXDS_FRM_TYPE_VIDEO_IDR == eFrmType || AXDS_FRM_TYPE_VIDEO_SPS == eFrmType) {
        if (m_nLastFrameType != AXDS_FRM_TYPE_VIDEO_SPS && m_nLastFrameType != AXDS_FRM_TYPE_VIDEO_PPS && m_nLastFrameType != AXDS_FRM_TYPE_VIDEO_VPS) {
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

AX_VOID CDataStreamRecord::ThreadStreamMonitor(AX_VOID* pArg) {
    prctl(PR_SET_NAME, "DS_MONITOR");

    AX_U64 nTickStart = CElapsedTimer::GetInstance()->GetTickCount();
    AX_U64 nTickEnd = nTickStart;
    std::map<AX_U8, std::vector<STREAM_MONITOR_INFO_T>> mapDev2MonitorInfoBackup = m_mapDev2MonitorInfo;
    while (m_threadStreamMonitor.IsRunning()) {
        nTickEnd = CElapsedTimer::GetInstance()->GetTickCount();
        if ((nTickEnd - nTickStart) >= m_tConfig.nFrequency * 1000) {
            LOG_M_C(TAG, "--------------------------------------------------------------------------------------");
            AX_U8 nMaxDevCnt = m_tInitAttr.uMaxDevCnt;
            for (AX_U8 i = 0; i < nMaxDevCnt; i++) {
                for (AX_U8 j = 0; j < m_tInitAttr.uStreamCnt; j++) {
                    m_mapDev2MonitorInfo[i][j].fBitrate = (m_mapDev2MonitorInfo[i][j].nSize - mapDev2MonitorInfoBackup[i][j].nSize) * 8 / (AX_F32)m_tConfig.nFrequency / 1000;
                    m_mapDev2MonitorInfo[i][j].nFramerate = (m_mapDev2MonitorInfo[i][j].nSeq - mapDev2MonitorInfoBackup[i][j].nSeq) / m_tConfig.nFrequency;
                    m_mapDev2MonitorInfo[i][j].Print();
                    mapDev2MonitorInfoBackup[i][j] = m_mapDev2MonitorInfo[i][j];
                }
            }

            nTickStart = nTickEnd;
        }

        CElapsedTimer::GetInstance()->mSleep(100);
    }
}