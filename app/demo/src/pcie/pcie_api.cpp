#include <iostream>
#include <sstream>
#include <cstring>
#include <map>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <iomanip>

#include "pcie_api.h"
#include "fs.hpp"
#include "ax_pcie_msg_api.h"
#include "ax_pcie_dma_api.h"
#include "AppLogApi.h"
#include "ax_sys_api.h"
#include "mm_buffer.h"


using namespace std;

#define MAX_SLAVE_DEVICE_NUMBER (8)
#define SHARED_PORT_NUMBER (1) // one shared port id(usually 0) for transfer command for each device
#define PORT_NO_MAX_NUM 118
#define PORT_BASE_NO 10
#define MAX_SIZE 3072 // 3K
#define TAG "APP_PCIE"


typedef struct _PORT_INFO_T {
    AX_U32 nSeq;
    struct _MM_INFO {
        AX_U8 *VirtualAddr;      /* 本机虚拟地址 */
        AX_U64 PhyBaseAddr;      /* 本机物理基地址 */
        AX_U64 PeerPhyBaseAddr;  /* 对端物理基地址 */
    } mm_info;

    _PORT_INFO_T() {
        nSeq = 0;
        mm_info.VirtualAddr     = NULL;
        mm_info.PhyBaseAddr     = 0;
        mm_info.PeerPhyBaseAddr = 0;
    }
} PORT_INFO_T;

typedef map<AX_S16, PORT_INFO_T> CChannelInfoMap;
static map<AX_S16, CChannelInfoMap> g_mapDev2ChnInfo;
static map<AX_S16, tuple<AX_S16, AX_S16>> g_mapChn2DevInfo;

static string g_VideoDataPath = "/opt/data/AiCard/video_data";
static map<AX_U32, fs::fstream> g_VideoDataFile;

static AX_S16 g_nTraceData  = 0;
static AX_S16 g_nRetryCount = 0;

AX_U16 g_nPortCnt = 0;
AX_S32 g_arrDeviceNo[MAX_SLAVE_DEVICE_NUMBER];
AX_S32 g_hPCIeDma = -1;
AX_BOOL g_bMaster = AX_FALSE;

static AX_U8 CheckSum(AX_U8 *pBuffer, AX_U8 nBufLen) {
    AX_U8 nIndex;
    AX_U8 nRet;
    AX_U16 sum = 0;
    for (nIndex=0; nIndex<nBufLen; nIndex++) {
        sum += pBuffer[nIndex];
    }
    nRet = (AX_U8)(sum & 0x00FF);
    return nRet;
}

static string ToHexString(AX_U8* pInput, AX_U32 nSize) {
    stringstream ss;
    ss<<setbase(16)<<setfill('0');
    for (AX_U32 i = 0; i < nSize; i++) {
        ss<<setw(2)<<(AX_U32)pInput[i]<<" ";
    }
    return ss.str();
}

static string GetTime() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

static AX_VOID LogData(AX_U32 nPort, AX_U32 nChannel, const AX_CHAR* direct, AX_U64 address, AX_U32 nDataSize, AX_U8* dataBuf, const AX_CHAR* logBuf) {
    if (g_nTraceData == 0) {
        return;
    }
    std::stringstream ss;
    ss << "[" << GetTime() << "]"
       << "[" << nPort << "]"
       << "[" << nChannel << "]"<<direct
       << "Size = " << nDataSize << " dst = 0x"<<setbase(16) << setfill('0')  // std::hex
       <<setw(8) <<address << '\n';

    if (logBuf) {
        ss << logBuf<<'\n';
        if (g_VideoDataFile[nPort] && ss.str().size() > 0) {
            g_VideoDataFile[nPort].write((char*)ss.str().c_str(), ss.str().size());
            g_VideoDataFile[nPort].flush();
        }
        return;
    }

    AX_U32 len = 0;
    AX_U32 priAddress = 0;
    //ss.str("");  //清空其内容
    while (nDataSize > 0) {
        std::stringstream stream;
        stream << "0x" <<setw(8) << setfill('0') << std::hex << priAddress<<"h";

        if (nDataSize >= 16) {
            ss << "                " << stream.str() << " : " << ToHexString(dataBuf + len, 16).c_str() << '\n';
            priAddress += 16;
            len += 16;
            nDataSize -= 16;
        } else {
            ss << "                " << stream.str() << " : " << ToHexString(dataBuf + len, nDataSize).c_str() << '\n';
            nDataSize = 0;
        }
    }

    if (g_VideoDataFile[nPort] && ss.str().size() > 0) {
        g_VideoDataFile[nPort].write((char*)ss.str().c_str(), ss.str().size());
        g_VideoDataFile[nPort].flush();
    }
}

