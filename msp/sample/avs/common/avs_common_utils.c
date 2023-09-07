#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "ax_sys_api.h"
#include "avs_common_utils.h"

AX_U32 GetFrameSize(AX_IMG_FORMAT_E inputFormat, AX_S32 strideSrc, AX_S32 heightSrc) {
    AX_U32 frameSize = 0;

    switch (inputFormat)
    {
    case AX_FORMAT_YUV420_PLANAR: // i420
        frameSize = strideSrc * heightSrc * 3 / 2;
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR: // nv12
        frameSize = strideSrc * heightSrc * 3 / 2;
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR_VU: // nv21
        frameSize = strideSrc * heightSrc * 3 / 2;
        break;
    case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
        frameSize = strideSrc * heightSrc * 2;
        break;
    case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
        frameSize = strideSrc * heightSrc * 2;
        break;
    default:
        SAMPLE_ERR_LOG("Invalid frame format!\n");
    }

    return frameSize;
}

AX_POOL PoolInit(AX_U32 frameSize, AX_U32 blkCnt)
{
    /* use pool to alloc buffer */
    AX_POOL_CONFIG_T stPoolConfig;
    AX_POOL s32UserPoolId;

    if (0 == frameSize) {
        SAMPLE_ERR_LOG("Forbid 0 == frameSize.\n");
        return AX_INVALID_POOLID;
    }

    memset(&stPoolConfig, 0, sizeof(AX_POOL_CONFIG_T));
    stPoolConfig.MetaSize = 512;
    stPoolConfig.BlkCnt = blkCnt;
    stPoolConfig.BlkSize = frameSize;
    stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
    memset(stPoolConfig.PartitionName, 0, sizeof(stPoolConfig.PartitionName));
    strcpy(stPoolConfig.PartitionName, "anonymous");

    s32UserPoolId = AX_POOL_CreatePool(&stPoolConfig);
    if (AX_INVALID_POOLID == s32UserPoolId) {
        SAMPLE_ERR_LOG("Create pool err! frameSize:0x%x .BlkSize:%lld .BlkCnt:%d.\n",
            frameSize, stPoolConfig.BlkSize, stPoolConfig.BlkCnt);
    }

    SAMPLE_DEBUG_LOG("FrameSize:0x%x .BlkSize:%lld .BlkCnt:%d s32UserPoolId:%d.\n",
        frameSize, stPoolConfig.BlkSize, stPoolConfig.BlkCnt, s32UserPoolId);
    return s32UserPoolId;
}

AX_POOL CommonPoolInit(AX_U32 frameSize, AX_U32 blkCnt)
{
	AX_S32 ret=0,i=0;
	AX_POOL_FLOORPLAN_T PoolFloorPlan;
	AX_U64 BlkSize;
	AX_BLK BlkId;
	AX_POOL PoolId;
	AX_U64 PhysAddr,MetaPhysAddr;
	AX_VOID *blockVirAddr,*metaVirAddr;

	AX_POOL_CONFIG_T PoolConfig;
	AX_POOL UserPoolId0,UserPoolId1,UserPoolId2;

	SAMPLE_DEBUG_LOG("sample_pool test begin\n\n");

	AX_SYS_Init();

	ret = AX_POOL_Exit();

	if(ret){
		SAMPLE_ERR_LOG("AX_POOL_Exit fail!!Error Code:0x%X\n",ret);
		return -1;
	}else{
		SAMPLE_DEBUG_LOG("AX_POOL_Exit success!\n");
	}

	memset(&PoolFloorPlan, 0, sizeof(AX_POOL_FLOORPLAN_T));
	PoolFloorPlan.CommPool[0].MetaSize   = 0x1000;
	PoolFloorPlan.CommPool[0].BlkSize   = frameSize;
	PoolFloorPlan.CommPool[0].BlkCnt    = blkCnt;
	PoolFloorPlan.CommPool[0].CacheMode = POOL_CACHE_MODE_NONCACHE;

	memset(PoolFloorPlan.CommPool[0].PartitionName,0,sizeof(PoolFloorPlan.CommPool[0].PartitionName));
	strcpy((char *)PoolFloorPlan.CommPool[0].PartitionName,"anonymous");

	ret = AX_POOL_SetConfig(&PoolFloorPlan);

	if(ret){
		SAMPLE_ERR_LOG("AX_POOL_SetConfig fail!Error Code:0x%X\n",ret);
		return -1;
	}else{
		SAMPLE_DEBUG_LOG("AX_POOL_SetConfig success!\n");
	}

	ret = AX_POOL_Init();
	if(ret){
		SAMPLE_ERR_LOG("AX_POOL_Init fail!!Error Code:0x%X, frame size %d, blk cnt %d\n",ret, frameSize, blkCnt);
		return -1;
	}else{
		SAMPLE_DEBUG_LOG("AX_POOL_Init success!\n");
	}
    SAMPLE_LOG("Init common pool, blk size %d\n", frameSize);
    return 0;
}

