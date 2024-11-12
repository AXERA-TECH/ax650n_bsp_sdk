/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "PoolConfig.h"
#include "AXStringHelper.hpp"
#include "AppLogApi.h"
#include "CommonUtils.hpp"
#include "GlobalDef.h"
#include "ax_buffer_tool.h"
#include "ax_pool_type.h"
#include "ax_sys_api.h"
#include "ax_vin_api.h"

#define POOL "POOL"

#define AX_RAW_META_SIZE (8 * 1024)
#define AX_YUV_META_SIZE (8 * 1024)

CPoolConfig::CPoolConfig(POOL_ATTR_T tAttr) : m_tAttr(tAttr) {
    memset(&m_tCommPoolFloorPlan, 0, sizeof(AX_POOL_FLOORPLAN_T));
    memset(&m_tPrivPoolFloorPlan, 0, sizeof(AX_POOL_FLOORPLAN_T));
    LoadConfig();
}

AX_BOOL CPoolConfig::LoadConfig() {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    string strPoolCfgFile = strConfigDir + "/pool.ini";

    return m_iniParser.Load(strPoolCfgFile);
}

AX_BOOL CPoolConfig::AddBlocks() {
    LOG_MM_C(POOL, "+++");
    AX_U32 nBlkSize = 0;
    AX_U64 nBlkSizeRotate90 = 0;
    AX_U32 nWidthStride = 0;
    AX_U32 nMaxWidth = m_tAttr.nMaxWidth;
    AX_U32 nMaxHeight = m_tAttr.nMaxHeight;
    AX_U32 nBlkCnt = 0;
    AX_U32 nMetaSize = AX_RAW_META_SIZE;
    AX_S32 nScenario;
    CCmdLineParser::GetInstance()->GetScenario(nScenario);
    std::string strPPLCfg = CAXStringHelper::Format("PPL%d", nScenario);
    AX_FRAME_COMPRESS_INFO_T tCompressInfo;

    std::map<std::string, std::string> mapKeys;
    m_iniParser.GetAllKeys(strPPLCfg, mapKeys);
    for (auto& v : mapKeys) {
        std::string strKey = v.first;
        nBlkCnt = stoi(v.second);
        tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)(strKey[strKey.length() - 3] - '0');
        tCompressInfo.u32CompressLevel = strKey[strKey.length() - 1] - '0';

        if (strKey.find("RAW") != std::string::npos) {
            /* RAW132_MAX_FBC_1_0 */
            AX_U32 nPixFmt = 0;
            sscanf(&strKey[0], "RAW%d", &nPixFmt);
            nBlkSize = AX_VIN_GetImgBufferSize(nMaxHeight, nMaxWidth, (AX_IMG_FORMAT_E)nPixFmt, &tCompressInfo, 64);
            if (!AddPrivVinBlock("RAW", nBlkSize, nMetaSize, nBlkCnt)) {
                return AX_FALSE;
            }
        } else if (strKey.find("YUV_MAX") != std::string::npos) {
            /* YUV_MAX_FBC_1_0 */
            nMetaSize = AX_YUV_META_SIZE;
            nWidthStride = nMaxWidth;
            if (AX_COMPRESS_MODE_NONE != tCompressInfo.enCompressMode) {
                nWidthStride = ALIGN_UP(nMaxWidth, 256);
            }
            nBlkSize = AX_VIN_GetImgBufferSize(nMaxHeight, nWidthStride, AX_FORMAT_YUV420_SEMIPLANAR, &tCompressInfo, 64);
            if (m_tAttr.bRotatetion) {
                nBlkSizeRotate90 =
                    AX_VIN_GetImgBufferSize(nMaxWidth, ALIGN_UP(nMaxHeight, 256), AX_FORMAT_YUV420_SEMIPLANAR, &tCompressInfo, 64);
                nBlkSize = AX_MAX(nBlkSize, nBlkSizeRotate90);
            }
            if (!AddBlock("YUV_MAX", nBlkSize, nMetaSize, nBlkCnt, POOL_CACHE_MODE_NONCACHE)) {
                return AX_FALSE;
            }
        } else {
            /* YUV_1920x1080_FBC_2_4 */
            nMetaSize = AX_YUV_META_SIZE;
            AX_U32 nWidth = 0;
            AX_U32 nHeight = 0;
            sscanf(&strKey[0], "YUV_%dx%d", &nWidth, &nHeight);
            nWidthStride = nWidth;
            if (AX_COMPRESS_MODE_NONE != tCompressInfo.enCompressMode) {
                nWidthStride = ALIGN_UP(nWidth, 256);
            }
            nBlkSize = AX_VIN_GetImgBufferSize(nHeight, nWidthStride, AX_FORMAT_YUV420_SEMIPLANAR, &tCompressInfo, 64);
            if (m_tAttr.bRotatetion) {
                nBlkSizeRotate90 = AX_VIN_GetImgBufferSize(nWidth, ALIGN_UP(nHeight, 256), AX_FORMAT_YUV420_SEMIPLANAR, &tCompressInfo, 64);
                nBlkSize = AX_MAX(nBlkSize, nBlkSizeRotate90);
            }
            if (!AddBlock("YUV", nBlkSize, nMetaSize, nBlkCnt, POOL_CACHE_MODE_NONCACHE)) {
                return AX_FALSE;
            }
        }
    }
    MergeBlocks();
    LOG_MM_C(POOL, "---");

    return AX_TRUE;
}

