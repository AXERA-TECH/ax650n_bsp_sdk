#include "sample_engine.h"

#include "detection.hpp"

#include "string.h"
#include "stdio.h"
#include "vector"
#include "fstream"

#include "ax_engine_api.h"

union ax_abgr_t
{
    unsigned char abgr[4];
    int iargb;

    ax_abgr_t()
    {
        iargb = 0;
    }

    ax_abgr_t(unsigned char a, unsigned char b, unsigned char g, unsigned char r)
    {
        abgr[0] = 0;
        abgr[1] = b;
        abgr[2] = g;
        abgr[3] = r;
    }
};

static const std::vector<ax_abgr_t> COCO_COLORS_ARGB = {
    {0, 255, 0, 255}, {128, 226, 255, 0}, {128, 0, 94, 255}, {128, 0, 37, 255}, {128, 0, 255, 94}, {128, 255, 226, 0}, {128, 0, 18, 255}, {128, 255, 151, 0}, {128, 170, 0, 255}, {128, 0, 255, 56}, {128, 255, 0, 75}, {128, 0, 75, 255}, {128, 0, 255, 169}, {128, 255, 0, 207}, {128, 75, 255, 0}, {128, 207, 0, 255}, {128, 37, 0, 255}, {128, 0, 207, 255}, {128, 94, 0, 255}, {128, 0, 255, 113}, {128, 255, 18, 0}, {128, 255, 0, 56}, {128, 18, 0, 255}, {128, 0, 255, 226}, {128, 170, 255, 0}, {128, 255, 0, 245}, {128, 151, 255, 0}, {128, 132, 255, 0}, {128, 75, 0, 255}, {128, 151, 0, 255}, {128, 0, 151, 255}, {128, 132, 0, 255}, {128, 0, 255, 245}, {128, 255, 132, 0}, {128, 226, 0, 255}, {128, 255, 37, 0}, {128, 207, 255, 0}, {128, 0, 255, 207}, {128, 94, 255, 0}, {128, 0, 226, 255}, {128, 56, 255, 0}, {128, 255, 94, 0}, {128, 255, 113, 0}, {128, 0, 132, 255}, {128, 255, 0, 132}, {128, 255, 170, 0}, {128, 255, 0, 188}, {128, 113, 255, 0}, {128, 245, 0, 255}, {128, 113, 0, 255}, {128, 255, 188, 0}, {128, 0, 113, 255}, {128, 255, 0, 0}, {128, 0, 56, 255}, {128, 255, 0, 113}, {128, 0, 255, 188}, {128, 255, 0, 94}, {128, 255, 0, 18}, {128, 18, 255, 0}, {128, 0, 255, 132}, {128, 0, 188, 255}, {128, 0, 245, 255}, {128, 0, 169, 255}, {128, 37, 255, 0}, {128, 255, 0, 151}, {128, 188, 0, 255}, {128, 0, 255, 37}, {128, 0, 255, 0}, {128, 255, 0, 170}, {128, 255, 0, 37}, {128, 255, 75, 0}, {128, 0, 0, 255}, {128, 255, 207, 0}, {128, 255, 0, 226}, {128, 255, 245, 0}, {128, 188, 255, 0}, {128, 0, 255, 18}, {128, 0, 255, 75}, {128, 0, 255, 151}, {128, 255, 56, 0}, {128, 245, 255, 0}};

static const char *CLASS_NAMES[] = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"};

const float ANCHORS[18] = {10, 13, 16, 30, 33, 23, 30, 61, 62, 45, 59, 119, 116, 90, 156, 198, 373, 326};

const float PROB_THRESHOLD = 0.3f;
const float NMS_THRESHOLD = 0.45f;

#define AX_CMM_ALIGN_SIZE 128

const char *AX_CMM_SESSION_NAME = "npu";

const std::vector<std::string> WANT_CLASSES = {"person"};

typedef enum
{
    AX_ENGINE_ABST_DEFAULT = 0,
    AX_ENGINE_ABST_CACHED = 1,
} AX_ENGINE_ALLOC_BUFFER_STRATEGY_T;

typedef std::pair<AX_ENGINE_ALLOC_BUFFER_STRATEGY_T, AX_ENGINE_ALLOC_BUFFER_STRATEGY_T> INPUT_OUTPUT_ALLOC_STRATEGY;

static bool read_file(const std::string &path, std::vector<char> &data)
{
    std::fstream fs(path, std::ios::in | std::ios::binary);

    if (!fs.is_open())
    {
        return false;
    }

    fs.seekg(std::ios::end);
    auto fs_end = fs.tellg();
    fs.seekg(std::ios::beg);
    auto fs_beg = fs.tellg();

    auto file_size = static_cast<size_t>(fs_end - fs_beg);
    auto vector_size = data.size();

    data.reserve(vector_size + file_size);
    data.insert(data.end(), std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());

    fs.close();

    return true;
}