static AX_S32 MasterInit(AX_U16 nTargetSlaveCnt, AX_U16 nChannelCnt, AX_U32 nDmaBufferSize) {
    AX_U8 *MM_VirtualAddr = NULL;
    AX_U64 MM_PhyBaseAddr = 0;
    AX_S32 nRet = PCIE_SUCCESS;
    AX_S32 arrPcieDevId[PCIE_MAX_CHIPNUM];
    AX_S32 *pArrPcieDevId = arrPcieDevId;
    AX_S32 nPcieDevCount = 0;

    nRet = AX_PCIe_GetTargetId(arrPcieDevId, &nPcieDevCount);
    if (nPcieDevCount <= 0) {
        LOG_M_E(TAG, "Can not find connected PCIe devices!");
        return PCIE_ERROR;
    }

    if (nTargetSlaveCnt > nPcieDevCount) {
        LOG_M_E(TAG, "Only %d device(s) found while %d devices required, please check PCIe connections or configurations.", nPcieDevCount, nTargetSlaveCnt);
        return PCIE_ERROR;
    }

    if (nTargetSlaveCnt > MAX_SLAVE_DEVICE_NUMBER) {
        LOG_MM_E(TAG, "Required devices is out of range, max %d devices is supported.", MAX_SLAVE_DEVICE_NUMBER);
        return PCIE_ERROR;;
    }

    AX_S32 nDataChnCnt = nChannelCnt - SHARED_PORT_NUMBER * nTargetSlaveCnt;
    AX_S32 nChnNumPerDev = nDataChnCnt % nTargetSlaveCnt ? nDataChnCnt / nTargetSlaveCnt + 1 : nDataChnCnt / nTargetSlaveCnt;
    AX_S32 nAllShardChnNum = SHARED_PORT_NUMBER * nTargetSlaveCnt;
    AX_S32 nAllSendCount = nDataChnCnt / 2;
    g_nPortCnt = nChnNumPerDev * nTargetSlaveCnt + nAllShardChnNum;

    if (g_nPortCnt > PORT_NO_MAX_NUM) {
        LOG_M_E(TAG, "Port number %d is out of range [0, %d]", g_nPortCnt, PORT_NO_MAX_NUM);
        return PCIE_ERROR;
    }

    nRet = AX_PCIe_InitRcMsg(g_nPortCnt, PORT_BASE_NO);
    if (nRet == -1) {
        LOG_M_E(TAG, "Init pcie rc msg failed!");
        return PCIE_ERROR;
    }

    for (AX_S32 i = 0; i < nTargetSlaveCnt; i++) {
        g_arrDeviceNo[i] = pArrPcieDevId[i];
        LOG_MM_C(TAG, "[%d] Device id is %d", i, g_arrDeviceNo[i]);
    }

    for (int i = 0; i < g_nPortCnt; i++) {
        nRet = MM_BufferInit(&MM_VirtualAddr, &MM_PhyBaseAddr, nDmaBufferSize);
        if (nRet < 0) {
            LOG_M_E(TAG, "Init buffer failed!");
            return PCIE_ERROR;
        }

        AX_U32 nDevIndex = 0;
        AX_U32 nChannelIndex = 0;
        if (i < nAllShardChnNum) {
            /* command channels */
            nDevIndex = i / SHARED_PORT_NUMBER;
            nChannelIndex = i % SHARED_PORT_NUMBER;
        } else if (i < nAllShardChnNum + nAllSendCount) {
            /* send channels */
            nDevIndex = (i - nAllShardChnNum) / (nChnNumPerDev / 2);
            nChannelIndex = (i - nAllShardChnNum) % (nChnNumPerDev / 2) + SHARED_PORT_NUMBER;
        } else {
            /* recv channels */
            nDevIndex = (i - nAllShardChnNum - nAllSendCount) / (nChnNumPerDev / 2);
            nChannelIndex = (i - nAllShardChnNum - nAllSendCount) % (nChnNumPerDev / 2) + SHARED_PORT_NUMBER + nChnNumPerDev / 2;
        }

        g_mapDev2ChnInfo[nDevIndex][nChannelIndex].mm_info.VirtualAddr = MM_VirtualAddr;
        g_mapDev2ChnInfo[nDevIndex][nChannelIndex].mm_info.PhyBaseAddr = MM_PhyBaseAddr;

        LOG_MM_I(TAG, "[Port %d] => [Device %d][Channel %d]", i, nDevIndex, nChannelIndex);

        g_mapChn2DevInfo[i] = make_tuple(nDevIndex, nChannelIndex);
    }

    for (auto devMap : g_mapDev2ChnInfo) {
        AX_S16 nDevIndex = devMap.first;
        CChannelInfoMap mapChnInfo = devMap.second;
        for (auto& portInfo : mapChnInfo) {
            AX_S16 nChnID = portInfo.first;
            AX_U64 nRcPhyBaseAddr = portInfo.second.mm_info.PhyBaseAddr;

            nRet = AX_PCIe_SendRcPhyBaseAddr(g_arrDeviceNo[nDevIndex], nChnID + PORT_BASE_NO, nRcPhyBaseAddr);
            if (nRet < 0) {
                LOG_M_E(TAG, "[%d-%d][%d] Send RC physical base address to EP failed!", nDevIndex, g_arrDeviceNo[nDevIndex], nChnID + PORT_BASE_NO);
                return PCIE_ERROR;
            }
        }
    }

    return nRet;
}

