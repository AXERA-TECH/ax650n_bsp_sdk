#ifndef __AVS_COMMON_UTILS_H__
#define __AVS_COMMON_UTILS_H__

#include <stdio.h>

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_pool_type.h"

#if 0
#define SAMPLE_DEBUG_LOG(str, arg...)    \
    do {    \
        printf("%s:%d "str"\n", __func__, __LINE__, ##arg); \
    } while(0)
#else
#define SAMPLE_DEBUG_LOG(str, arg...)    \
    do {    \
        ; \
    } while(0)
#endif

#define SAMPLE_LOG(str, arg...)    \
    do {    \
        printf("%s:%d "str"\n", __func__, __LINE__, ##arg); \
    } while(0)

#define SAMPLE_ERR_LOG(str, arg...)   \
    do{  \
        printf("%s:%d Error! "str"\n", __func__, __LINE__, ##arg); \
    }while(0)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define AX_ASSERT(cond, fmt, ...)      do {             \
    if (!(cond)) { printf(fmt, __VA_ARGS__); exit(0); } \
} while(0);

#define AX_MALLOC(ptr, type, num)                       \
    ptr = (type*)malloc(num*sizeof(type));              \
    if (!ptr) {                                         \
        printf("%s(%d): Failed to malloc Ptr", __FILENAME__, __LINE__); \
        exit(0);                                        \
    }                                                   \
    else {                                              \
        memset((AX_U8*)ptr, 0, num*sizeof(type));       \
    }

#define AX_PTR_MALLOC(ptr, type, num)                   \
    type *ptr = (type*)malloc(num*sizeof(type));        \
    if (!ptr) {                                         \
        printf("%s(%d): Failed to malloc Ptr", __FILENAME__, __LINE__); \
        exit(0);                                        \
    }                                                   \
    else {                                              \
        memset((AX_U8*)ptr, 0, num*sizeof(type));       \
    }

#define AX_FREE(ptr, fmt, ...) do {                     \
    if (ptr) {                                          \
        free(ptr);                                      \
        ptr = NULL;                                     \
    }                                                   \
    else {                                              \
        printf(fmt, __VA_ARGS__);                       \
    }                                                   \
}while(0);

#define AX_ALIGN_UP(x,a)    (((x)+(a)-1)&~(a-1))

#define TILE_ALIGN(var, align) ((var + align - 1) & (~(align - 1)))

extern AX_U32 gTileSizeTable[];

AX_POOL PoolInit(AX_U32 frameSize, AX_U32 blkCnt);
AX_POOL CommonPoolInit(AX_U32 frameSize, AX_U32 blkCnt);
AX_U32 GetFrameSize(AX_IMG_FORMAT_E inputFormat, AX_S32 strideSrc, AX_S32 heightSrc);
// To do: support multi plane
AX_U32 LoadFrameFromFbcFile(FILE *pFileIn, AX_S32 widthSrc, AX_S32 strideSrc, AX_S32 heightSrc, AX_IMG_FORMAT_E eFmt, AX_VOID *pVaddr, AX_S32 compress_level);
AX_U32 LoadFrameFromFile(FILE *pFileIn, AX_S32 widthSrc, AX_S32 strideSrc, AX_S32 heightSrc, AX_IMG_FORMAT_E eFmt, AX_VOID *pVaddr);
AX_S32 LoadFileToMem(const AX_CHAR *ps8File, AX_U8 **ppu8Mem, AX_S32 *ps32Len);
AX_S32 LoadMeshFileToMem(const AX_CHAR *pFile, AX_VOID **ppVirAddr, AX_U32 *pMeshTableSize);
void SaveYUV(AX_VIDEO_FRAME_INFO_T *frameInfo, FILE *fp_out);
void SaveFBC(AX_VIDEO_FRAME_INFO_T *frameInfo, FILE *fp_out);
#endif
