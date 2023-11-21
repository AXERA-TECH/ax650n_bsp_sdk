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
#include "DiskHelper.hpp"
#include "datastreamfile.hpp"
#include "datastreamIndFile.hpp"
#include "AppLogApi.h"
#include "istream.hpp"
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <stdio.h>

/* {device0->[main:[(start, end)], sub1:[(start, end)]], device1->[main:[(start, end)], sub1:[(start, end)]]} */
typedef std::map<AX_U8, std::vector<std::vector<AXDS_VIDEO_INFO_T>>> CVideoInfoMap;


typedef struct AXDS_PLAY_INIT_ATTR {
    std::string strParentDir; // 数据文件保存父目录
    AX_U8   uMaxDevCnt; // 最大通道数
    AX_U8   uStreamCnt; //子码流个数
} AXDS_PLAY_INIT_ATTR_T;

typedef struct AXDS_DEVICE_INFO {
    AX_BOOL bValid;
    AX_U32 nDeviceID;
    std::deque<DISK_FILE_INFO_T> qDateDir;

    AXDS_DEVICE_INFO(AX_VOID) = default;
    AXDS_DEVICE_INFO(AX_BOOL _valid, AX_U16 _device, std::deque<DISK_FILE_INFO_T> _qDateDir) {
        bValid = _valid;
        nDeviceID = _device;
        qDateDir = _qDateDir;
    }
} AXDS_DEVICE_INFO_T;

typedef struct AXDS_PLAY_THREAD_PARAM {
    AX_U32 nDevice;
    AX_U32 nStream;
    AX_U32  nYYYYMMDD;  // 整形表示的日期，例如：20230419
    AX_U32  nHHMMSS;    // 整形表示的时刻，例如：101010
    AX_BOOL bReverse;   // 回放模式。0：正向；1：逆向
    AX_BOOL bExited;
    CAXThread* pThread;
} AXDS_PLAY_THREAD_PARAM_T;

/* Index File Info */
typedef struct AXDS_STREAM_INFO {
    AX_U16 uWidth;
    AX_U16 uHeight;
    AX_U16 uGop;
    AX_U16 uFrameRate;
    AX_PAYLOAD_TYPE_E uEncodeType;
} AXDS_STREAM_INFO_T;

class CDataStreamPlay {
public:
    CDataStreamPlay(AX_VOID) = default;

    AX_BOOL Init(const AXDS_PLAY_INIT_ATTR_T& tAttr);
    AX_BOOL DeInit();

    AX_BOOL StartPlay(AX_U8 nDeviceID, AX_U8 nStreamID, AX_S32 nDate, AX_S32 nTime = 0, AX_BOOL bReverse = AX_FALSE);
    AX_BOOL StopPlay(AX_U8 nDeviceID, AX_U8 nStreamID);
    AX_BOOL PausePlay(AX_U8 nDeviceID, AX_U8 nStreamID);
    AX_BOOL ResumePlay(AX_U8 nDeviceID, AX_U8 nStreamID);
    AX_BOOL StepFrame(AX_U8 nDeviceID, AX_U8 nStreamID);
    AX_BOOL IsPlaying(AX_U8 nDeviceID, AX_U8 nStreamID);
    AX_BOOL ChangeDirection(AX_U8 nDeviceID, AX_U8 nStreamID, AX_BOOL bReverse);
    /* fFactor: 倍速因子，2即为2倍速，0.5即为半倍速等 */
    AX_BOOL ChangeSpeed(AX_U8 nDeviceID, AX_U8 nStreamID, AX_F32 fFactor);

    AX_BOOL GetStreamInfo(AX_U8 nDeviceID, AX_U8 nStreamID, AX_U32 nDateIntVal, AXDS_STREAM_INFO_T& tOutInfo);
    std::pair<AX_U32, AX_U32> GetCurrentDateTime(AX_U8 nDeviceID, AX_U8 nStreamID);

    AX_BOOL RegisterObserver(AX_U8 nDeviceID, AX_U8 nStreamID, IStreamObserver* pObs);
    AX_BOOL UnRegisterObserver(AX_U8 nDeviceID, AX_U8 nStreamID, IStreamObserver* pObs);

    /* Get all video valid time periods info for all streams of all devices of certain date*/
    static CVideoInfoMap GetVideoInfo(const std::string& strParentDir, const std::string& strYYYYmmdd);
    /* Get all device info */
    static std::vector<AXDS_DEVICE_INFO_T> GetAllDeviceInfo(const std::string& strParentDir);
    /* Get date dirs for certain device */
    static std::deque<DISK_FILE_INFO_T> GetDeviceSubFolders(const std::string& strParentDir, AX_U8 nDeviceID);

protected:
    AX_VOID PlayThread(AX_VOID* pArg);
    /* Create instance for searching frames with index file corresponding to device/stream/date/time info */
    CDataStreamIndFile* CreateSearchInstance(AX_U8 nDeviceID, AX_U8 nStreamID, AX_S32 nDate, AX_S32 nTime = 0);
    /* Destroy instance created by CreateSearchInstance */
    AX_VOID DestroySearchInstance(CDataStreamIndFile* pInstance);

private:
    static std::string FindIndFile(std::string strParentDir, AX_U8 nDeviceID, AX_U8 nStreamID, std::string strYYYYmmdd);
    AX_VOID GenCurrPTS(AX_U8 nDeviceID, AX_U8 nStreamID, const AXIF_FILE_INFO_EX_T& tInfo, AX_BOOL bReverse);

private:
    AXDS_PLAY_INIT_ATTR_T m_tInitAttr;

    std::map<AX_U8, std::vector<CAXThread*>> m_mapDev2ThreadPlay;
    std::map<AX_U8, std::vector<AXDS_PLAY_THREAD_PARAM_T>> m_mapDev2ThreadParam;
    std::map<AX_U8, std::vector<AXDS_DATETIME_T>> m_mapDev2ThreadPlayingTime;
    std::map<AX_U8, std::vector<AX_U64>> m_mapDev2CurrPTS;
    std::map<AX_U8, std::vector<AX_F32>> m_mapDev2SpeedFactor;
    std::map<AX_U8, std::vector<AX_BOOL>> m_mapDev2StepFrmOpen;

    std::map<AX_U8, std::vector<std::list<IStreamObserver*>>> m_mapDev2Obs;
    std::map<AX_U8, std::vector<std::mutex*>> m_mapDev2Mtx;
};