static AX_S32 MasterDeInit(AX_U32 nChannelCnt) {
    AX_S32 nRet = 0;

    for (AX_U32 i = 0; i < nChannelCnt; i++) {
        map<AX_S16, tuple<AX_S16, AX_S16>>::iterator itFinder = g_mapChn2DevInfo.find(i);
        if (itFinder != g_mapChn2DevInfo.end()) {
            /* PCIe close msg port */
            nRet = AX_PCIe_CloseMsgPort(std::get<0>(itFinder->second), PORT_BASE_NO + std::get<1>(itFinder->second));
            if (nRet < 0) {
                LOG_M_E(TAG, "Close msg port failed!");
                return AX_FAILURE;
            }
        }
    }

    return AX_SUCCESS;
}

static AX_S32 SlaveInit(AX_U32 nChannelCnt, AX_U32 nDmaBufferSize) {
    AX_S32 nRet = PCIE_SUCCESS;
    AX_U8 *MM_VirtualAddr = NULL;
    AX_U64 MM_PhyBaseAddr = 0;
    AX_U64 nPeerPhyBaseAddr = 0;

    g_nPortCnt = nChannelCnt;

    if (g_nPortCnt > PORT_NO_MAX_NUM) {
        LOG_M_E(TAG, "Port count %d out of range [0, %d]", g_nPortCnt, PORT_NO_MAX_NUM);
        return PCIE_ERROR;
    }

    nRet = AX_PCIe_InitEpMsg(g_nPortCnt, PORT_BASE_NO);
    if (nRet < 0) {
        LOG_M_E(TAG, "Init pcie ep msg failed!");
        return PCIE_ERROR;
    }

    //open pcie dma dev
    g_hPCIeDma = AX_PCIe_OpenDmaDev();
    if (g_hPCIeDma < 0) {
        LOG_M_E(TAG, "Open pcie dma dev failed!");
        return PCIE_ERROR;
    }

    for (AX_U32 i = 0; i < g_nPortCnt; i++) {
        nRet = MM_BufferInit(&MM_VirtualAddr, &MM_PhyBaseAddr, nDmaBufferSize);
        if (nRet < 0) {
            LOG_M_E(TAG, "Init buffer failed!");
            AX_PCIe_CloseDmaDev(g_hPCIeDma);
            return PCIE_ERROR;
        }

        g_mapDev2ChnInfo[0][i].mm_info.VirtualAddr = MM_VirtualAddr;
        g_mapDev2ChnInfo[0][i].mm_info.PhyBaseAddr = MM_PhyBaseAddr;

        g_mapChn2DevInfo[i] = make_tuple(0, i);
    }

    g_arrDeviceNo[0] = 0;

    for (auto devMap : g_mapDev2ChnInfo) {
        AX_S16 nDevIndex = devMap.first;
        CChannelInfoMap mapChnInfo = devMap.second;
        for (auto& portInfo : mapChnInfo) {
            AX_S16 nChnID = portInfo.first;

            nRet = AX_PCIe_WaitRcPhyBaseAddr(nChnID + PORT_BASE_NO, &nPeerPhyBaseAddr, -1);
            if (nRet < 0) {
                LOG_M_E(TAG, "Get RC physical base physical address to Slave failed!");
                return PCIE_ERROR;
            }

            g_mapDev2ChnInfo[nDevIndex][nChnID].mm_info.PeerPhyBaseAddr = nPeerPhyBaseAddr;
        }
    }

    return nRet;
}

