/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <cstdio>
#include <cstring>
#include <vector>
#include <utility>

#include <ax_sys_api.h>
#include <ax_engine_api.h>

#define AX_CMM_ALIGN_SIZE 128

const char* AX_CMM_SESSION_NAME = "npu";

typedef enum
{
    AX_ENGINE_ABST_DEFAULT = 0,
    AX_ENGINE_ABST_CACHED = 1,
} AX_ENGINE_ALLOC_BUFFER_STRATEGY_T;


typedef std::pair<AX_ENGINE_ALLOC_BUFFER_STRATEGY_T, AX_ENGINE_ALLOC_BUFFER_STRATEGY_T> INPUT_OUTPUT_ALLOC_STRATEGY;

#define SAMPLE_AX_ENGINE_DEAL_HANDLE            \
    if (0 != ret)                               \
    {                                           \
        return AX_ENGINE_DestroyHandle(handle); \
    }

#define SAMPLE_AX_ENGINE_DEAL_HANDLE_IO         \
    if (0 != ret)                               \
    {                                           \
        middleware::free_io(&io_data);          \
        return AX_ENGINE_DestroyHandle(handle); \
    }

namespace middleware
{

    void free_io_index(AX_ENGINE_IO_BUFFER_T* io_buf, size_t index)
    {
        for (size_t i = 0; i < index; ++i)
        {
            AX_ENGINE_IO_BUFFER_T* pBuf = io_buf + i;
            AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
        }
    }

    void free_io(AX_ENGINE_IO_T* io)
    {
        for (size_t j = 0; j < io->nInputSize; ++j)
        {
            AX_ENGINE_IO_BUFFER_T* pBuf = io->pInputs + j;
            AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
        }
        for (size_t j = 0; j < io->nOutputSize; ++j)
        {
            AX_ENGINE_IO_BUFFER_T* pBuf = io->pOutputs + j;
            AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
        }
        delete[] io->pInputs;
        delete[] io->pOutputs;
    }

    static inline int prepare_io(AX_ENGINE_IO_INFO_T* info, AX_ENGINE_IO_T* io_data, INPUT_OUTPUT_ALLOC_STRATEGY strategy)
    {
        memset(io_data, 0, sizeof(*io_data));
        io_data->pInputs = new AX_ENGINE_IO_BUFFER_T[info->nInputSize];
        io_data->nInputSize = info->nInputSize;

        auto ret = 0;
        for (AX_U32 i = 0; i < info->nInputSize; ++i)
        {
            auto meta = info->pInputs[i];
            auto buffer = &io_data->pInputs[i];
            if (strategy.first == AX_ENGINE_ABST_CACHED)
            {
                ret = AX_SYS_MemAllocCached((AX_U64*)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8*)(AX_CMM_SESSION_NAME));
            }
            else
            {
                ret = AX_SYS_MemAlloc((AX_U64*)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8*)(AX_CMM_SESSION_NAME));
            }

            if (ret != 0)
            {
                free_io_index(io_data->pInputs, i);
                fprintf(stderr, "Allocate input{%d} { phy: %p, vir: %p, size: %lu Bytes }. fail \n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
                return ret;
            }
            // fprintf(stderr, "Allocate input{%d} { phy: %p, vir: %p, size: %lu Bytes }. \n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
        }

        io_data->pOutputs = new AX_ENGINE_IO_BUFFER_T[info->nOutputSize];
        io_data->nOutputSize = info->nOutputSize;
        for (AX_U32 i = 0; i < info->nOutputSize; ++i)
        {
            auto meta = info->pOutputs[i];
            auto buffer = &io_data->pOutputs[i];
            buffer->nSize = meta.nSize;
            if (strategy.second == AX_ENGINE_ABST_CACHED)
            {
                ret = AX_SYS_MemAllocCached((AX_U64*)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8*)(AX_CMM_SESSION_NAME));
            }
            else
            {
                ret = AX_SYS_MemAlloc((AX_U64*)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8*)(AX_CMM_SESSION_NAME));
            }
            if (ret != 0)
            {
                fprintf(stderr, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes }. fail \n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
                free_io_index(io_data->pInputs, io_data->nInputSize);
                free_io_index(io_data->pOutputs, i);
                return ret;
            }
            // fprintf(stderr, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes }.\n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
        }

        return 0;
    }

    static int push_input(const std::vector<uint8_t>& data, AX_ENGINE_IO_T* io_t, AX_ENGINE_IO_INFO_T* info_t)
    {
        if (info_t->nInputSize != 1)
        {
            fprintf(stderr, "Only support Input size == 1 current now");
            return -1;
        }

        if (data.size() != info_t->pInputs[0].nSize)
        {
            fprintf(stderr, "The input data size is not matched with tensor {name: %s, size: %d}.\n", info_t->pInputs[0].pName, info_t->pInputs[0].nSize);
            return -1;
        }

        memcpy(io_t->pInputs[0].pVirAddr, data.data(), data.size());

        return 0;
    }
} // namespace middleware
