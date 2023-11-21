/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "AppLogApi.h"
#include "DiskHelper.hpp"
#include "ax_sys_api.h"
#include "cmdline.h"
#include "help.hpp"
#include "datastream_play.hpp"
#include "fs.hpp"
#include "ElapsedTimer.hpp"
#include "istream.hpp"

#define TAG "AXDS_SEARCH"
using namespace std;

#define IS_NULL(obj) (nullptr == obj ? AX_TRUE : AX_FALSE)
#define MAX_DEVICE_COUNT (32)
#define MAX_STREAM_COUNT_PER_DEV (2)

static std::unique_ptr<CDataStreamIndFile> CreateAndInitDSIF(string& strIndexFile);
static std::unique_ptr<CDataStreamFile> CreateAndOpenDSF(string& strDataFile);

typedef enum {
    CASE_TYPE_SEARCH_ALL = 0,
    CASE_TYPE_SEARCH_DATA,
    CASE_TYPE_SEARCH_IND,
    CASE_TYPE_SEARCH_IND_DAT,
    CASE_TYPE_GET_DEVICE_INFO,
    CASE_TYPE_GET_VIDEO_INFO,
    CASE_TYPE_MAX
} SEARCH_TYPE_E;


class CStreamTestObserver : public IStreamObserver {
public:
    CStreamTestObserver() = default;
    CStreamTestObserver(AX_S32 nDevice, AX_S32 nStream, AX_BOOL bSave = AX_FALSE) {
        m_nDevice = nDevice;
        m_nStream = nStream;
        m_bSaveStream = bSave;
        if (m_bSaveStream) {
            OpenFile();
        }
    };
    ~CStreamTestObserver() {
        if (m_bSaveStream) {
            CloseFile();
        }
    };

    AX_BOOL OnRecvStreamData(CONST STREAM_FRAME_T& stFrame) override;
    AX_BOOL OnRecvStreamInfo(CONST STREAM_INFO_T& stInfo) override;
    AX_VOID OnNotifyConnStatus(CONST AX_CHAR* pUrl, CONNECT_STATUS_E enStatus) override;

    AX_BOOL OpenFile();
    AX_VOID CloseFile();

public:
    AX_S32 m_nDevice {-1};
    AX_S32 m_nStream {-1};
    AX_S32 m_nFrmCount {0};
    AX_BOOL m_bSaveStream {AX_FALSE};
    AX_BOOL m_bFileOpened {AX_FALSE};
    AX_S32 m_hFD {-1};
};

AX_BOOL CStreamTestObserver::OnRecvStreamData(CONST STREAM_FRAME_T& stFrame) {
    std::pair<AX_U32, AX_U32> pairDateTime = CElapsedTimer::GetInstance()->GetDateTimeIntVal(stFrame.frame.stVideo.nPTS / 1000);
    LOG_MM_C(TAG, "[%d][%d] => [%d] NaluType: %d, FrmSize: %d, Date: %d, Time: %d", m_nDevice, m_nStream, ++m_nFrmCount, stFrame.frame.stVideo.enNalu, stFrame.frame.stVideo.nLen, pairDateTime.first, pairDateTime.second);
    if (m_bSaveStream) {
        write(m_hFD, stFrame.frame.stVideo.pData, stFrame.frame.stVideo.nLen);
    }
    return AX_TRUE;
}

AX_BOOL CStreamTestObserver::OnRecvStreamInfo(CONST STREAM_INFO_T& stInfo) {
    return AX_TRUE;
}

AX_VOID CStreamTestObserver::OnNotifyConnStatus(CONST AX_CHAR* pUrl, CONNECT_STATUS_E enStatus) {
    return;
}