static AX_S32 SlaveDeInit(AX_U32 nChannelCnt) {
    AX_S32 nRet = PCIE_SUCCESS;
    for (AX_U32 i = 0; i < nChannelCnt; i++) {
        /* PCIe close msg port */
        nRet = AX_PCIe_CloseMsgPort(0, PORT_BASE_NO + i);
        if (nRet < 0) {
            LOG_M_E(TAG, "Close msg port failed!");
            return PCIE_ERROR;
        }
    }

    if (-1 != g_hPCIeDma) {
        AX_PCIe_CloseDmaDev(g_hPCIeDma);
    }

    return nRet;
}

static AX_S32 SendCmd(AX_S32 nDevice, AX_S16 nChannel, PCIE_CMD_MSG_T* pData, AX_U32 nDataSize) {
    AX_S32 nRet;
    AX_U32 nMsgLen = sizeof(PCIE_CMD_MSG_HEAD_T) + nDataSize;
    pData->stMsgHead.nCheckSum = CheckSum(pData->nMsgBody, pData->stMsgHead.nDataLen);
    /* PCIe write msg */
    nRet = AX_PCIe_WriteMsg(g_arrDeviceNo[nDevice], nChannel + PORT_BASE_NO, pData, nMsgLen);
    if (nRet < 0) {
        LOG_M_E(TAG, "Send msg failed!, nChannel = %d", nChannel);
        return nRet;
    }

    return nMsgLen;
}

static AX_S32 RecvCmd(AX_S32 nDevice, AX_S16 nChannel, PCIE_CMD_MSG_T* pData, AX_S32 nTimeout) {
    AX_S32 nRet = AX_FAILURE;

    nRet = AX_PCIe_ReadMsg(g_arrDeviceNo[nDevice], nChannel + PORT_BASE_NO, pData, sizeof(PCIE_CMD_MSG_T), nTimeout);
    if (nRet < 0) {
        return nRet;
    }

    if ((pData->stMsgHead.nDataLen <= MAX_SIZE)
         && (pData->stMsgHead.nCheckSum != CheckSum(pData->nMsgBody, pData->stMsgHead.nDataLen))) {
        LOG_M_E(TAG, "Checksum  failed!, nChannel = %d, nCmdType = %d, nDataLen = %d",
              nChannel,
              pData->stMsgHead.nCmdType,
              pData->stMsgHead.nDataLen);
        nRet = AX_FAILURE;
        return nRet;
    }

    return nRet;
}

