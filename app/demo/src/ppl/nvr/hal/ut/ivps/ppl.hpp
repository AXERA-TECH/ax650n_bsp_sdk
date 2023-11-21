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
#include <string>
#include "ffmpegstream.hpp"
#include "help.hpp"
#include "ivps.hpp"
#include "linker.hpp"
#include "rtspstream.hpp"
#include "vdec.hpp"
#include "vo.hpp"

typedef struct {
    AX_U32 voChn; /* video chn id of VO */
    AX_IVPS_ENGINE_E enEngine;
    INPUT_TYPE_E enInput;
    std::string strUrl;
    AX_U32 playBackMode;
    AX_U32 ppVBCnt;
    AX_U32 ppDepth;
    AX_S32 nFps;
    AX_S32 nTimeOut;
    AX_U32 ivpsInDepth;
    AX_U32 ivpsBufCnt;
    CVO *vo;
    AX_VO_CHN_ATTR_T voChnAttr;
} PPL_ATTR_T;

/* PPL linked: VDEC (pp0) -> IVPS -> VO */
class PPL {
public:
    PPL(AX_VOID) = default;

    AX_BOOL Create(const PPL_ATTR_T &stAttr);
    AX_BOOL Destory(AX_VOID);

protected:
    AX_BOOL CreateStream(AX_VOID);
    AX_BOOL DestoryStream(AX_VOID);

    AX_BOOL CreateVDEC(AX_VOID);
    AX_BOOL DestoryVDEC(AX_VOID);

    AX_BOOL CreateIVPS(AX_VOID);
    AX_BOOL DestoryIVPS(AX_VOID);

    AX_BOOL SetupLink(AX_VOID);
    AX_BOOL DestroyLink(AX_VOID);

private:
    PPL_ATTR_T m_stAttr;
    IStream *m_stream = {nullptr};
    CVDEC *m_vdec = {nullptr};
    CIVPS *m_ivps = {nullptr};
    CVO *m_vo = {nullptr};
    CLinker m_linker;
};
