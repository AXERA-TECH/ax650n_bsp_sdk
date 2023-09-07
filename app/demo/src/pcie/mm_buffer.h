#ifndef MM_BUFFER_H__
#define MM_BUFFER_H__

#include "ax_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

AX_S32 MM_BufferInit(AX_U8 **MM_VirtualAddr, AX_U64 *MM_PhyBaseAddr, AX_U64 Size);

AX_VOID MM_BufferDeInit(AX_U64 PhyAddr, AX_U8* MM_VirtualAddr);

#ifdef __cplusplus
}
#endif
#endif