static AX_S32 MasterSendData(AX_S32 nDevice, AX_U32 nChannel,
                             AX_U8* MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                             AX_U8* pDataBuf, AX_U64 nSize, AX_U16 nRetryCount,
                             AX_S32 nTimeOut) {
    AX_S32 nReadSize;
    AX_S16 nPort = nChannel + PORT_BASE_NO;
    AX_S32 nRet = PCIE_ERROR;

    do {
        nReadSize = AX_PCIe_WaitReadDoneMsg(g_arrDeviceNo[nDevice], nPort, nTimeOut);
        if (nReadSize < 0) {
            LOG_M_E(TAG, "Wait read done msg failed, port = %d, %s", nPort, (nReadSize == -2?"time out":""));
            nRet = nReadSize;
        } else if (nReadSize == 0) {
            LOG_M_W(TAG, "Peer data CheckSum error, port = %d, nSize = %d", nPort, nSize);
            AX_U32 Size = nSize;
            LogData(nPort, nChannel, " ---> ", MM_PhyBaseAddr, Size, pDataBuf, NULL);
            nRet = PCIE_ERROR_CHECKSUM;
        } else {
            //printf("peer read data success\n");
            nRet = nReadSize;
            break;
        }
        break;
    } while (1);

    return nRet;
}

static AX_S32 MasterRecvDataWrapper(AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S32 nTimeOut) {
    if (pDataBuf == NULL) {
        return PCIE_ERROR;
    }

    map<AX_S16, tuple<AX_S16, AX_S16>>::iterator itFinder = g_mapChn2DevInfo.find(nChannel);
    if (itFinder == g_mapChn2DevInfo.end()) {
        LOG_M_E(TAG, "Invalid channel %d", nChannel);
        return PCIE_ERROR;
    }

    AX_S16 nDeviceID = std::get<0>(itFinder->second);
    AX_S16 nChannelID = std::get<1>(itFinder->second);
    if (!memcpy(pDataBuf, g_mapDev2ChnInfo[nDeviceID][nChannelID].mm_info.VirtualAddr, nSize)) {
        return PCIE_ERROR;
    }

    return PCIE_SUCCESS;
}

static AX_S32 SlaveSendData(AX_S16 nDeviceID, AX_S16 nChannelID,
                            AX_U8* MM_VirtualAddr, AX_U64 MM_PhyBaseAddr, AX_U64 DstPhyAddr,
                            AX_U8* pDataBuf, AX_U64 nSize, AX_U64 nRetryCount, AX_S32 nTimeOut)
{
    AX_S32 nReadSize;
    AX_S32 nRet = 0;
    AX_S16 nPort = nChannelID + PORT_BASE_NO;

    do {
        nRet = AX_PCIe_CreatDmaTask(g_hPCIeDma, DMA_WRITE, MM_PhyBaseAddr, DstPhyAddr, nSize, 1);
        if (nRet < 0) {
            LOG_M_E(TAG, "PCIe dma write failed, nPort = %d", nPort);
            nRet = PCIE_ERROR;
            nRetryCount--;
            continue;
        } else {
            //printf("pcie dma write success!\n");
        }

        //use msg mechanism
        //send a write_done message to RC
        //then wait RC's response(read_done)
        nRet = AX_PCIe_SendWriteDoneMsg(g_arrDeviceNo[nDeviceID], nPort, DstPhyAddr, nSize);
        if (nRet < 0) {
            LOG_M_E(TAG, "End write done msg failed, nPort = %d", nPort);
            nRet = PCIE_ERROR;
            nRetryCount--;
            continue;
        }

        nReadSize = AX_PCIe_WaitReadDoneMsg(g_arrDeviceNo[nDeviceID], nPort, nTimeOut);
        if (nReadSize < 0) {
            LOG_M_E(TAG, "Wait read done msg failed, nPort = %d, %s", nPort, (nReadSize == -2 ? "time out" : ""));
            nRet = nReadSize;
        } else if (nReadSize == 0) {
            LOG_M_E(TAG, "Peer data CheckSum error, nPort = %d", nPort);
            AX_U32 Size = nSize;
            LogData(nPort, nChannelID, " ---> ", MM_PhyBaseAddr, Size, pDataBuf, NULL);
            nRet = PCIE_ERROR_TIMEOUT;
        } else {
            nRet = nReadSize;
            break;
        }
        break;
    } while (1);

    return nRet;
}