AX_BOOL CPoolConfig::AddBlock(string strName, AX_U32 nBlkSize, AX_U32 nMetaSize, AX_U32 nBlkCount,
                              AX_POOL_CACHE_MODE_E eCacheMode /*= POOL_CACHE_MODE_NONCACHE*/) {
    if (0 == nBlkSize || 0 == nMetaSize || 0 == nBlkCount) {
        LOG_M_I(POOL, "Comm Block info invalid, Name=%s, nBlkSize=%d, nMetaSize=%d, nBlkCount=%d", strName.c_str(), nBlkSize, nMetaSize,
                nBlkCount);
        return AX_TRUE;
    }

    if (m_nPoolCount == AX_MAX_COMM_POOLS) {
        LOG_M_E(POOL, "Pool count exceeding max count %d", AX_MAX_COMM_POOLS);
        return AX_FALSE;
    }

    m_tCommPoolFloorPlan.CommPool[m_nPoolCount].MetaSize = nMetaSize;
    m_tCommPoolFloorPlan.CommPool[m_nPoolCount].BlkSize = nBlkSize;
    m_tCommPoolFloorPlan.CommPool[m_nPoolCount].BlkCnt = nBlkCount;
    m_tCommPoolFloorPlan.CommPool[m_nPoolCount].CacheMode = eCacheMode;

    memset(m_tCommPoolFloorPlan.CommPool[m_nPoolCount].PartitionName, 0, sizeof(m_tCommPoolFloorPlan.CommPool[m_nPoolCount].PartitionName));
    strcpy((char*)m_tCommPoolFloorPlan.CommPool[m_nPoolCount].PartitionName, "anonymous");

    m_nPoolCount += 1;

    // LOG_M(POOL, "Add Comm BLOCK (Name:%s, BlkSize:%d, MetaSize:%d, BlkCnt:%d, TotalCnt:%d)", strName.c_str(), nBlkSize, nMetaSize,
    //       nBlkCount, m_nPoolCount);

    return AX_TRUE;
}

