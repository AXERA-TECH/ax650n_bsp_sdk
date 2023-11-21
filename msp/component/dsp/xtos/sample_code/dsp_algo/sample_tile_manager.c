/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xtensa/hal.h>
#include <xtensa/xtutil.h>
#include <xtensa/config/core.h>
#include <xtensa/xtbsp.h>
#include <ax_base_type.h>
#include <ax650x_api.h>
#include <xtensa/idma.h>
#include <xtensa/hal.h>
#include <ax_dsp_common.h>
#include "ax_dsp_def.h"
#include "ax_dsp_tm.h"
#include "ax_dsp_trace.h"

#include <xtensa/xtruntime.h>

/* Input/Output dimensions     */
#define IMAGE_W    512
#define IMAGE_H    512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W     128
#define TILE_H     64
#define W_EXT 2
#define H_EXT 2

#define SRC_TILE_BYTES ((TILE_W+2*W_EXT)*(TILE_H+2*H_EXT))
#define DST_TILE_BYTES (SRC_TILE_BYTES*2)
#define MAX_TMPBUF_BYTES (TILE_W*TILE_H)

#define POOL_SIZE                ((SRC_TILE_BYTES + (2*DST_TILE_BYTES) + MAX_TMPBUF_BYTES) + 2048)
#define DMA_DESCR_CNT            (32) // number of DMA descriptors
#if (XCHAL_HW_VERSION_MINOR==1)
    #define MAX_PIF                  (64)
#else
    #define MAX_PIF                  (0)
#endif
#define MAX_BLOCK                (MAX_BLOCK_16)

#define INTERRUPT_ON_COMPLETION  (0)
#define RET_ERROR                (-1)