static AX_S32 SlaveRecvData(AX_S32 nDevice, AX_S16 nChannel,
                            AX_U8* MM_VirtualAddr, AX_U64 MM_PhyBaseAddr, AX_U64 DstPhyAddr,
                            AX_U8* pDataBuf, AX_U32 nSize, AX_S32 nTimeOut) {
    AX_S32 nRet = 0;
    AX_S16 nPort = nChannel + PORT_BASE_NO;
    AX_U32 nDataSize = nSize;

    //create dma read task
    nRet = AX_PCIe_CreatDmaTask(g_hPCIeDma, DMA_READ, DstPhyAddr, MM_PhyBaseAddr, nDataSize, 1);
    if (nRet < 0) {
        LOG_M_E(TAG, "PCIe dma read failed, channel = %d", nChannel);
        //send a read_done message to rc
        nRet = AX_PCIe_SendReadDoneMsg(g_arrDeviceNo[nDevice], nPort, 0);
        if (nRet < 0) {
            LOG_M_E(TAG, "Send read_done msg failed, channel = %d", nChannel);
        }
        return PCIE_ERROR;
    }

    memcpy(pDataBuf, MM_VirtualAddr, nDataSize);

    nRet = nDataSize;
    return nRet;
}

static AX_S32 SlaveRecvDataWrapper(AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S32 nTimeOut) {
    if (pDataBuf == NULL) {
        return PCIE_ERROR;
    }

    map<AX_S16, tuple<AX_S16, AX_S16>>::iterator itFinder = g_mapChn2DevInfo.find(nChannel);
    if (itFinder == g_mapChn2DevInfo.end()) {
        LOG_M_E(TAG, "Invalid channel %d", nChannel);
        return PCIE_ERROR;
    }

    AX_S16 nDeviceID = std::get<0>(itFinder->second);
    AX_S16 nChannelID = std::get<1>(itFinder->second);
    PORT_INFO_T* pPortInfo = &g_mapDev2ChnInfo[nDeviceID][nChannelID];

    return SlaveRecvData(nDeviceID,
                         nChannelID, pPortInfo->mm_info.VirtualAddr,
                         pPortInfo->mm_info.PhyBaseAddr,
                         pPortInfo->mm_info.PeerPhyBaseAddr,
                         pDataBuf, nSize, nTimeOut);
}