AX_BOOL CPoolConfig::AddPrivVinBlock(string strName, AX_U32 nBlkSize, AX_U32 nMetaSize, const AX_U32 nBlkCount,
                                     AX_POOL_CACHE_MODE_E eCacheMode /*= POOL_CACHE_MODE_NONCACHE*/) {
    if (0 == nBlkSize || 0 == nMetaSize || 0 == nBlkCount) {
        LOG_M_I(POOL, "Block info invalid, Name=%s, nBlkSize=%d, nMetaSize=%d, nBlkCount=%d", strName.c_str(), nBlkSize, nMetaSize,
                nBlkCount);
        return AX_TRUE;
    }

    if (m_nPrivPoolCount == AX_MAX_COMM_POOLS) {
        LOG_M_E(POOL, "Pool count exceeding max count %d", AX_MAX_COMM_POOLS);
        return AX_FALSE;
    }

    m_tPrivPoolFloorPlan.CommPool[m_nPrivPoolCount].MetaSize = nMetaSize;
    m_tPrivPoolFloorPlan.CommPool[m_nPrivPoolCount].BlkSize = nBlkSize;
    m_tPrivPoolFloorPlan.CommPool[m_nPrivPoolCount].BlkCnt = nBlkCount;
    m_tPrivPoolFloorPlan.CommPool[m_nPrivPoolCount].CacheMode = eCacheMode;

    memset(m_tPrivPoolFloorPlan.CommPool[m_nPrivPoolCount].PartitionName, 0,
           sizeof(m_tPrivPoolFloorPlan.CommPool[m_nPrivPoolCount].PartitionName));
    strcpy((char*)m_tPrivPoolFloorPlan.CommPool[m_nPrivPoolCount].PartitionName, "anonymous");

    m_nPrivPoolCount += 1;

    // LOG_M(POOL, "Add Priv BLOCK (Name:%s, BlkSize:%d, MetaSize:%d, BlkCnt:%d, TotalCnt:%d)", strName.c_str(), nBlkSize, nMetaSize,
    //       nBlkCount, m_nPrivPoolCount);

    return AX_TRUE;
}

AX_VOID CPoolConfig::MergeBlocks() {
    AX_U8 i = 0;
    AX_U8 j = 0;
    AX_POOL_CONFIG_T v;

    /* Sort in ascending order */
    for (i = 0; i < m_nPoolCount - 1; i++) {
        for (j = i + 1; j < m_nPoolCount; j++) {
            if (m_tCommPoolFloorPlan.CommPool[i].BlkSize > m_tCommPoolFloorPlan.CommPool[j].BlkSize) {
                v.BlkSize = m_tCommPoolFloorPlan.CommPool[i].BlkSize;
                v.BlkCnt = m_tCommPoolFloorPlan.CommPool[i].BlkCnt;
                v.MetaSize = m_tCommPoolFloorPlan.CommPool[i].MetaSize;
                v.CacheMode = m_tCommPoolFloorPlan.CommPool[i].CacheMode;

                m_tCommPoolFloorPlan.CommPool[i].BlkSize = m_tCommPoolFloorPlan.CommPool[j].BlkSize;
                m_tCommPoolFloorPlan.CommPool[i].BlkCnt = m_tCommPoolFloorPlan.CommPool[j].BlkCnt;
                m_tCommPoolFloorPlan.CommPool[i].MetaSize = m_tCommPoolFloorPlan.CommPool[j].MetaSize;
                m_tCommPoolFloorPlan.CommPool[i].CacheMode = m_tCommPoolFloorPlan.CommPool[j].CacheMode;

                m_tCommPoolFloorPlan.CommPool[j].BlkSize = v.BlkSize;
                m_tCommPoolFloorPlan.CommPool[j].BlkCnt = v.BlkCnt;
                m_tCommPoolFloorPlan.CommPool[j].MetaSize = v.MetaSize;
                m_tCommPoolFloorPlan.CommPool[j].CacheMode = v.CacheMode;
            }
        }
    }

    /* Merge by size */
    for (i = 1, j = 0; i < m_nPoolCount; i++) {
        if (m_tCommPoolFloorPlan.CommPool[j].BlkSize != m_tCommPoolFloorPlan.CommPool[i].BlkSize) {
            j += 1;
            m_tCommPoolFloorPlan.CommPool[j].BlkSize = m_tCommPoolFloorPlan.CommPool[i].BlkSize;
            m_tCommPoolFloorPlan.CommPool[j].BlkCnt = m_tCommPoolFloorPlan.CommPool[i].BlkCnt;
            m_tCommPoolFloorPlan.CommPool[j].MetaSize = m_tCommPoolFloorPlan.CommPool[i].MetaSize;
            m_tCommPoolFloorPlan.CommPool[j].CacheMode = m_tCommPoolFloorPlan.CommPool[i].CacheMode;
            if (i != j) {
                m_tCommPoolFloorPlan.CommPool[i].BlkSize = 0;
                m_tCommPoolFloorPlan.CommPool[i].BlkCnt = 0;
                m_tCommPoolFloorPlan.CommPool[i].MetaSize = 0;
                m_tCommPoolFloorPlan.CommPool[i].CacheMode = POOL_CACHE_MODE_NONCACHE;
            }
        } else {
            m_tCommPoolFloorPlan.CommPool[j].BlkCnt += m_tCommPoolFloorPlan.CommPool[i].BlkCnt;
            m_tCommPoolFloorPlan.CommPool[i].BlkSize = 0;
            m_tCommPoolFloorPlan.CommPool[i].BlkCnt = 0;
            m_tCommPoolFloorPlan.CommPool[i].MetaSize = 0;
            m_tCommPoolFloorPlan.CommPool[i].CacheMode = POOL_CACHE_MODE_NONCACHE;
        }
    }

    m_nPoolCount = j + 1;
    MergePrivBlocks();
}

