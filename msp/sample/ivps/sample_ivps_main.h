/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_IVPS_MAIN_H_
#define _SAMPLE_IVPS_MAIN_H_

#include "ivps_util.h"
#include "ivps_help.h"
#include "ivps_parser.h"
#include "sample_ivps_venc.h"
#include "sample_ivps_pipeline.h"
#include "sample_ivps_region.h"
#include "sample_ivps_dewarp.h"
#include "sample_ivps_sync_api.h"

typedef enum
{
    IVPS_LINK_NULL,
    IVPS_LINK_IVPS,
    IVPS_LINK_VENC,
    IVPS_LINK_JENC,
    IVPS_LINK_BUTT
} IVPS_LINK_T;

typedef struct
{
    IVPS_ARG_T tIvpsArg;
} SAMPLE_IVPS_MAIN_T;

AX_S32 SAMPLE_IVPS_LinkVenc(SAMPLE_IVPS_GRP_T *pGrp, AX_BOOL bVencMode);
AX_S32 SAMPLE_IVPS_LinkIvps(AX_S32 nGrpIdx, AX_S32 nChnIdx, SAMPLE_IVPS_GRP_T *pGrp);
AX_S32 SAMPLE_IVPS_SyncApi(const IVPS_ARG_T *ptArg, const SAMPLE_IVPS_GRP_T *pGrp,
                           const SAMPLE_IVPS_SYNC_API_T *ptSyncIntf);
AX_S32 SAMPLE_IVPS_SyncApiRegion(const IVPS_ARG_T *ptArg, const SAMPLE_IVPS_GRP_T *pGrp,
                                 const SAMPLE_IVPS_REGION_T *ptRegion);
#endif /* _SAMPLE_IVPS_MAIN_H_ */