AX_S32 PCIe_Init(AX_BOOL bMaster, AX_U16 nTargetSlaveCnt, AX_U16 nChannelNum, AX_U32 nDmaBufferSize, AX_S16 nTraceData, AX_S16 nRetryCount) {
    AX_S32 nRet = 0;
    g_nTraceData  = nTraceData;
    g_nRetryCount = nRetryCount;

    g_bMaster = bMaster;
    if (bMaster) {
        nRet = MasterInit(nTargetSlaveCnt, nChannelNum, nDmaBufferSize);
    } else {
        nRet = SlaveInit(nChannelNum, nDmaBufferSize);
    }

    if (nTraceData) {
        try {
            fs::create_directories(g_VideoDataPath);
        } catch (fs::filesystem_error& e) {
            LOG_M_I(TAG, "%s---> fail, %s", e.what());
        }

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&now), "%Y-%m-%d");
        string VideoDataFile = g_VideoDataPath;
        VideoDataFile += "/";
        VideoDataFile += oss.str();

        for(AX_U16 i = 0; i < g_nPortCnt; i++) {
            string tmp = VideoDataFile;
            tmp += "_" + to_string(i + PORT_BASE_NO) + "_data.txt";
            cout<<"tmp = "<<tmp<<endl;
            g_VideoDataFile[i + PORT_BASE_NO].open(tmp, ios::out);  // | ios::binary
            if (!g_VideoDataFile[i + PORT_BASE_NO]) {
                LOG_M_E(TAG, "%s---> %s, can't open , %s", tmp.c_str());
            }
        }
    }

    return nRet;
}

AX_S32 PCIe_DeInit() {
    if (g_bMaster) {
        MasterDeInit(g_nPortCnt);
    } else {
        SlaveDeInit(g_nPortCnt);
    }

    for(auto& it : g_VideoDataFile) {
        it.second.close();
    }

    g_VideoDataFile.erase(g_VideoDataFile.begin(), g_VideoDataFile.end());

    return 0;
}

AX_S32 PCIe_Send(PCIE_CMD_TYPE_E nCmdType, AX_S16 nDevice, AX_S16 nChannel,
                 AX_U8* pDataBuf, AX_U32 nSize, AX_S32 nTimeOut) {

    AX_S32 nRet = -1;

    AX_S32 nRetryCount = 0;

    if (pDataBuf == NULL) {
        return PCIE_ERROR;
    }

    if (nChannel >= PORT_NO_MAX_NUM) {
        return PCIE_ERROR;
    }

    PCIE_CMD_MSG_T* pStData = (PCIE_CMD_MSG_T*)pDataBuf;
    AX_U32 nDataSize = pStData->stMsgHead.nDataLen;
    if (pStData->stMsgHead.nDataLen > MAX_SIZE) {
        nDataSize = 0;
    }

    PORT_INFO_T* pPortInfo = &g_mapDev2ChnInfo[nDevice][nChannel];
    pPortInfo->nSeq += 1;
    if (0 == pPortInfo->nSeq) {
        pPortInfo->nSeq = 1;
    }
    pStData->stMsgHead.nSn = pPortInfo->nSeq;

    do {
        if (pStData->stMsgHead.nDataLen > MAX_SIZE) {
            if (nRetryCount == 0) {
                memcpy(pPortInfo->mm_info.VirtualAddr, pStData->nMsgBody, pStData->stMsgHead.nDataLen);
            }
        }

        nRet = SendCmd(nDevice, nChannel, pStData, nDataSize);
        if (nRet < 0) {
            return PCIE_ERROR;
        }

        if (pStData->stMsgHead.nDataLen <= MAX_SIZE) {
            return nRet;
        }

        if (g_bMaster) {
            nRet = MasterSendData(nDevice, nChannel,
                                  pPortInfo->mm_info.VirtualAddr, pPortInfo->mm_info.PhyBaseAddr,
                                  pStData->nMsgBody, pStData->stMsgHead.nDataLen, nRetryCount, nTimeOut);
        } else {
            nRet = SlaveSendData(nDevice, nChannel,
                                 pPortInfo->mm_info.VirtualAddr, pPortInfo->mm_info.PhyBaseAddr, pPortInfo->mm_info.PeerPhyBaseAddr,
                                 pStData->nMsgBody, pStData->stMsgHead.nDataLen, nRetryCount, nTimeOut);
        }

        if (nRet > 0) {
            break;
        }

        if (g_nRetryCount > 0) {
            if (nRetryCount < g_nRetryCount) {
                LogData(nDevice, nChannel, " ---> ", pPortInfo->mm_info.PhyBaseAddr, pStData->stMsgHead.nDataLen, NULL, "Retry");
                nRetryCount++;
                LOG_M_W(TAG, "%s: Retry send dma, port = %d, %d", nDevice, pStData->stMsgHead.nDataLen);
                continue;
            } else {
                LogData(nDevice, nChannel, " ---> ", pPortInfo->mm_info.PhyBaseAddr, pStData->stMsgHead.nDataLen, NULL, "Retry fail");
            }
        }
        break;
    } while (1);

    if (nRet < 0) {
        return nRet;
    }

    return nRet + sizeof(PCIE_CMD_MSG_HEAD_T);
}