AX_U32 LoadFrameFromFile(FILE *pFileIn, AX_S32 widthSrc, AX_S32 strideSrc, AX_S32 heightSrc, AX_IMG_FORMAT_E eFmt, AX_VOID *pVaddr)
{
    AX_U32 i, rows, realRead, readSize;

    if (!pFileIn)
        return -1;
    if (!pVaddr)
        return -1;

    readSize = 0;
    realRead = 0;

    switch(eFmt) {
        case AX_FORMAT_YUV420_PLANAR:
            rows = heightSrc;
            for (i = 0; i < rows; i++) {
                realRead = fread(pVaddr, 1, strideSrc, pFileIn);
                if (realRead < strideSrc)
                    break;
                readSize += realRead;
                pVaddr += strideSrc;
            }
            rows = heightSrc;
            for (i = 0; i < rows; i++) {
                realRead = fread(pVaddr, 1, strideSrc >> 1, pFileIn);
                if (realRead < (strideSrc >> 1))
                    break;
                readSize += realRead;
                pVaddr += (strideSrc >> 1);
            }
            break;
        case AX_FORMAT_YUV420_SEMIPLANAR:
        case AX_FORMAT_YUV420_SEMIPLANAR_VU:
            rows = heightSrc * 3 / 2;
            for (i = 0; i < rows; i++) {
                realRead = fread(pVaddr, 1, strideSrc, pFileIn);
                if (realRead < strideSrc)
                    break;
                readSize += realRead;
                pVaddr += strideSrc;
            }
            break;
        case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
        case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
            rows = heightSrc;
            for (i = 0; i < rows; i++) {
                realRead = fread(pVaddr, 1, strideSrc, pFileIn);
                if (realRead < strideSrc)
                    break;
                readSize += realRead;
                pVaddr += strideSrc;
            }
            break;
        default:
            SAMPLE_ERR_LOG("Invalid format, eFmt = %d\n", eFmt);
    }

    return readSize;
}

AX_S32 LoadFileToMem(const AX_CHAR *ps8File, AX_U8 **ppu8Mem, AX_S32 *ps32Len)
{
    /* Reading input file */
    FILE *f_in = fopen(ps8File, "rb");
    if (f_in == NULL) {
        SAMPLE_ERR_LOG("Unable to open input file, file name %s\n", ps8File);
        return -1;
    }

    /* file i/o pointer to full */
    fseek(f_in, 0L, SEEK_END);
    *ps32Len = ftell(f_in);
    rewind(f_in);

    *ppu8Mem = malloc(sizeof(AX_U8) * (*ps32Len));
    assert(*ppu8Mem != NULL);

    /* read input stream from file to buffer and close input file */
    if (fread(*ppu8Mem, sizeof(AX_U8), *ps32Len, f_in) != *ps32Len) {
        SAMPLE_ERR_LOG("fread error\n");
        free(*ppu8Mem);
        *ppu8Mem = NULL;
        fclose(f_in);
        return -1;
    }

    fclose(f_in);
    return 0;
}

AX_S32 LoadMeshFileToMem(const AX_CHAR *pFile, AX_VOID **ppVirAddr, AX_U32 *pMeshTableSize)
{
    AX_U64 *pData;

    FILE *fp = fopen(pFile, "r");
    if (!fp) {
        SAMPLE_ERR_LOG("open %s fail, %s", pFile, strerror(errno));
        return AX_FALSE;
    }

    fseek(fp, 0, SEEK_END);
    AX_U32 nFileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (0 == nFileSize) {
        SAMPLE_ERR_LOG("nFileSize is 0");
        fclose(fp);
        return AX_FALSE;
    }

    *ppVirAddr = malloc(sizeof(AX_U8) * nFileSize);
    assert(*ppVirAddr != NULL);

    AX_CHAR szLine[64] = {0};
    AX_U64 nValue = 0;
    AX_U32 nCount = 0;

    pData = (AX_U64 *)*ppVirAddr;
    while (fgets(szLine, 64, fp)) {
        sscanf(szLine, "%llx", &nValue);
        pData[nCount++] = nValue;
    }

    *pMeshTableSize = nCount * 8;

    fclose(fp);

    return 0;
}