static void free_io_index(AX_ENGINE_IO_BUFFER_T *io_buf, int index)
{
    for (int i = 0; i < index; ++i)
    {
        AX_ENGINE_IO_BUFFER_T *pBuf = io_buf + i;
        AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
    }
}

static void free_io(AX_ENGINE_IO_T *io)
{
    for (size_t j = 0; j < io->nInputSize; ++j)
    {
        AX_ENGINE_IO_BUFFER_T *pBuf = io->pInputs + j;
        AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
    }
    for (size_t j = 0; j < io->nOutputSize; ++j)
    {
        AX_ENGINE_IO_BUFFER_T *pBuf = io->pOutputs + j;
        AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
    }
    delete[] io->pInputs;
    delete[] io->pOutputs;
}

static inline int prepare_io(AX_ENGINE_IO_INFO_T *info, AX_ENGINE_IO_T *io_data, INPUT_OUTPUT_ALLOC_STRATEGY strategy)
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
            ret = AX_SYS_MemAllocCached((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
        }
        else
        {
            ret = AX_SYS_MemAlloc((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
        }

        if (ret != 0)
        {
            free_io_index(io_data->pInputs, i);
            fprintf(stderr, "Allocate input{%d} { phy: %p, vir: %p, size: %lu Bytes }. fail \n", i, (void *)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
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
            ret = AX_SYS_MemAllocCached((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
        }
        else
        {
            ret = AX_SYS_MemAlloc((AX_U64 *)(&buffer->phyAddr), &buffer->pVirAddr, meta.nSize, AX_CMM_ALIGN_SIZE, (const AX_S8 *)(AX_CMM_SESSION_NAME));
        }
        if (ret != 0)
        {
            fprintf(stderr, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes }. fail \n", i, (void *)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
            free_io_index(io_data->pInputs, io_data->nInputSize);
            free_io_index(io_data->pOutputs, i);
            return ret;
        }
        // fprintf(stderr, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes }.\n", i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize);
    }

    return 0;
}

static inline AX_S32 cache_io_flush(const AX_ENGINE_IO_BUFFER_T *io_buf) {
    if (io_buf->phyAddr != 0) {
        AX_SYS_MflushCache(io_buf->phyAddr, io_buf->pVirAddr, io_buf->nSize);
    }

    return 0;
}

static void post_process(AX_ENGINE_IO_INFO_T *io_info, AX_ENGINE_IO_T *io_data, std::vector<detection::Object> &objects, const std::vector<std::string> &want_classes)
{
    std::vector<detection::Object> proposals;
    float prob_threshold_u_sigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < io_info->nOutputSize; ++i)
    {
        auto &output = io_data->pOutputs[i];
        cache_io_flush(&output);
        auto ptr = (float *)output.pVirAddr;
        int32_t stride = (1 << i) * 8;
        detection::generate_proposals_255(stride, ptr, PROB_THRESHOLD, proposals, INFER_WIDTH, INFER_HEIGHT, ANCHORS, prob_threshold_u_sigmoid);
    }

    if (!want_classes.empty())
    {
        for (auto iter = proposals.begin(); iter != proposals.end();)
        {
            bool found = false;
            for (const auto &cls : want_classes)
            {
                if (CLASS_NAMES[iter->label] == cls)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                iter = proposals.erase(iter);
            else
                iter++;
        }
    }
    
    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, INFER_HEIGHT, INFER_WIDTH, INFER_HEIGHT, INFER_WIDTH);
}

struct ax_runner_ax650_handle_t
{
    AX_ENGINE_HANDLE handle;
    AX_ENGINE_IO_INFO_T *io_info;
    AX_ENGINE_IO_T io_data;

    int algo_width, algo_height;
    int algo_colorformat;
};

static struct ax_runner_ax650_handle_t *m_handle = nullptr;

AX_S32 SAMPLE_ENGINE_Load(AX_CHAR *model_file)
{
    if (m_handle)
    {
        return 0;
    }
    m_handle = new ax_runner_ax650_handle_t;

    // 1. init engine(vin has init)
    // AX_ENGINE_NPU_ATTR_T npu_attr;
    // memset(&npu_attr, 0, sizeof(npu_attr));
    // npu_attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_DISABLE;
    // auto ret = AX_ENGINE_Init(&npu_attr);
    // if (0 != ret)
    // {
    //     return ret;
    // }

    // 2. load model
    std::vector<char> model_buffer;
    if (!read_file(model_file, model_buffer))
    {
        fprintf(stderr, "Read Run-Joint model(%s) file failed.\n", model_file);
        return -1;
    }

    // 3. create handle

    auto ret = AX_ENGINE_CreateHandle(&m_handle->handle, model_buffer.data(), model_buffer.size());
    if (0 != ret)
    {
        return ret;
    }
    // fprintf(stdout, "Engine creating handle is done.\n");

    // 4. create context
    ret = AX_ENGINE_CreateContext(m_handle->handle);
    if (0 != ret)
    {
        return ret;
    }
    // fprintf(stdout, "Engine creating context is done.\n");

    // 5. set io

    ret = AX_ENGINE_GetIOInfo(m_handle->handle, &m_handle->io_info);
    if (0 != ret)
    {
        return ret;
    }
    // fprintf(stdout, "Engine get io info is done. \n");

    // 6. alloc io

    ret = prepare_io(m_handle->io_info, &m_handle->io_data, std::make_pair(AX_ENGINE_ABST_DEFAULT, AX_ENGINE_ABST_CACHED));
    if (0 != ret)
    {
        return ret;
    }
    // fprintf(stdout, "Engine alloc io is done. \n");

    m_handle->algo_width = m_handle->io_info->pInputs[0].pShape[2];

    switch (m_handle->io_info->pInputs[0].pExtraMeta->eColorSpace)
    {
    case AX_ENGINE_CS_NV12:
        m_handle->algo_colorformat = (int)AX_FORMAT_YUV420_SEMIPLANAR;
        m_handle->algo_height = m_handle->io_info->pInputs[0].pShape[1] / 1.5;
        // printf("NV12 MODEL\n");
        break;
    case AX_ENGINE_CS_RGB:
        m_handle->algo_colorformat = (int)AX_FORMAT_RGB888;
        m_handle->algo_height = m_handle->io_info->pInputs[0].pShape[1];
        // printf("RGB MODEL\n");
        break;
    case AX_ENGINE_CS_BGR:
        m_handle->algo_colorformat = (int)AX_FORMAT_BGR888;
        m_handle->algo_height = m_handle->io_info->pInputs[0].pShape[1];
        // printf("BGR MODEL\n");
        break;
    default:
        printf("now just only support NV12/RGB/BGR input format,you can modify by yourself");
        return -1;
    }

    return ret;
}
AX_S32 SAMPLE_ENGINE_Release()
{
    if (m_handle && m_handle->handle)
    {
        free_io(&m_handle->io_data);
        AX_ENGINE_DestroyHandle(m_handle->handle);
    }
    delete m_handle;
    m_handle = nullptr;
    return 0;
}

AX_S32 SAMPLE_ENGINE_Inference(AX_VIDEO_FRAME_T *pFrame, SAMPLE_ENGINE_Results *pResults)
{
    unsigned char *dst = (unsigned char *)m_handle->io_data.pInputs[0].pVirAddr;
    unsigned char *src = (unsigned char *)pFrame->u64VirAddr[0];

    AX_U32 stride = (0 == pFrame->u32PicStride[0]) ? pFrame->u32Width : pFrame->u32PicStride[0];

    if (stride == pFrame->u32Width) {
        memcpy(dst, src, pFrame->u32Width * pFrame->u32Height * 3);
    }
    else {
        for (size_t i = 0; i < pFrame->u32Height; i++)
        {
            memcpy(dst + i * pFrame->u32Width * 3, src + i * stride * 3, pFrame->u32Width * 3);
        }
    }

    AX_S32 ret = AX_ENGINE_RunSync(m_handle->handle, &m_handle->io_data);
    if (ret)
    {
        fprintf(stderr, "AX_ENGINE_RunSync 0x%x\n", ret);
        return -1;
    }
    std::vector<detection::Object> objects;
    post_process(m_handle->io_info, &m_handle->io_data, objects, WANT_CLASSES);

    pResults->obj_count = MIN(objects.size(), SAMPLE_ENGINE_OBJ_MAX_COUNT);

    for (AX_S32 i = 0; i < pResults->obj_count; i++)
    {
        pResults->objs[i].x = objects[i].rect.x;
        pResults->objs[i].y = objects[i].rect.y;
        pResults->objs[i].width = objects[i].rect.width;
        pResults->objs[i].height = objects[i].rect.height;
        pResults->objs[i].class_label = objects[i].label;
        pResults->objs[i].prob = objects[i].prob;
        pResults->objs[i].color = COCO_COLORS_ARGB[objects[i].label].iargb;
        strcpy(pResults->objs->class_name, CLASS_NAMES[objects[i].label]);
    }

    return 0;
}