AX_VOID CPoolConfig::MergePrivBlocks() {
    AX_U8 i = 0;
    AX_U8 j = 0;
    AX_POOL_CONFIG_T v;

    /* Sort in ascending order */
    for (i = 0; i < m_nPrivPoolCount - 1; i++) {
        for (j = i + 1; j < m_nPrivPoolCount; j++) {
            if (m_tPrivPoolFloorPlan.CommPool[i].BlkSize > m_tPrivPoolFloorPlan.CommPool[j].BlkSize) {
                v.BlkSize = m_tPrivPoolFloorPlan.CommPool[i].BlkSize;
                v.BlkCnt = m_tPrivPoolFloorPlan.CommPool[i].BlkCnt;
                v.MetaSize = m_tPrivPoolFloorPlan.CommPool[i].MetaSize;
                v.CacheMode = m_tPrivPoolFloorPlan.CommPool[i].CacheMode;

                m_tPrivPoolFloorPlan.CommPool[i].BlkSize = m_tPrivPoolFloorPlan.CommPool[j].BlkSize;
                m_tPrivPoolFloorPlan.CommPool[i].BlkCnt = m_tPrivPoolFloorPlan.CommPool[j].BlkCnt;
                m_tPrivPoolFloorPlan.CommPool[i].MetaSize = m_tPrivPoolFloorPlan.CommPool[j].MetaSize;
                m_tPrivPoolFloorPlan.CommPool[i].CacheMode = m_tPrivPoolFloorPlan.CommPool[j].CacheMode;

                m_tPrivPoolFloorPlan.CommPool[j].BlkSize = v.BlkSize;
                m_tPrivPoolFloorPlan.CommPool[j].BlkCnt = v.BlkCnt;
                m_tPrivPoolFloorPlan.CommPool[j].MetaSize = v.MetaSize;
                m_tPrivPoolFloorPlan.CommPool[j].CacheMode = v.CacheMode;
            }
        }
    }

    /* Merge by size */
    for (i = 1, j = 0; i < m_nPrivPoolCount; i++) {
        if (m_tPrivPoolFloorPlan.CommPool[j].BlkSize != m_tPrivPoolFloorPlan.CommPool[i].BlkSize) {
            j += 1;
            m_tPrivPoolFloorPlan.CommPool[j].BlkSize = m_tPrivPoolFloorPlan.CommPool[i].BlkSize;
            m_tPrivPoolFloorPlan.CommPool[j].BlkCnt = m_tPrivPoolFloorPlan.CommPool[i].BlkCnt;
            m_tPrivPoolFloorPlan.CommPool[j].MetaSize = m_tPrivPoolFloorPlan.CommPool[i].MetaSize;
            m_tPrivPoolFloorPlan.CommPool[j].CacheMode = m_tPrivPoolFloorPlan.CommPool[i].CacheMode;
            if (i != j) {
                m_tPrivPoolFloorPlan.CommPool[i].BlkSize = 0;
                m_tPrivPoolFloorPlan.CommPool[i].BlkCnt = 0;
                m_tPrivPoolFloorPlan.CommPool[i].MetaSize = 0;
                m_tPrivPoolFloorPlan.CommPool[i].CacheMode = POOL_CACHE_MODE_NONCACHE;
            }
        } else {
            m_tPrivPoolFloorPlan.CommPool[j].BlkCnt += m_tPrivPoolFloorPlan.CommPool[i].BlkCnt;
            m_tPrivPoolFloorPlan.CommPool[i].BlkSize = 0;
            m_tPrivPoolFloorPlan.CommPool[i].BlkCnt = 0;
            m_tPrivPoolFloorPlan.CommPool[i].MetaSize = 0;
            m_tPrivPoolFloorPlan.CommPool[i].CacheMode = POOL_CACHE_MODE_NONCACHE;
        }
    }

    m_nPrivPoolCount = j + 1;
}