void SaveYUV(AX_VIDEO_FRAME_INFO_T *frameInfo, FILE *fp_out)
{
    AX_U32 i;
    AX_VOID *p_lu = NULL;
    AX_VOID *p_ch = NULL;
    AX_U64 lu_buss = 0;
    AX_U64 ch_buss = 0;
    AX_S32 s32Ret = 0;
    AX_VOID *pLumaVirAddr = NULL;
    AX_VOID *pChromaVirAddr = NULL;
    AX_U32 lumaMapSize = 0;
    AX_U32 chromaMapSize = 0;

    lumaMapSize = frameInfo->stVFrame.u32PicStride[0] * AX_ALIGN_UP(frameInfo->stVFrame.u32Height, 16);
    pLumaVirAddr = AX_SYS_Mmap(frameInfo->stVFrame.u64PhyAddr[0], lumaMapSize);

    if (!pLumaVirAddr) {
        SAMPLE_ERR_LOG("SaveYUV:AX_SYS_Mmap luma failed, pLumaPhyAddr=%lld,lumaMapSize=%d\n", frameInfo->stVFrame.u64PhyAddr[0], lumaMapSize);
        return;
    } else {
        SAMPLE_DEBUG_LOG("SaveYUV:AX_SYS_Mmap luma success,pLumaVirAddr=%p,lumaMapSize=%d\n", pLumaVirAddr, lumaMapSize);
    }

    chromaMapSize = frameInfo->stVFrame.u32PicStride[0] * AX_ALIGN_UP(frameInfo->stVFrame.u32Height, 16) / 2;
    pChromaVirAddr = AX_SYS_Mmap(frameInfo->stVFrame.u64PhyAddr[1], chromaMapSize);

    if (!pChromaVirAddr) {
        SAMPLE_ERR_LOG("SaveYUV:AX_SYS_Mmap chroma failed\n");
        goto END;
    } else {
        SAMPLE_DEBUG_LOG("SaveYUV:AX_SYS_Mmap chroma success,pChromaVirAddr=%p,chromaMapSize=%d\n", pChromaVirAddr, chromaMapSize);
    }

    p_lu = pLumaVirAddr;
    lu_buss = frameInfo->stVFrame.u64PhyAddr[0];
    p_ch = pChromaVirAddr;
    ch_buss = frameInfo->stVFrame.u64PhyAddr[1];

    SAMPLE_DEBUG_LOG("p_lu: %p\n", p_lu);
    SAMPLE_DEBUG_LOG("lu_buss: 0x%llx\n", lu_buss);
    SAMPLE_DEBUG_LOG("p_ch: %p\n", p_ch);
    SAMPLE_DEBUG_LOG("ch_buss: 0x%llx\n", ch_buss);

    AX_U32 coded_width = frameInfo->stVFrame.u32Width;
    AX_U32 coded_height = frameInfo->stVFrame.u32Height;
    AX_U32 pic_stride = frameInfo->stVFrame.u32PicStride[0];
    AX_U32 coded_width_ch = frameInfo->stVFrame.u32Width;
    AX_U32 coded_h_ch = frameInfo->stVFrame.u32Height / 2;
    AX_U32 pic_stride_ch = frameInfo->stVFrame.u32PicStride[1];
    if (pic_stride_ch == 0) {
        pic_stride_ch = pic_stride;
    }
    SAMPLE_DEBUG_LOG("SaveYUV:p_lu: %p, p_ch: %p, coded_width: %u, coded_height: %u, pic_stride: %u, "
           "coded_width_ch: %u, coded_h_ch: %u, pic_stride_ch: %u, pixel_bytes: %u\n",
           p_lu, p_ch, coded_width, coded_height, pic_stride, coded_width_ch, coded_h_ch, pic_stride_ch, 1);


    SAMPLE_DEBUG_LOG("write Y\n");
    for (i = 0; i < coded_height; i++) {
        fwrite(p_lu, 1, coded_width, fp_out);
        p_lu += pic_stride;
    }

    SAMPLE_DEBUG_LOG("write UV\n");
    for (i = 0; i < coded_h_ch; i++) {
        fwrite(p_ch, 1, coded_width_ch, fp_out);
        p_ch += pic_stride_ch;
    }


END:
    if (pLumaVirAddr) {
        s32Ret = AX_SYS_Munmap(pLumaVirAddr, lumaMapSize);

        if (s32Ret) {
            SAMPLE_ERR_LOG("SaveYUV:AX_SYS_Munmap luma failed,s32Ret=0x%x\n", s32Ret);
        } else {
            SAMPLE_DEBUG_LOG("SaveYUV:AX_SYS_Munmap luma success,pLumaVirAddr=%p,lumaMapSize=%d\n", pLumaVirAddr, lumaMapSize);
        }
    }

    if (pChromaVirAddr) {
        s32Ret = AX_SYS_Munmap(pChromaVirAddr, chromaMapSize);

        if (s32Ret) {
            SAMPLE_ERR_LOG("SaveYUV:AX_SYS_Munmap chroma failed,s32Ret=0x%x\n", s32Ret);
        } else {
            SAMPLE_DEBUG_LOG("SaveYUV:AX_SYS_Munmap chroma success,pChromaVirAddr=%p,chromaMapSize=%d\n", pChromaVirAddr, chromaMapSize);
        }
    }

    SAMPLE_DEBUG_LOG("write end\n");
}