AX_BOOL CStreamTestObserver::OpenFile() {
    if (-1 != m_hFD) {
        return AX_TRUE;
    }

    AX_CHAR szSavedFile[256] = {0};
    sprintf(szSavedFile, "/opt/data/datastream/DEV_%02d_STREAM_%02d.h264", m_nDevice, m_nStream);
    m_hFD = open(szSavedFile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    return -1 == m_hFD ? AX_FALSE : AX_TRUE;
}

AX_VOID CStreamTestObserver::CloseFile() {
    if (-1 != m_hFD) {
        close(m_hFD);
        m_hFD = -1;
    }
}

int main(int argc, char *argv[]) {
    /* ============================================ SAMPLE COMMAND ============================================ */
    /* 加载"datastream"目录下所有.ind文件并遍历frame */
    /* ./ut_datastream_search -p "/opt/data/datastream" -t 0 */

    /* 加载"/opt/data/datastream/DEV_01/1970-01-01/main_13-53-59.dat"数据文件并遍历frame */
    /* ./ut_datastream_search -p "/opt/data/datastream/DEV_01/1970-01-01/main_13-53-59.dat" -t 1 */

    /* 加载"/opt/data/datastream/DEV_01/1970-01-01/main.ind"索引文件并遍历file info */
    /* ./ut_datastream_search -p "/opt/data/datastream/DEV_01/1970-01-01/main.ind" -t 2 */

    /* 加载"datastream/1970-01-01"目录Device_0, Stream_0的索引文件并已10:10:10秒开始遍历 */
    /* ./ut_datastream_search -p "/opt/data/datastream" -d 0 -s 0 -e 19700101 -m 101010 -t 3 */

    /* 检索“/opt/data/datastream”目录下，所有设备的日期子目录 */
    /* ./ut_datastream_search -p "/opt/data/datastream" -t 4 */

    /* 检索“/opt/data/datastream”目录下，指定日期的所有设备的帧时间区间信息 */
    /* ./ut_datastream_search -p "/opt/data/datastream" -e 19700101 -t 5 */
    /* ======================================================================================================== */

    cmdline::parser a;
    a.add<string>("path", 'p', "path, different meaning for different type", true);
    a.add<AX_S32>("type", 't', "play type: 0=ALL, 1=DSF, 2=DSIF, 3=DSIF+DSF, 4=Get Date Dirs, 5=Get Video Time Period", false, 0);
    a.add<AX_S32>("reverse", 'r', "reverse play", false, 0);
    a.add<AX_S32>("recursive", 'c', "recursive play", false, 0);
    a.add<AX_S32>("device", 'd', "device index", false, 0);
    a.add<AX_S32>("stream", 's', "stream index", false, 0);
    a.add<AX_S32>("date", 'e', "YYYYMMDD", false, 0);
    a.add<AX_S32>("time", 'm', "HHMMDD", false, 0);
    a.add<AX_S32>("save", 'a', "save data for debug", false, 0);
    a.add<AX_S32>("log", 'v', "log level", false, APP_LOG_WARN);
    a.parse_check(argc, argv);

    string strPath = a.get<string>("path");
    AX_S32 nDate = a.get<AX_S32>("date");
    AX_S32 nTime = a.get<AX_S32>("time");
    SEARCH_TYPE_E eType = (SEARCH_TYPE_E)a.get<AX_S32>("type");
    AX_S32 nDevice = a.get<AX_S32>("device");
    AX_S32 nStream = a.get<AX_S32>("stream");
    AX_BOOL bReverse = a.get<AX_S32>("reverse") == 0 ? AX_FALSE : AX_TRUE;
    AX_BOOL bRecursive = a.get<AX_S32>("recursive") == 0 ? AX_FALSE : AX_TRUE;
    AX_BOOL bSave = a.get<AX_S32>("save") == 0 ? AX_FALSE : AX_TRUE;

    APP_SYS_INIT(0, AX_FALSE, AX_FALSE);
    AX_APP_SetLogLevel(a.get<AX_S32>("log"));

    LOG_M_C(TAG, "Input parameters:");
    LOG_M_C(TAG, "  Path: %s", strPath.c_str());
    LOG_M_C(TAG, "  Search type: %d", eType);
    LOG_M_C(TAG, "  Reverse: %d", bReverse);
    LOG_M_C(TAG, "  Recursive: %d", bRecursive);
    LOG_M_C(TAG, "  Device id: %d", nDevice);
    LOG_M_C(TAG, "  Stream id: %d", nStream);
    LOG_M_C(TAG, "  Date: %d", nDate);
    LOG_M_C(TAG, "  Time: %d", nTime);
    LOG_M_C(TAG, "  Save data: %d", bSave);

    fs::path p(strPath);
    if (!fs::exists(p)) {
        LOG_MM_E(TAG, "Data path %s not exists.", strPath.c_str());
        exit(1);
    }

    switch (eType) {
        case CASE_TYPE_SEARCH_ALL: {
            std::deque<DISK_FILE_INFO_T> listAllFiles = CDiskHelper::TraverseFiles(strPath.c_str(), ".ind");
            for (auto &m : listAllFiles) {
                /* UT for iterator of certain day's all frames by streams */
                do {
                    std::string strIndFile = m.path;
                    std::unique_ptr<CDataStreamIndFile> dsif = CreateAndInitDSIF(strIndFile);
                    if (!dsif) {
                        LOG_MM_E(TAG, "Open index file %s failed.", strIndFile.c_str());
                        _exit(1);
                    } else {
                        LOG_MM_C(TAG, "Open index file %s successfully.", strIndFile.c_str());
                    }

                    AX_U32 nAllFrmCount = 0;
                    CDSIterator itDSStart = dsif->begin();
                    CDSIterator itDSEnd = dsif->end();
                    for (; itDSStart != itDSEnd; ++itDSStart) {
                        AXDS_FRAME_HEADER_T* pFrameStart = *itDSStart;
                        LOG_MM_C(TAG, "[%d] FrmType: %d, FrmSize: %d, Time: %lld", ++nAllFrmCount, pFrameStart->uFrameType, pFrameStart->uFrameSize, pFrameStart->tTimeStamp.Value());
                    }

                    dsif->DeInit();
                } while(0);
            }

            break;
        }
        case CASE_TYPE_SEARCH_DATA: {
            /* UT for iterator of single data file's all frames */
            do {
                std::unique_ptr<CDataStreamFile> dsf = CreateAndOpenDSF(strPath);
                if (!dsf) {
                    LOG_MM_E(TAG, "Open data file %s failed.", strPath.c_str());
                    _exit(1);
                }

                AX_U32 nSingleFileFrmCount = 0;
                CDSFIterator itDSFStart = dsf->frm_begin();
                CDSFIterator itDSFEnd = dsf->frm_end();
                for (; itDSFStart != itDSFEnd; ++itDSFStart) {
                    AXDS_FRAME_HEADER_T* pFrameStart = *itDSFStart;
                    if (nullptr != pFrameStart) {
                        LOG_MM_C(TAG, "[%d] FrmType: %d, FrmSize: %d, Time: %lld", ++nSingleFileFrmCount, pFrameStart->uFrameType, pFrameStart->uFrameSize, pFrameStart->tTimeStamp.Value());
                    }
                }

                dsf->Close();
            } while(0);

            break;
        }
        case CASE_TYPE_SEARCH_IND: {
            /* UT for iterator of index file */
            do {
                std::unique_ptr<CDataStreamIndFile> dsif = CreateAndInitDSIF(strPath);
                if (!dsif) {
                    LOG_MM_E(TAG, "Open index file %s failed.", strPath.c_str());
                    _exit(1);
                }

                AX_U32 nInfoCount = 0;
                CDSIFIterator itDSIFStart = dsif->info_begin();
                CDSIFIterator itDSIFEnd = dsif->info_end();
                for (; itDSIFStart != itDSIFEnd; itDSIFStart++) {
                    AXIF_FILE_INFO_EX_T info = *itDSIFStart;
                    LOG_MM_C(TAG, "[%d] File: %s, FileSize: %d, Time: %lld, FrmCnt: %d, IFrameCnt: %d"
                                , ++nInfoCount
                                , info.tInfo.szFilePath
                                , info.tInfo.uFileSize
                                , info.tInfo.tStartTime.Value()
                                , info.tInfo.uFrameCount
                                , info.tInfo.uIFrameCount);
                }

                dsif->DeInit();
            } while(0);

            break;
        }
        case CASE_TYPE_SEARCH_IND_DAT: {
            /* UT for iterator of certain day's all frames */
            do {
                unique_ptr<CDataStreamPlay> instance = make_unique<CDataStreamPlay>();

                AXDS_PLAY_INIT_ATTR_T tInitAttr = {strPath, MAX_DEVICE_COUNT, MAX_STREAM_COUNT_PER_DEV};
                if (!instance->Init(tInitAttr)) {
                    exit(1);
                }

                AXDS_STREAM_INFO_T tStreamInfo = {0};
                if (!instance->GetStreamInfo(nDevice, nStream, nDate, tStreamInfo)) {
                    LOG_MM_E(TAG, "Can not get stream base info.");
                    exit(1);
                } else {
                    LOG_MM_C(TAG, "Stream info:");
                    LOG_MM_C(TAG, "    Width: %d", tStreamInfo.uWidth);
                    LOG_MM_C(TAG, "    Height: %d", tStreamInfo.uHeight);
                    LOG_MM_C(TAG, "    Framerate: %d", tStreamInfo.uFrameRate);
                    LOG_MM_C(TAG, "    Type: %s", PT_H265 == tStreamInfo.uEncodeType ? "H265" : "H264");
                }

                CStreamTestObserver* pObserver = new CStreamTestObserver(nDevice, nStream, bSave);
                instance->RegisterObserver(nDevice, nStream, pObserver);
                instance->StartPlay(nDevice, nStream, nDate, nTime, bReverse);

                do {
                    while (!IS_APP_QUIT() && instance->IsPlaying(nDevice, nStream)) {
                        CElapsedTimer::GetInstance()->mSleep(100);
                        continue;
                    }

                    if (!IS_APP_QUIT() && bRecursive) {
                        bReverse = bReverse ? AX_FALSE : AX_TRUE;
                        instance->ChangeDirection(nDevice, nStream, bReverse);
                    }
                } while (!IS_APP_QUIT() && bRecursive);

                instance->StopPlay(nDevice, nStream);
                instance->UnRegisterObserver(nDevice, nStream, pObserver);
                instance->DeInit();
            } while(0);

            break;
        }
        case CASE_TYPE_GET_DEVICE_INFO: {
            std::vector<AXDS_DEVICE_INFO_T> vecDeviceInfo = CDataStreamPlay::GetAllDeviceInfo(strPath);
            for (auto &m : vecDeviceInfo) {
                if (m.bValid) {
                    LOG_MM_C(TAG, "Device %d has %d date folders.", m.nDeviceID + 1, m.qDateDir.size());
                }
            }

            break;
        }
        case CASE_TYPE_GET_VIDEO_INFO: {
            AX_S32 nYear = nDate / 10000;
            AX_S32 nMonth = (nDate % 10000) / 100;
            AX_S32 nDay = nDate % 100;

            AX_CHAR szDate[32] = {0};
            sprintf(szDate, "%04d-%02d-%02d", nYear, nMonth, nDay);
            CVideoInfoMap mapTimePeriodInfo = CDataStreamPlay::GetVideoInfo(strPath, szDate);
            if (mapTimePeriodInfo.size() > 0) {
                for (auto& dev: mapTimePeriodInfo) {
                    LOG_MM_C(TAG, "[Device %02d]:", dev.first);
                    for (AX_U32 i = 0; i < dev.second.size(); ++i) {
                        LOG_MM_C(TAG, "    [Stream %02d] =>", i);
                        for (auto& info: dev.second[i]) {
                            AX_U32 nDateStart = info.tStart.Value() / 1000;
                            AX_U32 nDateEnd = info.tEnd.Value() / 1000;
                            std::pair<AX_U32, AX_U32> pairStartDateTime = CElapsedTimer::GetInstance()->GetDateTimeIntVal(nDateStart);
                            std::pair<AX_U32, AX_U32> pairEndDateTime = CElapsedTimer::GetInstance()->GetDateTimeIntVal(nDateEnd);
                            LOG_MM_C(TAG, "        %s: (%08d, %06d) -> (%08d, %06d)", info.uEncodeType == PT_H265 ? "H265" : "H264", pairStartDateTime.first, pairStartDateTime.second, pairEndDateTime.first, pairEndDateTime.second);
                        }
                    }
                }
            } else {
                LOG_MM_E(TAG, "No video data found on %s under path %s.", szDate, strPath.c_str());
            }

            break;
        }
        default: {
            break;
        }
    }

    return 0;
}

std::unique_ptr<CDataStreamIndFile> CreateAndInitDSIF(string& strIndexFile) {
    std::unique_ptr<CDataStreamIndFile> dsif = make_unique<CDataStreamIndFile>();
    if (!dsif->Init(strIndexFile.c_str(), AXIF_OPEN_FOR_READ)) {
        return nullptr;
    }

    return dsif;
}

std::unique_ptr<CDataStreamFile> CreateAndOpenDSF(string& strDataFile) {
    std::unique_ptr<CDataStreamFile> dsf = make_unique<CDataStreamFile>();
    if (!dsf->Open(strDataFile.c_str(), AX_DSF_OPEN_FOR_READ)) {
        return nullptr;
    }

    return dsf;
}