AX_S32 PCIe_Recv(PCIE_CMD_TYPE_E nCmdType, AX_S16 nDevice, AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S32 nTimeOut) {
    AX_S32 nRet = PCIE_ERROR;
    AX_U32 nDataSize = 0;

    if (pDataBuf == NULL) {
        return PCIE_ERROR;
    }

    if (nChannel >= PORT_NO_MAX_NUM) {
        return PCIE_ERROR;
    }

    AX_S16 nPort = nChannel + PORT_BASE_NO;
    PORT_INFO_T* pPortInfo = &g_mapDev2ChnInfo[nDevice][nChannel];

    PCIE_CMD_MSG_T* pStData = (PCIE_CMD_MSG_T*)pDataBuf;
    pStData->stMsgHead.nDataLen = 0;

    nRet = RecvCmd(nDevice, nChannel, pStData, nTimeOut);
    if (nRet < 0) {
        return nRet;
    }

    if (pStData->stMsgHead.nDataLen <= MAX_SIZE) {
        return nRet;
    }

    if (pStData->stMsgHead.nDataLen > sizeof(pStData->nMsgBody)) {
        LOG_M_E(TAG, "Buffer is too small, buff size = %d, data size = %d", sizeof(pStData->nMsgBody), pStData->stMsgHead.nDataLen);
        return PCIE_ERROR;
    }

    if (g_bMaster) {
        nRet = MasterRecvDataWrapper(nChannel, pStData->nMsgBody, pStData->stMsgHead.nDataLen, nTimeOut);
    } else {
        nRet = SlaveRecvDataWrapper(nChannel, pStData->nMsgBody, pStData->stMsgHead.nDataLen, nTimeOut);
    }

    if (nRet < 0) {
        return nRet;
    }

    nDataSize = nRet;

    if (((AX_U32)nRet != pStData->stMsgHead.nDataLen)
        || (pStData->stMsgHead.nCheckSum != CheckSum(pStData->nMsgBody, pStData->stMsgHead.nDataLen))) {
        LOG_M_E(TAG, "Failed, nRet = %d, nPort = %d, nDataLen = %d, CheckSum error", nRet, nPort, pStData->stMsgHead.nDataLen);
        LogData(nPort, nChannel, " <--- ", pPortInfo->mm_info.PeerPhyBaseAddr, nDataSize, pStData->nMsgBody, NULL);
        nDataSize = 0;
    }

    // send a read_done message to rc
    nRet = AX_PCIe_SendReadDoneMsg(g_arrDeviceNo[nDevice], nPort, nDataSize);
    if (nRet < 0) {
        LOG_M_E(TAG, "Send read_done msg failed, nPort = %d", nPort);
        nRet = PCIE_ERROR;
        return nRet;
    }

    if (nDataSize == 0) {
        pPortInfo->nSeq = 0;
        return PCIE_ERROR_CHECKSUM;
    }

    if (pPortInfo->nSeq == pStData->stMsgHead.nSn) {
        LogData(nPort, nChannel, " <--- ", pPortInfo->mm_info.PeerPhyBaseAddr, nDataSize, pStData->nMsgBody, "repeat data");
        return PCIE_ERROR_REPEAT_DATA;
    } else {
        pPortInfo->nSeq = pStData->stMsgHead.nSn;
    }

    return nDataSize + sizeof(PCIE_CMD_MSG_HEAD_T);
}