AX_S32 CPoolConfig::InitPrivatePool() {
    /*initialize vin private pool*/
    AX_S32 axRet = AX_VIN_SetPoolAttr(&m_tPrivPoolFloorPlan);

    for (AX_U32 k = 0; k < m_nPrivPoolCount; k++) {
        LOG_M(POOL, "Priv Pool[%d] BlkSize:%d, BlkCnt:%d, MetaSize:%d", k, m_tPrivPoolFloorPlan.CommPool[k].BlkSize,
              m_tPrivPoolFloorPlan.CommPool[k].BlkCnt, m_tPrivPoolFloorPlan.CommPool[k].MetaSize);
    }

    if (0 != axRet) {
        LOG_MM_E(POOL, "AX_VIN_SetPoolAttr fail!Error Code:0x%X\n", axRet);
    } else {
        LOG_MM_I(POOL, "AX_VIN_SetPoolAttr success!\n");
    }
    return axRet;
}

AX_BOOL CPoolConfig::Start() {
    AX_S32 nRet = 0;
    if (AX_FALSE == AddBlocks()) {
        return AX_FALSE;
    }

    nRet = AX_POOL_SetConfig(&m_tCommPoolFloorPlan);

    for (AX_U32 k = 0; k < m_nPoolCount; k++) {
        LOG_M(POOL, "Comm Pool[%d] BlkSize:%d, BlkCnt:%d, MetaSize:%d", k, m_tCommPoolFloorPlan.CommPool[k].BlkSize,
              m_tCommPoolFloorPlan.CommPool[k].BlkCnt, m_tCommPoolFloorPlan.CommPool[k].MetaSize);
    }

    if (nRet) {
        LOG_M_E(POOL, "AX_POOL_SetConfig fail!Error Code:0x%X", nRet);
        return AX_FALSE;
    } else {
        LOG_M_D(POOL, "AX_POOL_SetConfig success!");
    }

    nRet = AX_POOL_Init();
    if (nRet) {
        LOG_M_E(POOL, "AX_POOL_Init fail!!Error Code:0x%X", nRet);
    } else {
        LOG_M_I(POOL, "AX_POOL_Init success!");
    }

    /*initialize private pool*/
    nRet = InitPrivatePool();
    return (nRet == 0) ? AX_TRUE : AX_FALSE;
}

AX_BOOL CPoolConfig::Stop() {
    return AX_TRUE;
}