static AX_VDSP_STS_E GaussianBlur_5x5_U8(AX_VDSP_TILE_S const *src, AX_VDSP_TILE_S const *dst)
{
    int32_t sstride = AX_VDSP_TILE_GET_PITCH(src);
    int32_t dstride = AX_VDSP_TILE_GET_PITCH(dst);
    int32_t width   = AX_VDSP_TILE_GET_WIDTH(src);
    int32_t height  = AX_VDSP_TILE_GET_HEIGHT(src);

    xb_vec2Nx8 *restrict psrc = AX_VDSP_OFFSET_PTR_2NX8(AX_VDSP_TILE_GET_DATA_PTR(src), 0, 0, -2);
    xb_vec2Nx8 *restrict pdst = (xb_vec2Nx8 *)AX_VDSP_TILE_GET_DATA_PTR(dst);
    xb_vec2Nx8 *restrict rdst;
    xb_vec2Nx8 *restrict rsrc;
    valign a_load, a_store = IVP_ZALIGN();

    for (int32_t j = 0; j < width; j += 2 * XCHAL_IVPN_SIMD_WIDTH) {
        xb_vecNx16 pp0_0, pp1_0, pp2_0, pp3_0, pp4_0;
        xb_vecNx16 pp0_1, pp1_1, pp2_1, pp3_1, pp4_1;
        // row 0
        rsrc = AX_VDSP_OFFSET_PTR_2NX8(psrc, -2, sstride, 0);
        {
            xb_vec2Nx8 vsel0, vtail;
            a_load = IVP_LA2NX8U_PP(rsrc);
            IVP_LAV2NX8_XP(vsel0, a_load, rsrc, (width - j) + 4);
            IVP_LAV2NX8_XP(vtail, a_load, rsrc, width - (j - 4) - (2 * XCHAL_IVPN_SIMD_WIDTH));
            xb_vec2Nx24 w1 = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x04060401);
            IVP_ADDWUA2NX8(w1, 0, IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_4));
            pp0_0 = IVP_PACKL2NX24_0(w1);
            pp0_1 = IVP_PACKL2NX24_1(w1);
        }
        // row 1
        rsrc = AX_VDSP_OFFSET_PTR_2NX8(psrc, -1, sstride, 0);
        {
            xb_vec2Nx8 vsel0, vtail;
            a_load = IVP_LA2NX8U_PP(rsrc);
            IVP_LAV2NX8_XP(vsel0, a_load, rsrc, (width - j) + 4);
            IVP_LAV2NX8_XP(vtail, a_load, rsrc, width - (j - 4) - (2 * XCHAL_IVPN_SIMD_WIDTH));
            xb_vec2Nx24 w1 = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x04060401);
            IVP_ADDWUA2NX8(w1, 0, IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_4));
            pp1_0 = IVP_PACKL2NX24_0(w1);
            pp1_1 = IVP_PACKL2NX24_1(w1);
        }
        // row 2
        rsrc = AX_VDSP_OFFSET_PTR_2NX8(psrc, 0, sstride, 0);
        {
            xb_vec2Nx8 vsel0, vtail;
            a_load = IVP_LA2NX8U_PP(rsrc);
            IVP_LAV2NX8_XP(vsel0, a_load, rsrc, (width - j) + 4);
            IVP_LAV2NX8_XP(vtail, a_load, rsrc, width - (j - 4) - (2 * XCHAL_IVPN_SIMD_WIDTH));
            xb_vec2Nx24 w1 = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x04060401);
            IVP_ADDWUA2NX8(w1, 0, IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_4));
            pp2_0 = IVP_PACKL2NX24_0(w1);
            pp2_1 = IVP_PACKL2NX24_1(w1);
        }
        // row 3
        rsrc = AX_VDSP_OFFSET_PTR_2NX8(psrc, 1, sstride, 0);
        {
            xb_vec2Nx8 vsel0, vtail;
            a_load = IVP_LA2NX8U_PP(rsrc);
            IVP_LAV2NX8_XP(vsel0, a_load, rsrc, (width - j) + 4);
            IVP_LAV2NX8_XP(vtail, a_load, rsrc, width - (j - 4) - (2 * XCHAL_IVPN_SIMD_WIDTH));
            xb_vec2Nx24 w1 = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x04060401);
            IVP_ADDWUA2NX8(w1, 0, IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_4));
            pp3_0 = IVP_PACKL2NX24_0(w1);
            pp3_1 = IVP_PACKL2NX24_1(w1);
        }
        int32_t dtail = XT_MIN(width - j, 2 * XCHAL_IVPN_SIMD_WIDTH);
        int32_t stail = XT_MIN(width - j + 4, 4 * XCHAL_IVPN_SIMD_WIDTH);
        int32_t s = sstride - stail;
        int32_t d = dstride - dtail;
        rsrc = AX_VDSP_OFFSET_PTR_2NX8(psrc, (0 + 2), sstride, 0);
        rdst = AX_VDSP_OFFSET_PTR_2NX8(pdst, 0, dstride, 0);

        for (int32_t i = 0; i < height; i++) {
            {
                xb_vec2Nx8 vsel0, vtail;
                a_load = IVP_LA2NX8U_PP(rsrc);
                IVP_LAV2NX8_XP(vsel0, a_load, rsrc, (width - j) + 4);
                IVP_LAV2NX8_XP(vtail, a_load, rsrc, width - (j - 4) - (2 * XCHAL_IVPN_SIMD_WIDTH));
                xb_vec2Nx24 w1 = IVP_MULUS4T2N8XR8(vtail, vsel0, 0x04060401);
                IVP_ADDWUA2NX8(w1, 0, IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_4));
                pp4_0 = IVP_PACKL2NX24_0(w1);
                pp4_1 = IVP_PACKL2NX24_1(w1);
            }

            rsrc = AX_VDSP_OFFSET_PTR_2NX8(rsrc, 1, s, 0);
            pp0_1 = IVP_ADDNX16(pp0_1, pp4_1);
            pp0_0 = IVP_ADDNX16(pp0_0, pp4_0);
            xb_vec2Nx24 w = IVP_MULPI2NR8X16(0x0601, pp2_1, pp2_0, pp0_1, pp0_0);
            IVP_MULUSAI2NX8X16(w, 4, IVP_ADDNX16(pp1_1, pp3_1), IVP_ADDNX16(pp1_0, pp3_0));
            IVP_SAV2NX8U_XP(IVP_PACKVRU2NX24(w, 8), a_store, rdst, (width - j));
            IVP_SAPOS2NX8U_FP(a_store, rdst);
            rdst = AX_VDSP_OFFSET_PTR_2NX8(rdst, 1, d, 0);
            pp0_0 = pp1_0;
            pp1_0 = pp2_0;
            pp2_0 = pp3_0;
            pp3_0 = pp4_0;
            pp0_1 = pp1_1;
            pp1_1 = pp2_1;
            pp2_1 = pp3_1;
            pp3_1 = pp4_1;
        }
        psrc += 1;
        pdst += 1;
    }
    return 0;
}

