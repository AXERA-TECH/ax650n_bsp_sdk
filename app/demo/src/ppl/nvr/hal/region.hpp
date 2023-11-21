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

#include <map>
#include <mutex>
#include <vector>
#include "AXResource.hpp"
#include "AXSingleton.h"
#include "AXThread.hpp"
#include "IObserver.h"
#include "ax_base_type.h"
#include "ax_ivps_api.h"
#include "detectResult.h"
#include "haltype.hpp"
#include "vo.hpp"
#include "framebufferPaint.hpp"

#define APP_REGION_MAX_THREAD (10)
#define APP_REGION_INVALID_VALUE (-1)


enum class AX_NVR_RGN_TYPE {
    IVPS    = 0,
    VOFB    = 1,
    BUTT    = 99,
};

class CRegion;
typedef struct _NVR_REGION_ATTR_T {
    AX_S32 nGroup{APP_REGION_INVALID_VALUE};
    AX_S32 nChn{APP_REGION_INVALID_VALUE};
    AX_U32 nWidth{0};   // ivps channel out resolution
    AX_U32 nHeight{0};  // ivps channel out resolution
    AX_U32 nSrcW{0};    // ivps group in resolution
    AX_U32 nSrcH{0};    // ivps group in resolution

    AX_NVR_RGN_TYPE enType = AX_NVR_RGN_TYPE::BUTT;
    CVO *pVo = nullptr;
    CFramebufferPaint *pFb = nullptr;
    VO_LAYER nVoLayer = (AX_U32)-1;
    VO_CHN nVoChannel = (AX_U32)-1;
    AX_U32 nW;
    AX_U32 nH;
} NVR_REGION_ATTR_T;

typedef struct _NVR_REGION_TASK_CFG_T : public NVR_REGION_ATTR_T {
    IVPS_RGN_HANDLE nRgnHandle{APP_REGION_INVALID_VALUE};
    CRegion* pRegion{nullptr};
    AX_VOID setTaskCfg(IVPS_RGN_HANDLE rgn, NVR_REGION_ATTR_T attr, CRegion* pRgn) {
        nGroup = attr.nGroup;
        nChn = attr.nChn;
        nWidth = attr.nWidth;
        nHeight = attr.nHeight;
        nSrcW = attr.nSrcW;
        nSrcH = attr.nSrcH;

        enType = attr.enType;
        pVo = attr.pVo;
        pFb = attr.pFb;
        nVoLayer = attr.nVoLayer;
        nVoChannel = attr.nVoChannel;
        nW = attr.nW;
        nH = attr.nH;
        nRgnHandle = rgn;
        pRegion = pRgn;
    }
} NVR_REGION_TASK_CFG_T;

class CRegion {
public:
    static CRegion* CreateInstance(CONST NVR_REGION_ATTR_T& stAttr);
    CRegion(AX_VOID) = default;
    AX_VOID Destory(AX_VOID);

    AX_BOOL Start();
    AX_BOOL Stop();

    AX_BOOL EnableAiRegion(AX_BOOL bEnable = AX_TRUE);
    AX_VOID ClearAiRegion();

    AX_BOOL SetDetectedRect(DETECT_RESULT_T& stRect);
    AX_VOID GetDetectedRect(DETECT_RESULT_T& stRect);

    AX_BOOL GetRectDisp(const NVR_REGION_TASK_CFG_T &attr, AX_IVPS_RGN_DISP_GROUP_T& disp);

    AX_BOOL Init(NVR_REGION_ATTR_T stAttr);
    AX_BOOL DeInit();

    NVR_REGION_TASK_CFG_T m_RgnAttr;

private:
    CRegion(CONST CRegion&) = delete;
    CRegion& operator=(CONST CRegion&) = delete;


private:
    std::mutex m_mtxRect;
    DETECT_RESULT_T m_stRect;
    AX_BOOL m_bStarted = AX_FALSE;
};

class CRegionTask : public CAXSingleton<CRegionTask> {
    friend class CAXSingleton<CRegionTask>;
    friend class CRegion;

public:
    AX_BOOL RegisterTask(const NVR_REGION_TASK_CFG_T& stAttr);
    AX_VOID UnRegisterTask(const NVR_REGION_TASK_CFG_T& stAttr);
    AX_BOOL UpdateRegionAttr(const NVR_REGION_TASK_CFG_T& stAttr);
    AX_VOID Stop(AX_VOID);
    AX_VOID Start(AX_VOID);

protected:
    virtual AX_BOOL InitOnce() override;

private:
    AX_VOID UpdateThread(AX_VOID* pArg);
    AX_BOOL CreateThread(AX_VOID);
    AX_VOID DestoryThread(AX_VOID);

private:
    std::vector<NVR_REGION_TASK_CFG_T> m_vecRgnAttr;

    CAXThread m_thread[APP_REGION_MAX_THREAD];

    AX_BOOL m_bStarted = {AX_FALSE};
    AX_U32 m_nTotalThreadNum{0};

    std::mutex m_mtxVec;
};

class CRegionObserver : public IObserver {
public:
    CRegionObserver(AX_VOID) = default;
    CRegionObserver(CRegion* pSink) : m_pSink(pSink){};
    AX_VOID SetSink(CRegion* pSink) {m_pSink = pSink;};

    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        DETECT_RESULT_T tRect = *(DETECT_RESULT_T*)pData;
        if (!m_pSink) {
            return AX_FALSE;
        }

        m_pSink->SetDetectedRect(tRect);
        return AX_TRUE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E, AX_U32, AX_U32, OBS_TRANS_ATTR_PTR pParams) override {
        return AX_TRUE;
    }

private:
    CRegion* m_pSink{nullptr};
};
