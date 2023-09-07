#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "mm_buffer.h"
#include "ax_sys_api.h"

AX_S32 MM_BufferInit(AX_U8 **MM_VirtualAddr, AX_U64 *MM_PhyBaseAddr, AX_U64 Size)
{
    AX_S32 nRet = 0;
    nRet = AX_SYS_MemAlloc(MM_PhyBaseAddr, (AX_VOID**)MM_VirtualAddr, Size, 256, NULL);
    if(nRet != 0) {
        return -1;
    }
    return 0;
}

AX_VOID MM_BufferDeInit(AX_U64 MM_PhyBaseAddr, AX_U8* MM_VirtualAddr) {

    AX_SYS_MemFree(MM_PhyBaseAddr, (AX_VOID**)&MM_VirtualAddr);
    return;
}