static int GaussianBlur_5x5_U8_Proc(int64_t gsrc, int64_t gdstX)
{
    AX_S32 srcWidth, srcHeight, srcBytes;
    AX_S32 dstWidth, dstHeight, dstBytes;

    AX_VDSP_PTILE_S srcT[2], dstT[2];
    AX_VDSP_FRAME_S *pInFrame, *pOutFrameX;
    AX_VOID *inTileBuffers[2];
    AX_VOID *outTileBuffers[2];
    AX_S32 retVal, pingPongFlag = 0;
    AX_S32 frameSizeSrc, frameSizeDst;
    AX_S32 tileSizeSrc, tileSizeDst;

    // Tile dimensions
    AX_S32 inTileW, inTileH, inTilePitch;
    AX_S32 outTileW, outTileH, outTilePitch;

    //Indexing
    AX_S32 indx, indy;
    AX_S32 indxIn, indyIn;

    // Set frame dimensions
    srcWidth  = IMAGE_W;
    srcHeight = IMAGE_H;
    srcBytes  = SRC_BIT_DEPTH >> 3;
    dstWidth  = srcWidth;
    dstHeight = srcHeight;
    dstBytes  = DST_BIT_DEPTH >> 3;

    inTileW = TILE_W;
    inTileH = TILE_H;
    inTilePitch = inTileW + 2 * W_EXT;
    outTileW = TILE_W;
    outTileH = TILE_H;
    outTilePitch = outTileW + 2 * W_EXT;

    frameSizeSrc = (srcWidth * srcHeight * srcBytes);
    frameSizeDst = (dstWidth * dstHeight * dstBytes);
    tileSizeSrc  = inTilePitch * (inTileH + 2 * H_EXT);
    tileSizeDst  = (inTilePitch * (outTileH + 2 * H_EXT) * dstBytes);

    AX_VDSP_LOG_DBG("%llx, %llx\n", gsrc, gdstX);
    retVal = AX_VDSP_CreateFrames(&pInFrame, 1, (AX_U64)(gsrc), frameSizeSrc, srcWidth, srcHeight, srcWidth, 1, 1,
                                  FRAME_ZERO_PADDING, 0);
    retVal = AX_VDSP_CreateFrames(&pOutFrameX, 1, (AX_U64)(gdstX), frameSizeDst, dstWidth, dstHeight, dstWidth, 1, 1,
                                  FRAME_ZERO_PADDING, 0);
    retVal = AX_VDSP_AllocateBuffers(inTileBuffers, 2, tileSizeSrc, XV_MEM_BANK_COLOR_0, 64);
    retVal = AX_VDSP_AllocateBuffers(outTileBuffers, 2, tileSizeDst, XV_MEM_BANK_COLOR_0, 64);
    retVal = AX_VDSP_AllocateTiles(srcT, 2);
    retVal = AX_VDSP_AllocateTiles(dstT, 2);
    AX_VDSP_SETUP_TILE(srcT[0], inTileBuffers[0], tileSizeSrc, pInFrame, inTileW, inTileH, inTilePitch, XV_TILE_U8, W_EXT,
                       H_EXT, 0, 0, EDGE_ALIGNED_64);
    AX_VDSP_SETUP_TILE(srcT[1], inTileBuffers[1], tileSizeSrc, pInFrame, inTileW, inTileH, inTilePitch, XV_TILE_U8, W_EXT,
                       H_EXT, 0, 0, EDGE_ALIGNED_64);

    AX_VDSP_SETUP_TILE(dstT[0], outTileBuffers[0], tileSizeDst, pOutFrameX, outTileW, outTileH, outTilePitch,
                       XV_TILE_U8, W_EXT, H_EXT, 0, 0, EDGE_ALIGNED_64);
    AX_VDSP_SETUP_TILE(dstT[1], outTileBuffers[1], tileSizeDst, pOutFrameX, outTileW, outTileH, outTilePitch,
                       XV_TILE_U8, W_EXT, H_EXT, 0, 0, EDGE_ALIGNED_64);

    indxIn = 0;
    indyIn = 0;
    pingPongFlag = 0;

    AX_VDSP_TILE_SET_X_COORD(srcT[pingPongFlag], indxIn);
    AX_VDSP_TILE_SET_Y_COORD(srcT[pingPongFlag], indyIn);

    retVal = AX_VDSP_ReqTileTransferIn(srcT[pingPongFlag], NULL, INTERRUPT_ON_COMPLETION);

    if (retVal == XVTM_ERROR) {
        return (RET_ERROR);
    }
    pingPongFlag = pingPongFlag ^ 0x1;
    indxIn += TILE_W;
    if (indxIn >= srcWidth) {
        indxIn = 0;
        indyIn += TILE_H;
    }

    AX_VDSP_TILE_SET_X_COORD(srcT[pingPongFlag], indxIn);
    AX_VDSP_TILE_SET_Y_COORD(srcT[pingPongFlag], indyIn);
    retVal = AX_VDSP_ReqTileTransferIn(srcT[pingPongFlag], NULL, INTERRUPT_ON_COMPLETION);

    if (retVal == XVTM_ERROR) {
        return (RET_ERROR);
    }
    pingPongFlag = pingPongFlag ^ 0x1;
    indxIn += TILE_W;
    if (indxIn >= srcWidth) {
        indxIn = 0;
        indyIn += TILE_H;
    }

    //loop over input frame tile by tile
    for (indy = 0; indy < srcHeight; indy += TILE_H) {
        for (indx = 0; indx < srcWidth; indx += TILE_W) {
            AX_VDSP_WaitForTile(srcT[pingPongFlag]);
            retVal = GaussianBlur_5x5_U8(srcT[pingPongFlag], dstT[pingPongFlag]);
            if (retVal != 0) {
                AX_VDSP_LOG_ERROR("Error: %x\n", retVal);
            }

            AX_VDSP_TILE_SET_X_COORD(dstT[pingPongFlag], indx);
            AX_VDSP_TILE_SET_Y_COORD(dstT[pingPongFlag], indy);
            retVal = AX_VDSP_ReqTileTransferOut(dstT[pingPongFlag], INTERRUPT_ON_COMPLETION);
            if (retVal == XVTM_ERROR) {
                AX_VDSP_LOG_ERROR("Error: %x\n", retVal);
                return (RET_ERROR);
            }
            if (indyIn < srcHeight) {
                AX_VDSP_TILE_SET_X_COORD(srcT[pingPongFlag], indxIn);
                AX_VDSP_TILE_SET_Y_COORD(srcT[pingPongFlag], indyIn);
                retVal = AX_VDSP_ReqTileTransferIn(srcT[pingPongFlag], NULL, INTERRUPT_ON_COMPLETION);
                if (retVal == XVTM_ERROR) {
                    AX_VDSP_LOG_ERROR("Error: %x\n", retVal);
                    return (RET_ERROR);
                }
                indxIn += TILE_W;
                if (indxIn >= srcWidth) {
                    indxIn = 0;
                    indyIn += TILE_H;
                }
            }
            pingPongFlag = pingPongFlag ^ 0x1;
        }
    }
    AX_VDSP_WaitForTile(dstT[pingPongFlag ^ 0x01]);
    AX_VDSP_FreeBuffers(inTileBuffers, 2);
    AX_VDSP_FreeBuffers(outTileBuffers, 2);
    AX_VDSP_FreeTiles(srcT, 2);
    AX_VDSP_FreeTiles(dstT, 2);
    AX_VDSP_FreeFrames(&pInFrame, 1);
    AX_VDSP_FreeFrames(&pOutFrameX, 1);
    return 0;
}

int AX_DSP_AlgoGaussianBlur(AX_DSP_MESSAGE_T *msg)
{
    AX_U64 src;
    AX_U64 dst;
    AX_VDSP_LOG_DBG("%x, %x, %x, %x\r\n", msg->u32Body[0], msg->u32Body[1], msg->u32Body[2],
                    msg->u32Body[3]);
    src = msg->u32Body[0];
    src |= ((AX_U64)msg->u32Body[1]) << 32;
    dst = msg->u32Body[2];
    dst |= ((AX_U64)msg->u32Body[3]) << 32;
    GaussianBlur_5x5_U8_Proc(src, dst);
    return 0;
}
