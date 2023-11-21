/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "datastream_play.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "fs.hpp"
#include <fcntl.h>
#include <unistd.h>

#define TAG "DATA_STREAM_P"
#define DS_MAX_SUPPORTED_DEVICE_COUNT (128)
#define DS_MAX_SUPPORTED_STREAM_COUNT (2)

using namespace std;


AX_BOOL CDataStreamPlay::Init(const AXDS_PLAY_INIT_ATTR_T& tAttr) {
    m_tInitAttr = tAttr;

    for (AX_U32 i = 0; i < m_tInitAttr.uMaxDevCnt; ++i) {
        m_mapDev2ThreadPlay[i].resize(m_tInitAttr.uStreamCnt);
        m_mapDev2ThreadParam[i].resize(m_tInitAttr.uStreamCnt);
        m_mapDev2ThreadPlayingTime[i].resize(m_tInitAttr.uStreamCnt);
        m_mapDev2SpeedFactor[i].resize(m_tInitAttr.uStreamCnt);
        m_mapDev2StepFrmOpen[i].resize(m_tInitAttr.uStreamCnt);
        m_mapDev2CurrPTS[i].resize(m_tInitAttr.uStreamCnt);
        m_mapDev2Obs[i].resize(m_tInitAttr.uStreamCnt);
        m_mapDev2Mtx[i].resize(m_tInitAttr.uStreamCnt);
        for (AX_U32 j = 0; j < m_tInitAttr.uStreamCnt; ++j) {
            m_mapDev2ThreadPlay[i][j] = new CAXThread();
            m_mapDev2Mtx[i][j] = new std::mutex();
            m_mapDev2ThreadParam[i][j].nDevice = i;
            m_mapDev2ThreadParam[i][j].nStream = j;
            m_mapDev2ThreadParam[i][j].bExited = AX_TRUE;
            m_mapDev2SpeedFactor[i][j] = 1;
            m_mapDev2StepFrmOpen[i][j] = AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::DeInit() {
    for (AX_U32 i = 0; i < m_tInitAttr.uMaxDevCnt; ++i) {
        for (AX_U32 j = 0; j < m_tInitAttr.uStreamCnt; ++j) {
            StopPlay(i, j);

            delete m_mapDev2ThreadPlay[i][j];
            m_mapDev2ThreadPlay[i][j] = nullptr;

            delete m_mapDev2Mtx[i][j];
            m_mapDev2Mtx[i][j] = nullptr;
        }
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::StartPlay(AX_U8 nDeviceID, AX_U8 nStreamID, AX_S32 nDate, AX_S32 nTime /*= 0*/, AX_BOOL bReverse /*= AX_FALSE*/) {
    if (nullptr == m_mapDev2ThreadPlay[nDeviceID][nStreamID]) {
        return AX_FALSE;
    }

    m_mapDev2ThreadParam[nDeviceID][nStreamID].nYYYYMMDD = nDate;
    m_mapDev2ThreadParam[nDeviceID][nStreamID].nHHMMSS = nTime;
    m_mapDev2ThreadParam[nDeviceID][nStreamID].bReverse = bReverse;
    m_mapDev2ThreadParam[nDeviceID][nStreamID].bExited = AX_FALSE;
    m_mapDev2ThreadParam[nDeviceID][nStreamID].pThread = m_mapDev2ThreadPlay[nDeviceID][nStreamID];
    if (!m_mapDev2ThreadPlay[nDeviceID][nStreamID]->Start([this](AX_VOID* pArg) -> AX_VOID { PlayThread(pArg); }, &m_mapDev2ThreadParam[nDeviceID][nStreamID], "ds_play", SCHED_FIFO, 99)) {
        LOG_MM_E(TAG, "[%d][%d] Create data stream play thread failed.", nDeviceID, nStreamID);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::StopPlay(AX_U8 nDeviceID, AX_U8 nStreamID) {
    if (m_mapDev2ThreadPlay[nDeviceID][nStreamID]->IsRunning()) {
        m_mapDev2ThreadPlay[nDeviceID][nStreamID]->Stop();
        m_mapDev2ThreadPlay[nDeviceID][nStreamID]->Join();
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::PausePlay(AX_U8 nDeviceID, AX_U8 nStreamID) {
    if (m_mapDev2ThreadPlay[nDeviceID][nStreamID]->IsRunning()) {
        m_mapDev2ThreadPlay[nDeviceID][nStreamID]->Pause();
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::ResumePlay(AX_U8 nDeviceID, AX_U8 nStreamID) {
    if (m_mapDev2ThreadPlay[nDeviceID][nStreamID]->IsPausing()) {
        m_mapDev2ThreadPlay[nDeviceID][nStreamID]->Resume();
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::StepFrame(AX_U8 nDeviceID, AX_U8 nStreamID) {
    m_mapDev2StepFrmOpen[nDeviceID][nStreamID] = AX_TRUE;
    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::IsPlaying(AX_U8 nDeviceID, AX_U8 nStreamID) {
    if (nullptr == m_mapDev2ThreadPlay[nDeviceID][nStreamID]) {
        return AX_FALSE;
    }

    return m_mapDev2ThreadParam[nDeviceID][nStreamID].bExited ? AX_FALSE : AX_TRUE;
}

AX_BOOL CDataStreamPlay::ChangeDirection(AX_U8 nDeviceID, AX_U8 nStreamID, AX_BOOL bReverse) {
    if (bReverse == m_mapDev2ThreadParam[nDeviceID][nStreamID].bReverse) {
        return AX_TRUE;
    }

    AX_BOOL bIsPausing = m_mapDev2ThreadPlay[nDeviceID][nStreamID]->IsPausing();
    if (bIsPausing) {
        // Resume playing to quit thread via StopPlay
        m_mapDev2ThreadPlay[nDeviceID][nStreamID]->Resume();
    }

    StopPlay(nDeviceID, nStreamID);

    std::pair<AX_U32, AX_U32> pairDateTime = GetCurrentDateTime(nDeviceID, nStreamID);

    AX_BOOL bNewReverseFlg = m_mapDev2ThreadParam[nDeviceID][nStreamID].bReverse ? AX_FALSE : AX_TRUE;
    StartPlay(nDeviceID, nStreamID, pairDateTime.first, pairDateTime.second, bNewReverseFlg);

    if (bIsPausing) {
        /* Recover to pausing status */
        m_mapDev2ThreadPlay[nDeviceID][nStreamID]->Pause();
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::ChangeSpeed(AX_U8 nDeviceID, AX_U8 nStreamID, AX_F32 fFactor) {
    if (fFactor < 1e-6) {
        LOG_M_E(TAG, "Invalid speed factor %f", fFactor);
        return AX_FALSE;
    }

    m_mapDev2SpeedFactor[nDeviceID][nStreamID] = fFactor;
    return AX_TRUE;
}

std::pair<AX_U32, AX_U32> CDataStreamPlay::GetCurrentDateTime(AX_U8 nDeviceID, AX_U8 nStreamID) {
    AXDS_DATETIME_T& tCurrPlayedTime = m_mapDev2ThreadPlayingTime[nDeviceID][nStreamID];
    if (0 == tCurrPlayedTime.uSec && 0 == tCurrPlayedTime.uUsec) {
        return make_pair(0, 0);
    }

    return CElapsedTimer::GetInstance()->GetDateTimeIntVal(tCurrPlayedTime.uSec);
}

AX_VOID CDataStreamPlay::PlayThread(AX_VOID* pArg) {
    AXDS_PLAY_THREAD_PARAM_T* pParams = (AXDS_PLAY_THREAD_PARAM_T*)pArg;
    AX_U32 nDevice = pParams->nDevice;
    AX_U32 nStream = pParams->nStream;
    AX_U32 nYYYYMMDD = pParams->nYYYYMMDD;
    AX_U32 nHHMMSS = pParams->nHHMMSS;
    AX_BOOL bReverse = pParams->bReverse;
    CAXThread* pThread = pParams->pThread;

    LOG_MM_I(TAG, "[%d][%d] +++", nDevice, nStream);

    CDataStreamIndFile* pInstance = CreateSearchInstance(nDevice, nStream, nYYYYMMDD, nHHMMSS);
    if (nullptr == pInstance) {
        return;
    }

    AXIF_FILE_INFO_EX_T tInfo = pInstance->FindInfo(0); /* Ignore the case that fps changes between different dsf */

    CDSIterator itDSStart = bReverse ? pInstance->rbegin() : pInstance->begin();
    CDSIterator itDSEnd = bReverse ? pInstance->rend() : pInstance->end();

    for (; itDSStart != itDSEnd; ++itDSStart) {
        while (pThread->IsPausing()) {
            if (m_mapDev2StepFrmOpen[nDevice][nStream]) {
                m_mapDev2StepFrmOpen[nDevice][nStream] = AX_FALSE;
                break;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        if (!pThread->IsRunning() && !pThread->IsPausing()) {
            itDSStart.Terminate();
            break;
        }

        AXDS_FRAME_HEADER_T* pFrameStart = *itDSStart;

        /* Save time for reversely play at the paused time */
        m_mapDev2ThreadPlayingTime[nDevice][nStream] = pFrameStart->tTimeStamp;
        GenCurrPTS(nDevice, nStream, tInfo, bReverse);

        LOG_MM_D(TAG, "Send frame to observers, pts = %lld, type = %d", m_mapDev2CurrPTS[nDevice][nStream], pFrameStart->GetNaluType());

        {
            std::lock_guard<std::mutex> lck(*m_mapDev2Mtx[nDevice][nStream]);
            for (auto&& m : m_mapDev2Obs[nDevice][nStream]) {
                if (!pThread->IsRunning() && !pThread->IsPausing()) {
                    break;
                }

                STREAM_FRAME_T tFrame;
                memset(&tFrame, 0, sizeof(STREAM_FRAME_T));
                tFrame.enPayload = tInfo.tInfo.uEncodeType;
                tFrame.nPrivData = 0;
                tFrame.frame.stVideo.nPTS = m_mapDev2CurrPTS[nDevice][nStream];
                tFrame.frame.stVideo.enNalu = pFrameStart->GetNaluType();
                tFrame.frame.stVideo.pData = (AX_U8*)pFrameStart + pFrameStart->uHeaderSize;
                tFrame.frame.stVideo.nLen = pFrameStart->uFrameSize;
                if (m != nullptr) {
                    if (!m->OnRecvStreamData(tFrame)) {
                        break;
                    }
                }
            }
        }
    }

    itDSStart.Destroy();
    pInstance = nullptr;

    pParams->bExited = AX_TRUE;

    LOG_MM_I(TAG, "[%d][%d] ---", nDevice, nStream);
}

std::deque<DISK_FILE_INFO_T> CDataStreamPlay::GetDeviceSubFolders(const string& strParentDir, AX_U8 nDeviceID) {
    AX_CHAR szDevPath[300] = {0};
    sprintf(szDevPath, "%s/DEV_%02d", strParentDir.c_str(), nDeviceID + 1);
    return CDiskHelper::TraverseDirs(szDevPath);
}

vector<AXDS_DEVICE_INFO_T> CDataStreamPlay::GetAllDeviceInfo(const string& strParentDir) {
    vector<AXDS_DEVICE_INFO_T> vecInfo;
    for (AX_U8 i = 0; i < DS_MAX_SUPPORTED_DEVICE_COUNT; ++i) {
        std::deque<DISK_FILE_INFO_T> dirs = GetDeviceSubFolders(strParentDir, i);
        vecInfo.emplace_back(dirs.size() > 0 ? AX_TRUE : AX_FALSE, i, dirs);
    }

    return vecInfo;
}

CVideoInfoMap CDataStreamPlay::GetVideoInfo(const string& strParentDir, const string& strYYYYmmdd) {
    CVideoInfoMap mapVideoInfo;

    AX_U8 nMaxDev = DS_MAX_SUPPORTED_DEVICE_COUNT;
    AX_U8 nMaxStream = DS_MAX_SUPPORTED_STREAM_COUNT;
    string strIndFile;
    AXIF_FILE_HEADER_T tHeader;
    AX_U32 nFileCount = 0;
    for (AX_U8 i = 0; i < nMaxDev; ++i) {
        vector<vector<AXDS_VIDEO_INFO_T>> vecStreamPeriod;
        for (AX_U8 j = 0; j < nMaxStream; ++j) {
            strIndFile = FindIndFile(strParentDir, i, j, strYYYYmmdd.c_str());
            if (!strIndFile.empty()) {
                unique_ptr<CDataStreamIndFile> file = make_unique<CDataStreamIndFile>();
                if (!file->Init(strIndFile.c_str(), AXIF_OPEN_FOR_READ)) {
                    continue;
                }

                tHeader = file->GetFileHeader();
                nFileCount = tHeader.uFileCount;

                vector<AXDS_VIDEO_INFO_T> vecPeriod;
                vecPeriod.reserve(nFileCount);

                for (CDSIFIterator itBegin = file->info_begin(), itEnd = file->info_end(); itBegin != itEnd; ++itBegin) {
                    AXDS_VIDEO_INFO_T period((*itBegin).tInfo.tStartTime, (*itBegin).tInfo.tEndTime);
                    vecPeriod.push_back(period);
                }

                file->DeInit();

                vecStreamPeriod.emplace_back(vecPeriod);
            }
        }

        if (vecStreamPeriod.size() > 0) {
            mapVideoInfo[i] = vecStreamPeriod;
        }
    }

    return mapVideoInfo;
}

AX_BOOL CDataStreamPlay::GetStreamInfo(AX_U8 nDeviceID, AX_U8 nStreamID, AX_U32 nDateIntVal, AXDS_STREAM_INFO_T& tOutInfo) {
    AX_CHAR szDate[32] = {0};
    sprintf(szDate, "%04d-%02d-%02d", nDateIntVal / 10000, (nDateIntVal % 10000) / 100, nDateIntVal % 100);

    string strIndFile = FindIndFile(m_tInitAttr.strParentDir, nDeviceID, nStreamID, szDate);
    if (!strIndFile.empty()) {
        unique_ptr<CDataStreamIndFile> file = make_unique<CDataStreamIndFile>();
        if (!file->Init(strIndFile.c_str(), AXIF_OPEN_FOR_READ)) {
            return AX_FALSE;
        }

        for (CDSIFIterator itBegin = file->info_begin(), itEnd = file->info_end(); itBegin != itEnd; ++itBegin) {
            AXIF_FILE_INFO_T& tInfo = (*itBegin).tInfo;
            std::pair<AX_U32, AX_U32> pairDateTime = CElapsedTimer::GetInstance()->GetDateTimeIntVal(tInfo.tStartTime.uSec);
            if (pairDateTime.first == nDateIntVal) {
                tOutInfo.uEncodeType = tInfo.uEncodeType;
                tOutInfo.uWidth = tInfo.uWidth;
                tOutInfo.uHeight = tInfo.uHeight;
                tOutInfo.uFrameRate = tInfo.uFrameRate;
                tOutInfo.uGop = tInfo.uGop;

                return AX_TRUE;
            }
        }

        file->DeInit();
    }

    return AX_FALSE;
}

CDataStreamIndFile* CDataStreamPlay::CreateSearchInstance(AX_U8 nDeviceID, AX_U8 nStreamID, AX_S32 nDate, AX_S32 nTime /*= 0*/) {
    AX_S32 nYear = nDate / 10000;
    AX_S32 nMonth = (nDate % 10000) / 100;
    AX_S32 nDay = nDate % 100;

    AX_CHAR szDate[32] = {0};
    sprintf(szDate, "%02d-%02d-%02d", nYear, nMonth, nDay);
    string strIndexFile = CDataStreamPlay::FindIndFile(m_tInitAttr.strParentDir, nDeviceID, nStreamID, szDate);
    if (strIndexFile.empty()) {
        return nullptr;
    }

    CDataStreamIndFile* dsif = new CDataStreamIndFile();
    if (!dsif->Init(strIndexFile.c_str(), AXIF_OPEN_FOR_READ, nDate, nTime)) {
        return nullptr;
    }

    return dsif;
}

AX_VOID CDataStreamPlay::DestroySearchInstance(CDataStreamIndFile* pInstance) {
    if (nullptr == pInstance) {
        return;
    }

    pInstance->DeInit();
    delete pInstance;
    pInstance = nullptr;
}

string CDataStreamPlay::FindIndFile(std::string strParentDir, AX_U8 nDeviceID, AX_U8 nStreamID, string strYYYYmmdd) {
    AX_CHAR szFilePath[300] = {0};
    sprintf(szFilePath, "%s/DEV_%02d/%s/%s", strParentDir.c_str(), nDeviceID + 1, strYYYYmmdd.c_str(), CDataStreamIndFile::FormatFileName(nStreamID).c_str());
    fs::path p(szFilePath);
    if (fs::exists(p)) {
        return szFilePath;
    }

    return string();
}

AX_VOID CDataStreamPlay::GenCurrPTS(AX_U8 nDeviceID, AX_U8 nStreamID, const AXIF_FILE_INFO_EX_T& tInfo, AX_BOOL bReverse) {
    AX_U32 nStep = 0;
    if (bReverse) {
        nStep = (AX_F32)tInfo.tInfo.uGop / tInfo.tInfo.uFrameRate * 1000000;
    } else {
        //nStep = 1000 / tInfo.tInfo.uFrameRate * 1000;
        nStep = 1000000 / tInfo.tInfo.uFrameRate;
    }

    nStep /= m_mapDev2SpeedFactor[nDeviceID][nStreamID];

    m_mapDev2CurrPTS[nDeviceID][nStreamID] += nStep;
}

AX_BOOL CDataStreamPlay::RegisterObserver(AX_U8 nDeviceID, AX_U8 nStreamID, IStreamObserver* pObs) {
    if (!pObs) {
        LOG_M_E(TAG, "%s: observer is nil", __func__);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(*m_mapDev2Mtx[nDeviceID][nStreamID]);
    std::list<IStreamObserver*>& observers = m_mapDev2Obs[nDeviceID][nStreamID];
    auto it = std::find(observers.begin(), observers.end(), pObs);
    if (it != observers.end()) {
        LOG_M_W(TAG, "%s: observer has been registed", __func__);
    } else {
        observers.push_back(pObs);
        LOG_M_I(TAG, "%s: regist observer %p ok", __func__, pObs);
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamPlay::UnRegisterObserver(AX_U8 nDeviceID, AX_U8 nStreamID, IStreamObserver* pObs) {
    if (!pObs) {
        LOG_M_E(TAG, "%s: observer is nil", __func__);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(*m_mapDev2Mtx[nDeviceID][nStreamID]);
    std::list<IStreamObserver*>& observers = m_mapDev2Obs[nDeviceID][nStreamID];
    auto it = std::find(observers.begin(), observers.end(), pObs);
    if (it != observers.end()) {
        observers.remove(pObs);
        LOG_M_I(TAG, "%s: unregist observer %p ok", __func__, pObs);
        return AX_TRUE;
    } else {
        LOG_M_E(TAG, "%s: observer %p is not registed", __func__, pObs);
        return AX_FALSE;
    }
}