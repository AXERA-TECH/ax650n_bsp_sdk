/*
 * UVC gadget test application
 *
 * Copyright (C) 2010 Ideas on board SPRL <laurent.pinchart@ideasonboard.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 */
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/usb/ch9.h>
#include <linux/usb/video.h>
#include <linux/videodev2.h>

#include <pthread.h>
#include <unistd.h>

#include "uvc.h"
#include "video_util.h"

/* Enable debug prints. */
#undef ENABLE_BUFFER_DEBUG
#undef ENABLE_USB_REQUEST_DEBUG

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define clamp(val, min, max)                                                                                           \
    ({                                                                                                                 \
        typeof(val) __val = (val);                                                                                     \
        typeof(min) __min = (min);                                                                                     \
        typeof(max) __max = (max);                                                                                     \
        (void)(&__val == &__min);                                                                                      \
        (void)(&__val == &__max);                                                                                      \
        __val = __val < __min ? __min : __val;                                                                         \
        __val > __max ? __max : __val;                                                                                 \
    })

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(a[0])))
#define pixfmtstr(x) (x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, ((x) >> 24) & 0xff

#define HEADER_SIZE 32
#define PAYLOAD_HEADER_SIZE 2
#define MAGIC_NUMBER 123
#define ENABLE_FILTER_DUPLICATED_FRAME 1
#define DISABLE_FILTER_DUPLICATED_FRAME 0
#define UVC_MAX_FRAME_BUFFER_SIZE     (1920 * 1080 * 2 + HEADER_SIZE)

#define PU_BRIGHTNESS_MIN_VAL 0
#define PU_BRIGHTNESS_MAX_VAL 255
#define PU_BRIGHTNESS_STEP_SIZE 1
#define PU_BRIGHTNESS_DEFAULT_VAL 127

#define UVC_FORMAT_NUM 4
#define UVC_FORMAT_NUM_DUMMY 2

#define UVC_RESOLUTION_ARR_SIZE 6

volatile int uvc_exit = 0;

static const struct uvc_frame_info uvc_frames_yuyv_os08a20[UVC_RESOLUTION_ARR_SIZE] = {
    {640, 360, {333333, 0}, 0},
    {640, 480, {333333, 0}, 0},
    {960, 540, {500000, 0}, 0},
    {1280, 720, {1000000, 0}, 0},
    {1920, 1080, {2000000, 0}, 0},
    {0, 0, { 0, 0}, 0},
};

static const struct uvc_frame_info uvc_frames_mjpeg_os08a20[UVC_RESOLUTION_ARR_SIZE] = {
    {640, 360, {333333,  0}, 2000},
    {960, 540, {333333,  0}, 2000},
    {1280, 720, {333333,  0}, 8000},
    {1920, 1080, {333333,  0}, 8000},
    {3840, 2160, {333333,  0}, 30000},
    {0, 0, {0,0}, 0},
};

static const struct uvc_frame_info uvc_frames_h264_os08a20[UVC_RESOLUTION_ARR_SIZE] = {
    {640, 360, {333333,  0}, 1000},
    {960, 540, {333333,  0}, 1000},
    {1280, 720, {333333,  0}, 8000},
    {1920, 1080, {333333,  0}, 8000},
    {3840, 2160, {333333,  0}, 30000},
    {0, 0, {0, 0}, 0},
};

static const struct uvc_frame_info uvc_frames_h265_os08a20[UVC_RESOLUTION_ARR_SIZE] = {
    {640, 360, {333333,  0}, 500},
    {960, 540, {333333,  0}, 500},
    {1280, 720, {333333,  0}, 4000},
    {1920, 1080, {333333,  0}, 4000},
    {3840, 2160, {333333,  0}, 15000},
    {0, 0, {0, 0}, 0},
};

static const struct uvc_frame_info uvc_frames_yuyv_dummy[UVC_RESOLUTION_ARR_SIZE] = {
    {640, 360, {333333, 0}, 0},
    {640, 480, {333333, 0}, 0},
    {960, 540, {500000, 0}, 0},
    {1280, 720, {1000000, 0}, 0},
    {1920, 1080, {2000000, 0}, 0},
    {0, 0, { 0, 0}, 0},
};

static const struct uvc_frame_info uvc_frames_mjpeg_dummy[UVC_RESOLUTION_ARR_SIZE] = {
    {1280, 720, {333333,  0}, 0},
    {1280, 720, {333333,  0}, 0},
    {1280, 720, {333333,  0}, 0},
    {1280, 720, {333333,  0}, 0},
    {1280, 720, {333333,  0}, 0},
    {0, 0, {0,0}, 0},
};

static struct uvc_format_info uvc_formats[UVC_FORMAT_NUM] = {0};

static const struct uvc_format_info uvc_formats_os08a20[UVC_FORMAT_NUM] = {
    {V4L2_PIX_FMT_YUYV, uvc_frames_yuyv_os08a20},
    {V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg_os08a20},
    {V4L2_PIX_FMT_H264, uvc_frames_h264_os08a20},
    {V4L2_PIX_FMT_H265, uvc_frames_h265_os08a20},
};

static const struct uvc_format_info uvc_formats_dummy[UVC_FORMAT_NUM_DUMMY] = {
    {V4L2_PIX_FMT_YUYV, uvc_frames_yuyv_dummy},
    {V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg_dummy},
};

/* ---------------------------------------------------------------------------
 * UVC device instances
 */
struct uvc_device *udev[MAX_UVC_CAMERAS] = {0};

int uvc_img_cache_init(struct uvc_device *dev, unsigned int  img_max_size)
{
    printf("%s +++\n", __func__);

    if (NULL == dev) {
        printf("%s failed: dev is null\n", __func__);
        return -1;
    }

    dev->img_cache.img_max_size = img_max_size;
    for (int i = 0; i < UVC_IMG_CACHE_COUNT; i++) {
        dev->img_cache.imgs[i].imgdata = malloc(img_max_size);
        if(dev->img_cache.imgs[i].imgdata == NULL) {
            printf("%s failed: malloc size=%u failed\n", __func__, dev->img_cache.img_max_size);
            return -1;
        }
        memset(dev->img_cache.imgs[i].imgdata, 0, img_max_size);
    }

    printf("%s ---\n", __func__);
    return 0;
}

int uvc_img_cache_deinit(struct uvc_device *dev)
{
    printf("%s +++\n", __func__);

    if (NULL == dev) {
        printf("%s failed: dev is null\n", __func__);
        return -1;
    }

    for (int i = 0; i < UVC_IMG_CACHE_COUNT; i++) {
        if(dev->img_cache.imgs[i].imgdata != 0) {
           free(dev->img_cache.imgs[i].imgdata);
           dev->img_cache.imgs[i].imgdata = NULL;
        }
    }

    printf("%s ---\n", __func__);
    return 0;
}

int uvc_img_cache_clear(struct uvc_device *dev)
{
    printf("%s +++\n", __func__);

    if (NULL == dev) {
        printf("%s failed: dev is null\n", __func__);
        return -1;
    }

    dev->img_cache.img_head = 0;
    dev->img_cache.img_tail = 0;
    dev->img_cache.has_lost = AX_FALSE;

    printf("%s ---\n", __func__);
    return 0;
}

int uvc_img_cache_put(struct uvc_device *dev, unsigned char * pBuf, unsigned int nSize, unsigned long long int iSerial, unsigned long long int pts, int isIFrame)
{
    if (NULL == dev) {
        printf("%s failed: dev is null\n", __func__);
        return -1;
    }

    if (nSize > dev->img_cache.img_max_size) {
            // must not go to here
        if (isIFrame) {
            dev->img_cache.has_lost = AX_TRUE;
        }
        printf("img cache: drop one frame for size is big\n");
        return -1;
    }

    if ((dev->img_cache.img_tail - dev->img_cache.img_head) == UVC_IMG_CACHE_COUNT) {
        // is full
        //printf("img cache: full, tail=%llu, head=%llu\n", dev->img_cache.img_tail, dev->img_cache.img_head);
        if (isIFrame) {
            // replace the tail with new I Frame
            dev->img_cache.img_tail --;
            dev->img_cache.has_lost = AX_FALSE;
        }
        else {
            dev->img_cache.has_lost = AX_TRUE;
            //printf("img cache: drop one p frame for is full\n");
            return -1;
        }

    } else {
        if (isIFrame)  {
            dev->img_cache.has_lost = AX_FALSE; // add new I frame
        }
        else {
            if (dev->img_cache.has_lost)  {
                // drop this P Frame
                //printf("img cache: drop one p frame for lost yet\n");
                return -1;
            }
        }
    }

    AX_U64 nIndex = dev->img_cache.img_tail % UVC_IMG_CACHE_COUNT;
    memcpy(dev->img_cache.imgs[nIndex].imgdata, pBuf, nSize);
    dev->img_cache.imgs[nIndex].imgusedsize = nSize;
    dev->img_cache.imgs[nIndex].is_i_frame = isIFrame;
    dev->img_cache.imgs[nIndex].pts = pts;
    dev->img_cache.imgs[nIndex].iSerial = iSerial;
    dev->img_cache.img_tail++;

    return 0;
}

struct uvc_img_info * uvc_img_cache_pop(struct uvc_device *dev)
{
    if (NULL == dev) {
        printf("%s failed: dev is null\n", __func__);
        return NULL;
    }

    if (dev->img_cache.img_head == dev->img_cache.img_tail) {
        return NULL;
    }

    AX_U64 nIndex = dev->img_cache.img_head % UVC_IMG_CACHE_COUNT;
    dev->img_cache.img_head ++;

    return &(dev->img_cache.imgs[nIndex]);
}

static int uvc_video_set_format(struct uvc_device *dev)
{
    struct v4l2_format fmt;
    int ret;

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width = dev->width;
    fmt.fmt.pix.height = dev->height;
    fmt.fmt.pix.pixelformat = dev->fcc;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if ((V4L2_PIX_FMT_MJPEG == dev->fcc) || (V4L2_PIX_FMT_H264 == dev->fcc) || (V4L2_PIX_FMT_H265 == dev->fcc)){
        if(NULL != dev->imgdata){
            fmt.fmt.pix.sizeimage = UVC_MAX_FRAME_BUFFER_SIZE;
        } else {
            fmt.fmt.pix.sizeimage = dev->img_cache.img_max_size;
        }
    }
    else if(V4L2_PIX_FMT_YUYV == dev->fcc){
        fmt.fmt.pix.sizeimage = fmt.fmt.pix.width * fmt.fmt.pix.height * 2;
    }

    ret = ioctl(dev->uvc_fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        printf("UVC: Unable to set format %s (%d).\n", strerror(errno), errno);
        return ret;
    }

    printf("UVC: Setting format to: %c%c%c%c %ux%u\n", pixfmtstr(dev->fcc), dev->width, dev->height);

    return 0;
}

static int uvc_video_stream(struct uvc_device *dev, int enable)
{
    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    int ret;

    if (!enable) {
        ret = ioctl(dev->uvc_fd, VIDIOC_STREAMOFF, &type);
        if (ret < 0) {
            printf("UVC: VbIDIOC_STREAMOFF failed: %s (%d).\n", strerror(errno), errno);
            return ret;
        }

        printf("UVC: Stopping video stream.\n");

        return 0;
    }

    ret = ioctl(dev->uvc_fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        printf("UVC: Unable to start streaming %s (%d).\n", strerror(errno), errno);
        return ret;
    }

    printf("UVC: Starting video stream.\n");

    dev->uvc_shutdown_requested = 0;

    return 0;
}

static int uvc_uninit_device(struct uvc_device *dev)
{
    unsigned int i;
    int ret;

    switch (dev->io) {
    case IO_METHOD_MMAP:
        for (i = 0; i < dev->nbufs; ++i) {
            ret = munmap(dev->mem[i].start, dev->mem[i].length);
            if (ret < 0) {
                printf("UVC: munmap failed\n");
                return ret;
            }
        }

        free(dev->mem);
        break;

    case IO_METHOD_USERPTR:
    default:
        if (dev->run_standalone) {
            for (i = 0; i < dev->nbufs; ++i)
                free(dev->dummy_buf[i].start);

            free(dev->dummy_buf);
        }
        break;
    }

    return 0;
}

static int uvc_open(struct uvc_device **uvc, char *devname)
{
    printf("%s +++\n", __func__);
    struct uvc_device *dev;
    struct v4l2_capability cap;
    int fd;
    int ret = -1;

    fd = open(devname, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        printf("UVC: device open failed: %s (%d).\n", strerror(errno), errno);
        return ret;
    }

    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        printf("UVC: unable to query uvc device: %s (%d)\n", strerror(errno), errno);
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        printf("UVC: %s is no video output device\n", devname);
        goto err;
    }

    dev = calloc(1, sizeof *dev);
    if (dev == NULL) {
        ret = -ENOMEM;
        goto err;
    }
    memset(dev, 0, sizeof *dev);

    printf("uvc device is %s on bus %s\n", cap.card, cap.bus_info);
    printf("uvc open succeeded, file descriptor = %d\n", fd);

    dev->uvc_fd = fd;
    *uvc = dev;

    printf("%s ---\n", __func__);
    return 0;
err:
    close(fd);
    printf("%s ---\n", __func__);
    return ret;
}

static void uvc_close(struct uvc_device *dev)
{
    printf("%s +++\n", __func__);
    if(dev) {
        close(dev->uvc_fd);
        uvc_img_cache_deinit(dev);
        free(dev);
    }
    printf("%s ---\n", __func__);
}

#ifdef TEST_LATENCY
AX_U64 g_u64FillBuferLatency = 0;
AX_U64 g_u64FillBuferCount = 0;
AX_U64 g_u64IoctlLatency = 0;
AX_U64 g_u64IoctlCount = 0;
#endif

/* ---------------------------------------------------------------------------
 * UVC streaming related
 */
static void uvc_video_fill_buffer(struct uvc_device *dev, struct v4l2_buffer *buf)
{
    unsigned char *start = (unsigned char*)(dev->mem[buf->index].start);
    struct uvc_img_info * img = NULL;

    if(NULL == dev->imgdata){
        img = uvc_img_cache_pop(dev);
    }

#ifdef DEBUG_TIMESTAMP_ENABLE
    printf("%llu us, %ld s, %08ld us.\n", dev->pts, buf->timestamp.tv_sec, buf->timestamp.tv_usec);
#endif

 /* Fill the buffer with video data. */
    if(NULL == dev->imgdata){
        switch (dev->fcc) {
        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:{
            if(img == NULL){
                buf->bytesused = PAYLOAD_HEADER_SIZE;
                start[0] = MAGIC_NUMBER;
                start[1] = ENABLE_FILTER_DUPLICATED_FRAME;
            }else{
                buf->timestamp.tv_sec = img->pts / (1000 * 1000);
                buf->timestamp.tv_usec = img->pts % (1000 * 1000);
                buf->bytesused = img->imgusedsize;
                start[0] = MAGIC_NUMBER;
                start[1] = DISABLE_FILTER_DUPLICATED_FRAME;
                memcpy(start + HEADER_SIZE, img->imgdata, img->imgusedsize);

#ifdef TEST_LATENCY
                AX_U64 u64CurPts = 0;
                AX_SYS_GetCurPTS(&u64CurPts);
                if (g_u64FillBuferCount < 1000) {
                    g_u64FillBuferLatency += u64CurPts - img->pts;
                    g_u64FillBuferCount++;
                    if (g_u64FillBuferCount == 1000) {
                        printf("===============fill buf done: avg latency: %llu\n", g_u64FillBuferLatency/1000);
                        g_u64FillBuferLatency = 0;
                        g_u64FillBuferCount = 0;
                    }
                }
#endif
            }
            break;
        }
        default:
            printf("format is not supported!\n");
        }
    } else {
        switch (dev->fcc) {
        case V4L2_PIX_FMT_YUYV:{
            start[0] = MAGIC_NUMBER;
            start[1] = DISABLE_FILTER_DUPLICATED_FRAME;
            uvc_draw_yuv422_color_stripe(dev->width, dev->height, start + HEADER_SIZE);
            buf->bytesused = dev->width * dev->height * 2;
            break;
        }
        case V4L2_PIX_FMT_MJPEG:{
            buf->bytesused = dev->imgsize;
            start[0] = MAGIC_NUMBER;
            start[1] = DISABLE_FILTER_DUPLICATED_FRAME;
            memcpy(start + HEADER_SIZE, dev->imgdata, dev->imgsize);
            break;
        }
        default:
            printf("format is not supported!\n");
        }
    }
}

static int uvc_video_process(struct uvc_device *dev)
{
    struct v4l2_buffer ubuf;
    int ret;
    /*
     * Return immediately if UVC video output device has not started
     * streaming yet.
     */
    if (!dev->is_streaming)
        return 0;
    /* Prepare a v4l2 buffer to be dequeued from UVC domain. */
    CLEAR(ubuf);

    ubuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    switch (dev->io) {
    case IO_METHOD_MMAP:
        ubuf.memory = V4L2_MEMORY_MMAP;
        break;

    case IO_METHOD_USERPTR:
    default:
        ubuf.memory = V4L2_MEMORY_USERPTR;
        break;
    }

    if (dev->run_standalone) {
        /* send v4l2 buffer as soon as possible. */
        do {
            /* UVC stanalone setup. */
            ret = ioctl(dev->uvc_fd, VIDIOC_DQBUF, &ubuf);
            if (ret < 0) {
                return ret;
            }
            dev->dqbuf_count++;
#ifdef ENABLE_BUFFER_DEBUG
            printf("DeQueued buffer at UVC side = %d\n", ubuf.index);
#endif
            pthread_mutex_lock(&dev->img_mutex);
            uvc_video_fill_buffer(dev, &ubuf);
            pthread_mutex_unlock(&dev->img_mutex);

#ifdef TEST_LATENCY
            AX_U64 uFrm64Pts = ubuf.timestamp.tv_sec * 1000 * 1000 + ubuf.timestamp.tv_usec;
#endif
            ret = ioctl(dev->uvc_fd, VIDIOC_QBUF, &ubuf);

#ifdef TEST_LATENCY
            AX_U64 u64CurPts = 0;
			AX_SYS_GetCurPTS(&u64CurPts);
            if (ubuf.bytesused > PAYLOAD_HEADER_SIZE) {
                if (g_u64IoctlCount < 1000) {
                    g_u64IoctlLatency += u64CurPts - uFrm64Pts;
                    g_u64IoctlCount++;
                    if (g_u64IoctlCount == 1000) {
                        printf("===============ioctl done: avg latency: %llu\n", g_u64IoctlLatency/1000);
                        g_u64IoctlLatency = 0;
                        g_u64IoctlCount = 0;
                    }
                }
            }
#endif

            if (ret < 0)
                return ret;

            dev->qbuf_count++;
#ifdef ENABLE_BUFFER_DEBUG
            printf("ReQueueing buffer at UVC side = %d\n", ubuf.index);
#endif
        } while (ret==0);
    } else {
        return -1;
    }

    return 0;
}

static int uvc_video_qbuf_mmap(struct uvc_device *dev)
{
    unsigned int i;
    int ret;

    for (i = 0; i < dev->nbufs; ++i) {
        memset(&dev->mem[i].buf, 0, sizeof(dev->mem[i].buf));

        dev->mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        dev->mem[i].buf.memory = V4L2_MEMORY_MMAP;
        dev->mem[i].buf.index = i;

        /* UVC standalone setup. */
        if (dev->run_standalone)
            uvc_video_fill_buffer(dev, &(dev->mem[i].buf));

        ret = ioctl(dev->uvc_fd, VIDIOC_QBUF, &(dev->mem[i].buf));
        if (ret < 0) {
            printf("UVC: VIDIOC_QBUF failed : %s (%d).\n", strerror(errno), errno);
            return ret;
        }

        dev->qbuf_count++;
    }

    return 0;
}

static int uvc_video_qbuf_userptr(struct uvc_device *dev)
{
    unsigned int i;
    int ret;

    /* UVC standalone setup. */
    if (dev->run_standalone) {
        for (i = 0; i < dev->nbufs; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.m.userptr = (unsigned long)dev->dummy_buf[i].start;
            buf.length = dev->dummy_buf[i].length;
            buf.index = i;
            buf.bytesused = 0;
            ret = ioctl(dev->uvc_fd, VIDIOC_QBUF, &buf);
            if (ret < 0) {
                printf("UVC: VIDIOC_QBUF failed : %s (%d).\n", strerror(errno), errno);
                return ret;
            }

            dev->qbuf_count++;
        }
    }

    return 0;
}

static int uvc_video_qbuf(struct uvc_device *dev)
{
    int ret = 0;

    switch (dev->io) {
    case IO_METHOD_MMAP:
        ret = uvc_video_qbuf_mmap(dev);
        break;

    case IO_METHOD_USERPTR:
        ret = uvc_video_qbuf_userptr(dev);
        break;

    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static int uvc_video_reqbufs_mmap(struct uvc_device *dev, int nbufs)
{
    struct v4l2_requestbuffers rb;
    unsigned int i;
    int ret;

    CLEAR(rb);

    rb.count = nbufs;
    rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    rb.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(dev->uvc_fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        if (ret == -EINVAL)
            printf("UVC: does not support memory mapping\n");
        else
            printf("UVC: Unable to allocate buffers: %s (%d).\n", strerror(errno), errno);
        goto err;
    }

    if (!rb.count)
        return 0;

    if (rb.count < 2) {
        printf("UVC: Insufficient buffer memory.\n");
        ret = -EINVAL;
        goto err;
    }

    /* Map the buffers. */
    dev->mem = calloc(rb.count, sizeof dev->mem[0]);
    if (!dev->mem) {
        printf("UVC: Out of memory\n");
        ret = -ENOMEM;
        goto err;
    }

    for (i = 0; i < rb.count; ++i) {
        memset(&dev->mem[i].buf, 0, sizeof(dev->mem[i].buf));

        dev->mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        dev->mem[i].buf.memory = V4L2_MEMORY_MMAP;
        dev->mem[i].buf.index = i;

        ret = ioctl(dev->uvc_fd, VIDIOC_QUERYBUF, &(dev->mem[i].buf));
        if (ret < 0) {
            printf(
                "UVC: VIDIOC_QUERYBUF failed for buf %d: "
                "%s (%d).\n",
                i, strerror(errno), errno);
            ret = -EINVAL;
            goto err_free;
        }
        dev->mem[i].start =
            mmap(NULL /* start anywhere */, dev->mem[i].buf.length, PROT_READ | PROT_WRITE /* required */,
                 MAP_SHARED /* recommended */, dev->uvc_fd, dev->mem[i].buf.m.offset);

        if (MAP_FAILED == dev->mem[i].start) {
            printf("UVC: Unable to map buffer %u: %s (%d).\n", i, strerror(errno), errno);
            dev->mem[i].length = 0;
            ret = -EINVAL;
            goto err_free;
        }

        dev->mem[i].length = dev->mem[i].buf.length;
        printf("UVC: Buffer %u mapped at address %p.\n", i, dev->mem[i].start);
    }

    dev->nbufs = rb.count;
    printf("UVC: %u buffers allocated.\n", rb.count);

    return 0;

err_free:
    free(dev->mem);
err:
    return ret;
}

static int uvc_video_reqbufs_userptr(struct uvc_device *dev, int nbufs)
{
    struct v4l2_requestbuffers rb;
    unsigned int i = 0, payload_size = 0;
    int ret;
    char header[HEADER_SIZE] = {0};

    CLEAR(rb);

    rb.count = nbufs;
    rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    rb.memory = V4L2_MEMORY_USERPTR;

    ret = ioctl(dev->uvc_fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        if (ret == -EINVAL)
            printf("UVC: does not support user pointer i/o\n");
        else
            printf("UVC: VIDIOC_REQBUFS error %s (%d).\n", strerror(errno), errno);
        goto err;
    }

    if (!rb.count)
        return 0;

    dev->nbufs = rb.count;
    printf("UVC: %u buffers allocated.\n", rb.count);

    if (dev->run_standalone) {
        /* Allocate buffers to hold dummy data pattern. */
        dev->dummy_buf = calloc(rb.count, sizeof dev->dummy_buf[0]);
        if (!dev->dummy_buf) {
            printf("UVC: Out of memory\n");
            ret = -ENOMEM;
            goto err;
        }

        switch (dev->fcc) {
        case V4L2_PIX_FMT_YUYV:
            /* YUYV: one pixel occupy 2 byte*/
            payload_size = UVC_MAX_FRAME_BUFFER_SIZE;
            break;
        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            if(NULL != dev->imgdata){
                payload_size = UVC_MAX_FRAME_BUFFER_SIZE;
            } else {
                payload_size = dev->img_cache.img_max_size;
            }
            break;
        }

        for (i = 0; i < rb.count; ++i) {
            dev->dummy_buf[i].length = payload_size;
            dev->dummy_buf[i].start = malloc(payload_size);
            if (!dev->dummy_buf[i].start) {
                printf("UVC: Out of memory\n");
                ret = -ENOMEM;
                goto err;
            }
            memset(dev->dummy_buf[i].start , 0, payload_size);

            if (V4L2_PIX_FMT_MJPEG == dev->fcc || V4L2_PIX_FMT_H264 == dev->fcc ||\
                V4L2_PIX_FMT_YUYV == dev->fcc || V4L2_PIX_FMT_H265 == dev->fcc)
            {
                pthread_mutex_lock(&dev->img_mutex);
                header[0] = MAGIC_NUMBER;
                memcpy((char *)(dev->dummy_buf[i].start), header, HEADER_SIZE);
                pthread_mutex_unlock(&dev->img_mutex);
            }
        }
        dev->mem = dev->dummy_buf;
    }

    return 0;

err:
    return ret;
}

static int uvc_video_reqbufs(struct uvc_device *dev, int nbufs)
{
    int ret = 0;

    switch (dev->io) {
    case IO_METHOD_MMAP:
        ret = uvc_video_reqbufs_mmap(dev, nbufs);
        break;

    case IO_METHOD_USERPTR:
        ret = uvc_video_reqbufs_userptr(dev, nbufs);
        break;

    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

/*
 * This function is called in response to either:
 * 	- A SET_ALT(interface 1, alt setting 1) command from USB host,
 * 	  if the UVC gadget supports an ISOCHRONOUS video streaming endpoint
 * 	  or,
 *
 *	- A UVC_VS_COMMIT_CONTROL command from USB host, if the UVC gadget
 *	  supports a BULK type video streaming endpoint.
 */
static int uvc_handle_streamon_event(struct uvc_device *dev)
{
    int ret;

    ret = uvc_video_reqbufs(dev, dev->nbufs);
    if (ret < 0)
        goto err;

    if (!dev->run_standalone) {
        return -1;
    }

    /* Queue buffers to UVC domain and start streaming. */
    ret = uvc_video_qbuf(dev);
    if (ret < 0)
        goto err;

    if (dev->run_standalone) {
        uvc_video_stream(dev, 1);
        dev->first_buffer_queued = 1;
        dev->is_streaming = 1;
    }

    return 0;

err:
    return ret;
}

/* ---------------------------------------------------------------------------
 * UVC Request processing
 */
static void
uvc_fill_streaming_control(struct uvc_device *dev, struct uvc_streaming_control *ctrl, int iframe, int iformat)
{
    const struct uvc_format_info *format;
    const struct uvc_frame_info *frame;
    unsigned int nframes;

    if(NULL != dev->imgdata){
        if (iformat < 0)
            iformat = ARRAY_SIZE(uvc_formats_dummy) + iformat;
        if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats_dummy))
            return;
        format = &uvc_formats_dummy[iformat];
    } else {
        if (iformat < 0)
            iformat = ARRAY_SIZE(uvc_formats) + iformat;
        if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats))
            return;
        format = &uvc_formats[iformat];
    }

    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;

    if (iframe < 0)
        iframe = nframes + iframe;
    if (iframe < 0 || iframe >= (int)nframes)
        return;
    frame = &format->frames[iframe];

    memset(ctrl, 0, sizeof *ctrl);

    ctrl->bmHint = 1;
    ctrl->bFormatIndex = iformat + 1;
    ctrl->bFrameIndex = iframe + 1;
    ctrl->dwFrameInterval = frame->intervals[0];

    switch (format->fcc) {
    case V4L2_PIX_FMT_YUYV:
        ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_MJPEG:
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        if(NULL != dev->imgdata){
            ctrl->dwMaxVideoFrameSize = UVC_MAX_FRAME_BUFFER_SIZE;
        } else {
            ctrl->dwMaxVideoFrameSize = dev->img_cache.img_max_size;
        }
        break;
    }

    /* TODO: the UVC maxpayload transfer size should be filled
     * by the driver.
     */
    if (!dev->bulk)
        ctrl->dwMaxPayloadTransferSize = (dev->maxpkt) * (dev->mult + 1) * (dev->burst + 1);
    else
        ctrl->dwMaxPayloadTransferSize = ctrl->dwMaxVideoFrameSize;

    ctrl->bmFramingInfo = 3;
    ctrl->bPreferedVersion = 1;
    ctrl->bMaxVersion = 1;
}

static void
uvc_events_process_standard(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
    printf("standard request\n");
    (void)dev;
    (void)ctrl;
    (void)resp;
}

static void uvc_events_process_control(
    struct uvc_device *dev, uint8_t req, uint8_t cs, uint8_t entity_id, uint8_t len, struct uvc_request_data *resp)
{
    switch (entity_id) {
    case 0:
        switch (cs) {
        case UVC_VC_REQUEST_ERROR_CODE_CONTROL:
            /* Send the request error code last prepared. */
            resp->data[0] = dev->request_error_code.data[0];
            resp->length = dev->request_error_code.length;
            break;

        default:
            /*
             * If we were not supposed to handle this
             * 'cs', prepare an error code response.
             */
            dev->request_error_code.data[0] = 0x06;
            dev->request_error_code.length = 1;
            break;
        }
        break;

    /* Camera terminal unit 'UVC_VC_INPUT_TERMINAL'. */
    case 1:
        switch (cs) {
        /*
         * We support only 'UVC_CT_AE_MODE_CONTROL' for CAMERA
         * terminal, as our bmControls[0] = 2 for CT. Also we
         * support only auto exposure.
         */
        case UVC_CT_AE_MODE_CONTROL:
            switch (req) {
            case UVC_SET_CUR:
                /* Incase of auto exposure, attempts to
                 * programmatically set the auto-adjusted
                 * controls are ignored.
                 */
                resp->data[0] = 0x01;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error.
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;

            case UVC_GET_INFO:
                /*
                 * TODO: We support Set and Get requests, but
                 * don't support async updates on an video
                 * status (interrupt) endpoint as of
                 * now.
                 */
                resp->data[0] = 0x03;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error.
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;

            case UVC_GET_CUR:
            case UVC_GET_DEF:
            case UVC_GET_RES:
                /* Auto Mode â€“ auto Exposure Time, auto Iris. */
                resp->data[0] = 0x02;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error.
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            default:
                /*
                 * We don't support this control, so STALL the
                 * control ep.
                 */
                resp->length = -EL2HLT;
                /*
                 * For every unsupported control request
                 * set the request error code to appropriate
                 * value.
                 */
                dev->request_error_code.data[0] = 0x07;
                dev->request_error_code.length = 1;
                break;
            }
            break;

        default:
            /*
             * We don't support this control, so STALL the control
             * ep.
             */
            resp->length = -EL2HLT;
            /*
             * If we were not supposed to handle this
             * 'cs', prepare a Request Error Code response.
             */
            dev->request_error_code.data[0] = 0x06;
            dev->request_error_code.length = 1;
            break;
        }
        break;

    /* processing unit 'UVC_VC_PROCESSING_UNIT' */
    case 2:
        switch (cs) {
        /*
         * We support only 'UVC_PU_BRIGHTNESS_CONTROL' for Processing
         * Unit, as our bmControls[0] = 1 for PU.
         */
        case UVC_PU_BRIGHTNESS_CONTROL:
            switch (req) {
            case UVC_SET_CUR:
                resp->data[0] = 0x0;
                resp->length = len;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_MIN:
                resp->data[0] = PU_BRIGHTNESS_MIN_VAL;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_MAX:
                resp->data[0] = PU_BRIGHTNESS_MAX_VAL;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_CUR:
                resp->length = 2;
                memcpy(&resp->data[0], &dev->brightness_val, resp->length);
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_INFO:
                /*
                 * We support Set and Get requests and don't
                 * support async updates on an interrupt endpt
                 */
                resp->data[0] = 0x03;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_DEF:
                resp->data[0] = PU_BRIGHTNESS_DEFAULT_VAL;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_RES:
                resp->data[0] = PU_BRIGHTNESS_STEP_SIZE;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            default:
                /*
                 * We don't support this control, so STALL the
                 * default control ep.
                 */
                resp->length = -EL2HLT;
                /*
                 * For every unsupported control request
                 * set the request error code to appropriate
                 * code.
                 */
                dev->request_error_code.data[0] = 0x07;
                dev->request_error_code.length = 1;
                break;
            }
            break;

        default:
            /*
             * We don't support this control, so STALL the control
             * ep.
             */
            resp->length = -EL2HLT;
            /*
             * If we were not supposed to handle this
             * 'cs', prepare a Request Error Code response.
             */
            dev->request_error_code.data[0] = 0x06;
            dev->request_error_code.length = 1;
            break;
        }

        break;

    default:
        /*
         * If we were not supposed to handle this
         * 'cs', prepare a Request Error Code response.
         */
        dev->request_error_code.data[0] = 0x06;
        dev->request_error_code.length = 1;
        break;
    }

    printf("control request (req %02x cs %02x)\n", req, cs);
}

static void uvc_events_process_streaming(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp)
{
    struct uvc_streaming_control *ctrl;

    printf("streaming request (req %02x cs %02x)\n", req, cs);

    if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
        return;

    ctrl = (struct uvc_streaming_control *)&resp->data;
    resp->length = sizeof *ctrl;

    switch (req) {
    case UVC_SET_CUR:
        dev->control = cs;
        resp->length = 34;
        break;

    case UVC_GET_CUR:
        if (cs == UVC_VS_PROBE_CONTROL)
            memcpy(ctrl, &dev->probe, sizeof *ctrl);
        else
            memcpy(ctrl, &dev->commit, sizeof *ctrl);
        break;

    case UVC_GET_MIN:
    case UVC_GET_MAX:
    case UVC_GET_DEF:
        uvc_fill_streaming_control(dev, ctrl, req == UVC_GET_MAX ? -1 : 0, req == UVC_GET_MAX ? -1 : 0);
        break;

    case UVC_GET_RES:
        memset(ctrl,0,sizeof(struct uvc_streaming_control));
        break;

    case UVC_GET_LEN:
        resp->data[0] = 0x00;
        resp->data[1] = 0x22;
        resp->length = 2;
        break;

    case UVC_GET_INFO:
        resp->data[0] = 0x03;
        resp->length = 1;
        break;
    }
}

static void
uvc_events_process_class(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
    if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
        return;

    switch (ctrl->wIndex & 0xff) {
    case UVC_INTF_CONTROL:
    case UVC_INTF_CONTROL2:
    case UVC_INTF_CONTROL3:
	case UVC_INTF_CONTROL4:
        uvc_events_process_control(dev, ctrl->bRequest, ctrl->wValue >> 8, ctrl->wIndex >> 8, ctrl->wLength, resp);
        break;

    case UVC_INTF_STREAMING:
    case UVC_INTF_STREAMING2:
    case UVC_INTF_STREAMING3:
	case UVC_INTF_STREAMING4:
        uvc_events_process_streaming(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
        break;

    default:
        break;
    }
}
static void
uvc_events_process_setup(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
    dev->control = 0;

#ifdef ENABLE_USB_REQUEST_DEBUG
    printf(
        "\nbRequestType %02x bRequest %02x wValue %04x wIndex %04x "
        "wLength %04x\n",
        ctrl->bRequestType, ctrl->bRequest, ctrl->wValue, ctrl->wIndex, ctrl->wLength);
#endif
    switch (ctrl->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:
        uvc_events_process_standard(dev, ctrl, resp);
        break;

    case USB_TYPE_CLASS:
        uvc_events_process_class(dev, ctrl, resp);
        break;

    default:
        break;
    }
}

static int
uvc_events_process_control_data(struct uvc_device *dev, uint8_t cs, uint8_t entity_id, struct uvc_request_data *data)
{
    switch (entity_id) {
    /* Processing unit 'UVC_VC_PROCESSING_UNIT'. */
    case 2:
        switch (cs) {
        /*
         * We support only 'UVC_PU_BRIGHTNESS_CONTROL' for Processing
         * Unit, as our bmControls[0] = 1 for PU.
         */
        case UVC_PU_BRIGHTNESS_CONTROL:
            memcpy(&dev->brightness_val, data->data, data->length);
            break;

        default:
            break;
        }

        break;

    default:
        break;
    }

    printf("Control Request data phase (cs %02x entity %02x)\n", cs, entity_id);

    return 0;
}

static void uvc_stop_get_stream(struct uvc_device *dev){
    unsigned int fcc = 0;

    if(dev->bulk) {
        fcc = dev->last_fcc;
    } else {
        fcc = uvc_formats[dev->stream_info.u64FmtIndex-1].fcc;
    }

    if(V4L2_PIX_FMT_MJPEG == fcc){
        venc_chn_deinit(dev, V4L2_PIX_FMT_MJPEG);
        if(dev->is_link){
            unlink_vin_venc_mod(dev);
        }
    }else if(V4L2_PIX_FMT_H264 == fcc || V4L2_PIX_FMT_H265 == fcc){
        venc_chn_deinit(dev, V4L2_PIX_FMT_H264);
        if(dev->is_link){
            unlink_vin_venc_mod(dev);
        }
    }else if(V4L2_PIX_FMT_YUYV == fcc){
        uncompressed_chn_deinit(dev);
    }
    // pthread_cancel(thread_id);
    pthread_join(dev->get_stream_pid, NULL);
}

static int uvc_start_get_stream(struct uvc_device *dev){
    int frames_index = 0;
    int u8Pipe = dev->pipe;
    int ret = -1;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int max_width = 0;
    unsigned int max_height = 0;
    unsigned int bitrate = 0;
    AX_VIN_CHN_ATTR_T vin_chn_attr = {0};

    /*default physical chn 1 of vin module*/
    int chn_id = 1;

    switch(dev->sns_type){
        case SAMPLE_SNS_DUMMY:
        case OMNIVISION_OS08A20:
            max_width = UVC_SNS_OS08A20_MAX_WIDTH;
            max_height = UVC_SNS_OS08A20_MAX_HEIGHT;
            break;
        default:
            printf("sensor type is not supported");
            return -1;
    }

    vin_chn_attr.ePixelFmt = AX_FORMAT_YUV420_SEMIPLANAR;
    vin_chn_attr.nDepth = 2;
    vin_chn_attr.tFrameRateCtrl.fSrcFrameRate = 30;
    vin_chn_attr.tFrameRateCtrl.fDstFrameRate = dev->stream_info.u64FrameRate;
    dev->stUvcCfgYUYV.u8DstFrameRate = dev->stream_info.u64FrameRate;

    frames_index = UVC_RESOLUTION_ARR_SIZE - dev->stream_info.u64ResoIndex - 1;

    width = uvc_formats[dev->stream_info.u64FmtIndex-1].frames[frames_index].width;
    height = uvc_formats[dev->stream_info.u64FmtIndex-1].frames[frames_index].height;
    bitrate = uvc_formats[dev->stream_info.u64FmtIndex-1].frames[frames_index].bitrate;

    if(SAMPLE_SNS_DUMMY != dev->sns_type){
        if(width > max_width || width < max_width / 16 || height > max_height || height < max_height / 16){
            printf("invalid width or height\n");
            return -1;
        }

        if(width == max_width && height == max_height){
            chn_id = 0;
            vin_chn_attr.nWidth = width;
            vin_chn_attr.nHeight = height;
            vin_chn_attr.nWidthStride = vin_chn_attr.nWidth;
        }
        else if((width >= max_width / 4) && (height >= max_height / 4)){
            chn_id = 1;
            vin_chn_attr.nWidth = width;
            vin_chn_attr.nHeight = height;
            vin_chn_attr.nWidthStride = vin_chn_attr.nWidth;

            ret = uvc_set_chn_attr(u8Pipe, chn_id, &vin_chn_attr);
            if(AX_SUCCESS != ret){
                printf("uvc_set_chn_attr failed, ret = 0x%x\n", ret);
                return -1;
            }

            vin_chn_attr.nWidth = width / 4;
            vin_chn_attr.nHeight = height / 4;
            vin_chn_attr.nWidthStride = vin_chn_attr.nWidth;

            ret = uvc_set_chn_attr(u8Pipe, chn_id + 1, &vin_chn_attr);
            if(AX_SUCCESS != ret){
                printf("uvc_set_chn_attr failed, ret = 0x%x\n", ret);
                return -1;
            }
        }else{
            chn_id = 2;
            vin_chn_attr.nWidth = width;
            vin_chn_attr.nHeight = height;
            vin_chn_attr.nWidthStride = vin_chn_attr.nWidth;

            ret = uvc_set_chn_attr(u8Pipe, chn_id, &vin_chn_attr);
            if(AX_SUCCESS != ret){
                printf("uvc_set_chn_attr failed, ret = 0x%x\n", ret);
                return -1;
            }

            vin_chn_attr.nWidth = width * 4;
            vin_chn_attr.nHeight = height * 4;
            vin_chn_attr.nWidthStride = vin_chn_attr.nWidth;

            ret = uvc_set_chn_attr(u8Pipe, chn_id - 1, &vin_chn_attr);
            if(AX_SUCCESS != ret){
                printf("uvc_set_chn_attr failed, ret = 0x%x\n", ret);
                return -1;
            }
        }
    }

    dev->vin_chn = chn_id;

    switch(uvc_formats[dev->stream_info.u64FmtIndex-1].fcc){
        case V4L2_PIX_FMT_YUYV:{
            ret = uncompressed_chn_init(dev, width, height, AX_FORMAT_YUV420_SEMIPLANAR,\
                                         AX_FORMAT_YUV422_INTERLEAVED_YUYV, width, height, chn_id);
            if(AX_SUCCESS != ret){
                printf("uncompressed_chn_init failed, ret = 0x%x\n", ret);
                return -1;
            }

            printf("video streaming info, format:YUY2, resolution:%dx%d pixels, fps:%d\n",\
                    width, height, dev->stream_info.u64FrameRate);
            break;
        }
        case V4L2_PIX_FMT_MJPEG:{
            dev->venc_stream_param.vencMod.enModId = AX_ID_VENC;
            dev->venc_stream_param.enPayloadType = PT_MJPEG;
            dev->venc_stream_param.vinMod.s32ChnId = dev->vin_chn;

            if(dev->is_link){
                ret = link_vin_venc_mod(dev);
                if(AX_SUCCESS != ret){
                    printf("link_vin_venc_mod failed!\n");
                    return -1;
                }
            }

            ret = jenc_chn_attr_init(dev, width, height, AX_FORMAT_YUV420_SEMIPLANAR,\
                    width, height, chn_id, bitrate);
            if(AX_SUCCESS != ret){
                printf("jenc_chn_attr_init failed!\n");
                return -1;
            }

            printf("video streaming info, format:MJPG, resolution:%dx%d pixels, fps:%d, bitrate:%u\n",\
                    width, height, dev->stream_info.u64FrameRate, bitrate);

            break;
        }
        case V4L2_PIX_FMT_H264:{
            dev->venc_stream_param.vencMod.enModId = AX_ID_VENC;
            dev->venc_stream_param.enPayloadType = PT_H264;
            dev->venc_stream_param.vinMod.s32ChnId = dev->vin_chn;

            if(dev->is_link){
                ret = link_vin_venc_mod(dev);
                if(AX_SUCCESS != ret){
                    printf("link_vin_venc_mod failed!\n");
                    return -1;
                }
            }

            ret = venc_chn_attr_init(dev, width, height, AX_FORMAT_YUV420_SEMIPLANAR,\
                        width, height, chn_id, PT_H264, bitrate);
            if(AX_SUCCESS != ret){
                printf("venc_chn_attr_init failed!\n");
                return -1;
            }

            printf("video streaming info, format:H264, resolution:%dx%d pixels, fps:%d, bitrate:%u\n",\
                    width, height, dev->stream_info.u64FrameRate, bitrate);

            break;
        }
        case V4L2_PIX_FMT_H265:{
            dev->venc_stream_param.vencMod.enModId = AX_ID_VENC;
            dev->venc_stream_param.enPayloadType = PT_H265;
            dev->venc_stream_param.vinMod.s32ChnId = dev->vin_chn;

            if(dev->is_link){
                ret = link_vin_venc_mod(dev);
                if(AX_SUCCESS != ret){
                    printf("link_vin_venc_mod failed!\n");
                    return -1;
                }
            }

            ret = venc_chn_attr_init(dev, width, height, AX_FORMAT_YUV420_SEMIPLANAR,\
                        width, height, chn_id, PT_H265, bitrate);
            if(AX_SUCCESS != ret){
                printf("venc_chn_attr_init failed!\n");
                return -1;
            }

            printf("video streaming info, format:H265, resolution:%dx%d pixels, fps:%d, bitrate:%u\n",\
                    width, height, dev->stream_info.u64FrameRate, bitrate);

            break;
        }
        default:
            printf("format is not supproted!\n");
            uvc_exit = 1;
            return -1;
    }

    return 0;
}

static int uvc_events_process_data(struct uvc_device *dev, struct uvc_request_data *data)
{
    struct uvc_streaming_control *target;
    struct uvc_streaming_control *ctrl;
    const struct uvc_format_info *format;
    const struct uvc_frame_info *frame;
    const unsigned int *interval;
    unsigned int iformat, iframe;
    unsigned int nframes;
    unsigned int *val = (unsigned int *)data->data;
    int ret = -1;

    switch (dev->control) {
    case UVC_VS_PROBE_CONTROL:
        printf("setting probe control, length = %d\n", data->length);
        target = &dev->probe;
        break;

    case UVC_VS_COMMIT_CONTROL:
        printf("setting commit control, length = %d\n", data->length);
        target = &dev->commit;
        break;

    default:
        printf("setting unknown control, length = %d\n", data->length);

        /*
         * As we support only BRIGHTNESS control, this request is
         * for setting BRIGHTNESS control.
         * Check for any invalid SET_CUR(BRIGHTNESS) requests
         * from Host. Note that we support Brightness levels
         * from 0x0 to 0x10 in a step of 0x1. So, any request
         * with value greater than 0x10 is invalid.
         */
        if (*val > PU_BRIGHTNESS_MAX_VAL) {
            return -EINVAL;
        } else {
            ret = uvc_events_process_control_data(dev, UVC_PU_BRIGHTNESS_CONTROL, 2, data);
            if (ret < 0)
                goto err;

            return 0;
        }
    }

    ctrl = (struct uvc_streaming_control *)&data->data;
    if(NULL != dev->imgdata){
        iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U, (unsigned int)ARRAY_SIZE(uvc_formats_dummy));
        format = &uvc_formats_dummy[iformat - 1];
    } else {
        iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U, (unsigned int)ARRAY_SIZE(uvc_formats));
        format = &uvc_formats[iformat - 1];
    }

    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;

    iframe = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
    frame = &format->frames[UVC_RESOLUTION_ARR_SIZE - iframe - 1];
    interval = frame->intervals;

    while (interval[0] < ctrl->dwFrameInterval && interval[1])
        ++interval;

    target->bFormatIndex = iformat;
    target->bFrameIndex = iframe;
    dev->stream_info.u64FmtIndex = target->bFormatIndex;
    dev->stream_info.u64ResoIndex = target->bFrameIndex;
    dev->stream_info.u64FrameRate = 10 * 1000 * 1000 / ctrl->dwFrameInterval;

    switch (format->fcc) {
    case V4L2_PIX_FMT_YUYV:
        target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_MJPEG:
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        if(NULL == dev->imgdata){
            if(0 == dev->img_cache.img_max_size){
                printf("WARNING: MJPEG requested and no image loaded.\n");
            }
            target->dwMaxVideoFrameSize = dev->img_cache.img_max_size;
        } else {
            if(0 == dev->imgsize){
                printf("WARNING: MJPEG requested and no image loaded.\n");
            }
            target->dwMaxVideoFrameSize = UVC_MAX_FRAME_BUFFER_SIZE;
        }
        break;
    }
    target->dwFrameInterval = *interval;

    if (dev->control == UVC_VS_COMMIT_CONTROL) {
        dev->fcc = format->fcc;
        dev->width = frame->width;
        dev->height = frame->height;

        if(dev->bulk){
            if(dev->is_streaming){
                if(NULL == dev->imgdata){
                    uvc_stop_get_stream(dev);
                }
                uvc_video_stream(dev, 0);
                uvc_uninit_device(dev);
                uvc_video_reqbufs(dev, 0);
                dev->is_streaming = 0;
                dev->first_buffer_queued = 0;
            }

            ret = uvc_video_set_format(dev);
            if (ret < 0)
                goto err;

            dev->last_fcc = dev->fcc;

            if(NULL == dev->imgdata){
                uvc_img_cache_clear(dev);
            }

            uvc_handle_streamon_event(dev);
            sleep(1);
            if(NULL == dev->imgdata){
                ret = uvc_start_get_stream(dev);
                if(ret < 0)
                    goto err;
            }

        }
    }
    return 0;
err:
    return ret;
}

static void uvc_events_process(struct uvc_device *dev)
{
    struct v4l2_event v4l2_event;
    struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
    struct uvc_request_data resp;
    int ret = -1;

    ret = ioctl(dev->uvc_fd, VIDIOC_DQEVENT, &v4l2_event);
    if (ret < 0) {
        printf("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno), errno);
        return;
    }

    memset(&resp, 0, sizeof resp);
    resp.length = -EL2HLT;

    switch (v4l2_event.type) {
    case UVC_EVENT_CONNECT:
        return;

    case UVC_EVENT_DISCONNECT:
        dev->uvc_shutdown_requested = 1;
        printf(
            "UVC: Possible USB shutdown requested from "
            "Host, seen via UVC_EVENT_DISCONNECT\n");
        return;

    case UVC_EVENT_SETUP:
        uvc_events_process_setup(dev, &uvc_event->req, &resp);
        break;

    case UVC_EVENT_DATA:
        ret = uvc_events_process_data(dev, &uvc_event->data);
        if (ret < 0)
            break;
        return;

    case UVC_EVENT_STREAMON:{
        /* ... and now UVC streaming.. */
        if(!dev->bulk){
            if(NULL == dev->imgdata){
                uvc_img_cache_clear(dev);
            }
            uvc_handle_streamon_event(dev);
            sleep(1);
            if(NULL == dev->imgdata){
                ret = uvc_start_get_stream(dev);
                if(ret < 0)
                    break;
            }
            return;
        }
    }

    case UVC_EVENT_STREAMOFF:{
        if (dev->is_streaming) {
            if(NULL == dev->imgdata){
                uvc_stop_get_stream(dev);
            }
            uvc_video_stream(dev, 0);
            uvc_uninit_device(dev);
            uvc_video_reqbufs(dev, 0);
            dev->is_streaming = 0;
            dev->first_buffer_queued = 0;
        }
        return;
    }
    }

    ret = ioctl(dev->uvc_fd, UVCIOC_SEND_RESPONSE, &resp);
    if (ret < 0) {
        printf("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno), errno);
        return;
    }
}

static void uvc_events_init(struct uvc_device *dev)
{
    printf("%s +++\n", __func__);
    struct v4l2_event_subscription sub;
    unsigned int payload_size = 0;

    switch (dev->fcc) {
    case V4L2_PIX_FMT_YUYV:
        payload_size = dev->width * dev->height * 2;
        break;
    case V4L2_PIX_FMT_MJPEG:
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        if(NULL != dev->imgdata){
            payload_size = UVC_MAX_FRAME_BUFFER_SIZE;
        } else {
            payload_size = dev->img_cache.img_max_size;
        }
        break;
    }

    uvc_fill_streaming_control(dev, &dev->probe, 0, 0);
    uvc_fill_streaming_control(dev, &dev->commit, 0, 0);

    if (dev->bulk) {
        /* FIXME Crude hack, must be negotiated with the driver. */
        dev->probe.dwMaxPayloadTransferSize = payload_size - HEADER_SIZE;
        dev->commit.dwMaxPayloadTransferSize = payload_size - HEADER_SIZE;
    }

    memset(&sub, 0, sizeof sub);
    sub.type = UVC_EVENT_SETUP;
    ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_DATA;
    ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMON;
    ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMOFF;
    ioctl(dev->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    printf("%s ---\n", __func__);
}

static void* uvc_handle_event(void* args){
    int ret = -1;
    char thread_name[64] = {0};
    fd_set fdsu;

    struct timeval tv;
    struct uvc_device* dev = (struct uvc_device*)args;

    sprintf(thread_name ,"uvc_handle_event_dev%d", dev->dev_id);
    prctl(PR_SET_NAME, thread_name);

    tv.tv_sec  = 3;
    tv.tv_usec = 0;

    while (!uvc_exit) {
        FD_ZERO(&fdsu);

        /* We want both setup and data events on UVC interface.. */
        FD_SET(dev->uvc_fd, &fdsu);

        fd_set efds = fdsu;
        fd_set dfds = fdsu;

        ret = select(dev->uvc_fd + 1, NULL, &dfds, &efds, &tv);
        if (-1 == ret) {
#ifdef ENABLE_BUFFER_DEBUG
            printf("select error %d, %s\n", errno, strerror(errno));
#endif
            if (EINTR == errno)
                continue;

            break;
        }

        if (0 == ret) {
            continue;
        }

        if (FD_ISSET(dev->uvc_fd, &efds))
            uvc_events_process(dev);
        if (FD_ISSET(dev->uvc_fd, &dfds))
            uvc_video_process(dev);

        usleep(1);
    }

    if (dev->is_streaming) {
        /* ... and stop UVC streaming.. */
        printf("uvc_video_stream: stop ...\n");
        uvc_video_stream(dev, 0);

        printf("uvc_uninit_device ...\n");
        uvc_uninit_device(dev);

        printf("uvc_video_reqbufs: clean ...\n");
        uvc_video_reqbufs(dev, 0);
    }

    if(NULL == dev->imgdata){
        if(dev->is_streaming){
            switch(uvc_formats[dev->stream_info.u64FmtIndex-1].fcc){
                case V4L2_PIX_FMT_YUYV:
                    uncompressed_chn_deinit(dev);
                    break;
                case V4L2_PIX_FMT_MJPEG:
                    venc_chn_deinit(dev, V4L2_PIX_FMT_MJPEG);
                    if(dev->is_link){
                        unlink_vin_venc_mod(dev);
                    }
                    break;
                case V4L2_PIX_FMT_H264:
                case V4L2_PIX_FMT_H265:
                    venc_chn_deinit(dev, V4L2_PIX_FMT_H264);
                    if(dev->is_link){
                        unlink_vin_venc_mod(dev);
                    }
                    break;
            }
            // pthread_cancel(thread_id);
            printf("wait video thread exit ...\n");
            pthread_join(dev->get_stream_pid, NULL);
            pthread_mutex_destroy(&dev->img_mutex);
            dev->is_streaming = 0;
        }
    }

    printf("uvc %d close ...\n", dev->dev_id);
    uvc_close(dev);

    return (void *)(intptr_t)ret;
}

static void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [options]\n", argv0);
    fprintf(stderr, "Available options are\n");
    fprintf(stderr, " -b		Use bulk mode\n");
    fprintf(stderr, " -d		Do not use any real V4L2 capture device\n");
    fprintf(stderr, " -p		Start isp tuning\n");
    fprintf(stderr,
            " -f <format>    Select frame format\n\t"
            "0 = V4L2_PIX_FMT_YUYV\n\t"
            "1 = V4L2_PIX_FMT_MJPEG\n\t"
            "2 = V4L2_PIX_FMT_H264\n\t"
            "3 = V4L2_PIX_FMT_H265\n");
    fprintf(stderr, " -h		Print this help screen and exit\n");
    fprintf(stderr, " -m		Streaming mult for ISOC (b/w 0 and 2)\n");
    fprintf(stderr, " -n		Number of Video buffers (b/w 2 and 32)\n");
    fprintf(stderr,
            " -o <IO method> Select UVC IO method:\n\t"
            "0 = MMAP\n\t"
            "1 = USER_PTR\n");
    fprintf(stderr,
            " -r <resolution> Select frame resolution:\n\t"
            "0 = 720p, WXGA (1280x720)\n\t"
            "1 = 1080p, (1920x1080)\n");
    fprintf(stderr,
            " -s <speed>	Select USB bus speed (b/w 0 and 2)\n\t"
            "0 = Full Speed (FS)\n\t"
            "1 = High Speed (HS)\n\t"
            "2 = Super Speed (SS)\n");
    fprintf(stderr, " -t		Streaming burst (b/w 0 and 15)\n");
    fprintf(stderr, " -l		max packet size (0 - 1024)\n");
    fprintf(stderr, " -u		1st UVC Video Output device\n");
    fprintf(stderr, " -z		2nd UVC Video Output device\n");
    fprintf(stderr,
            " -w link mode or unlink mode:\n\t"
            "0 = unlink mode\n\t"
            "1 = link mode\n");
    fprintf(stderr,
            " -y <sensor>	Select sensor type\n\t"
            "0 = single os08a20 sensor\n\t"
            "1 = dual os08a20 sensors\n\t"
            "2 = single dummy sensor\n\t"
            "3 = dual dummy sensors\n");
    fprintf(stderr,
            " -a enable aiisp, default 1:\n\t"
            "0 = disable\n\t"
            "1 = enable\n");
}

static int image_load(struct uvc_device *dev, const char *img)
{
    int fd = -1;

    if ((NULL == img) || (NULL == dev))
        return -1;

    fd = open(img, O_RDONLY);
    if (fd == -1) {
        printf("Unable to open MJPEG image '%s'\n", img);
        return -1;
    }

    dev->imgsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if (dev->imgdata == NULL) {
        printf("no memory for loading  MJPEG image\n");
        dev->imgsize = 0;
        return -1;
    }

    read(fd, dev->imgdata, dev->imgsize);
    close(fd);
    return 0;
}

static AX_VOID __sigint(int iSigNo)
{
    printf("Catch signal %d\n", iSigNo);
    uvc_exit = 1;
    return;
}

/* ---------------------------------------------------------------------------
 * main
 */
int main(int argc, char *argv[])
{
    printf("UVC Sample. Build at %s %s\n", __DATE__, __TIME__);
    char *uvc_devname[MAX_UVC_CAMERAS] = {"/dev/video0", "/dev/video1"};
    char *input_pic = NULL;

    int ret, opt;
    int bulk_mode = 0;
    int dummy_data_gen_mode = 1;

    /* Frame format/resolution related params. */
    int default_format = 1;     /* V4L2_PIX_FMT_MJPEG */
    int default_resolution = 0; /* 720p */
    int nbufs = 2;              /* Ping-Pong buffers */

    /* USB speed related params */
    int mult = 0;
    int burst = 0;
    int max_packet_size = 1024;
    int sensor_type = 0;
    int camera_count = 1;
    int isp_tuning = 0;
    unsigned char link = 1;
    int aiisp_enable = 1;

    pthread_t uvc_handle_thread_id[MAX_UVC_CAMERAS] = {-1};

    enum usb_device_speed speed = USB_SPEED_HIGH; /* High-Speed: default usb2.0 */
    enum io_method uvc_io_method = IO_METHOD_USERPTR;

    UVC_SYS_CASE_E eSysCase = UVC_SYS_CASE_NONE;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    AX_SNS_HDR_MODE_E eHdrMode = AX_SNS_MODE_NONE;
    SAMPLE_SNS_TYPE_E eSnsType = SAMPLE_SNS_TYPE_NONE;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;

#ifdef SUPPORT_DSP_CSC
    AX_DSP_ID_E dsp_id = AX_DSP_ID_1;
    const char* itcm = "/opt/data/dsp/itcm.bin";
    const char* sram = "/opt/data/dsp/sram.bin";
#endif

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigint);

    while ((opt = getopt(argc, argv, "a:b::d::f:h::i:l:m:n:o:p::r:s:t:u:w:y:z:")) != -1) {
        switch (opt) {
        case 'a':
            if (atoi(optarg) != 0 && atoi(optarg) != 1) {
                usage(argv[0]);
                return 1;
            }

            aiisp_enable = atoi(optarg);
            printf("aiisp enable = %d\n", nbufs);
            break;

        case 'b':
            bulk_mode = 1;
            break;

        case 'd':
            dummy_data_gen_mode = 1;
            break;

        case 'f':
            if (atoi(optarg) < 0 || atoi(optarg) > 2) {
                usage(argv[0]);
                return 1;
            }

            default_format = atoi(optarg);
            break;

        case 'h':
            usage(argv[0]);
            return 1;

        case 'i':
            input_pic = optarg;
            break;

        case 'l':
            if (atoi(optarg) < 0 || atoi(optarg) > 1024) {
                usage(argv[0]);
                return 1;
            }

            max_packet_size = atoi(optarg);
            break;

        case 'm':
            if (atoi(optarg) < 0 || atoi(optarg) > 2) {
                usage(argv[0]);
                return 1;
            }

            mult = atoi(optarg);
            printf("Requested Mult value = %d\n", mult);
            break;

        case 'n':
            if (atoi(optarg) < 2 || atoi(optarg) > 32) {
                usage(argv[0]);
                return 1;
            }

            nbufs = atoi(optarg);
            printf("Number of buffers requested = %d\n", nbufs);
            break;

        case 'o':
            if (atoi(optarg) < 0 || atoi(optarg) > 1) {
                usage(argv[0]);
                return 1;
            }

            uvc_io_method = atoi(optarg);
            printf("UVC: IO method requested is %s\n", (uvc_io_method == IO_METHOD_MMAP) ? "MMAP" : "USER_PTR");
            break;

        case 'p':
            isp_tuning = 1;
            break;

        case 'r':
            if (atoi(optarg) < 0 || atoi(optarg) > 1) {
                usage(argv[0]);
                return 1;
            }

            default_resolution = atoi(optarg);
            break;

        case 's':
            if (atoi(optarg) < 0 || atoi(optarg) > 2) {
                usage(argv[0]);
                return 1;
            }

            speed = atoi(optarg);
            break;

        case 't':
            if (atoi(optarg) < 0 || atoi(optarg) > 15) {
                usage(argv[0]);
                return 1;
            }

            burst = atoi(optarg);
            printf("Requested Burst value = %d\n", burst);
            break;

        case 'u':
            uvc_devname[0] = optarg;
            break;

        case 'w':
            if(atoi(optarg) < 0 || atoi(optarg) > 1){
                usage(argv[0]);
                return 1;
            }
            link = atoi(optarg);
            break;

        case 'y':
            if (atoi(optarg) < 0 || atoi(optarg) > 3) {
                usage(argv[0]);
                return 1;
            }

            sensor_type = atoi(optarg);
            break;

        case 'z':
            uvc_devname[1] = optarg;
            break;

        default:
            printf("Invalid option '-%c'\n", opt);
            usage(argv[0]);
            return 1;
        }
    }

    /* config syscase, sysmode, hdrmode, snstype according to sensor_type input param */
    switch(sensor_type){
        case 0:
            eSysCase = UVC_SYS_CASE_SINGLE_OS08A20;
            eHdrMode = AX_SNS_LINEAR_MODE;
            eSnsType = OMNIVISION_OS08A20;
            memcpy(&uvc_formats, &uvc_formats_os08a20, sizeof(uvc_formats_os08a20));
            break;
        case 1:
            eSysCase = UVC_SYS_CASE_DUAL_OS08A20;
            eHdrMode = AX_SNS_LINEAR_MODE;
            eSnsType = OMNIVISION_OS08A20;
            camera_count = 2;
            memcpy(&uvc_formats, &uvc_formats_os08a20, sizeof(uvc_formats_os08a20));
            break;
        case 2:
            eSysCase = UVC_SYS_CASE_SINGLE_DUMMY;
            eHdrMode = AX_SNS_MODE_NONE;
            eSnsType = SAMPLE_SNS_DUMMY;
            break;
        case 3:
            eSysCase = UVC_SYS_CASE_DUAL_DUMMY;
            eHdrMode = AX_SNS_MODE_NONE;
            eSnsType = SAMPLE_SNS_DUMMY;
            camera_count = 2;
            break;
        default:
           printf("sensor type is not supported!\n");
           return -1;
    }

    for(int i = 0; i < camera_count; i++){
        /* Open the UVC device. */
        printf("open %s ...\n", uvc_devname[i]);
        ret = uvc_open(&udev[i], uvc_devname[i]);
        if (udev[i] == NULL || ret < 0) {
            printf("open uvc %d device failed\n", i);
            return 1;
        }

        udev[i]->sns_type = eSnsType;
        udev[i]->uvc_devname = uvc_devname[i];
        udev[i]->dev_id = i;

        udev[i]->venc_stream_param.vinMod.enModId = AX_ID_VIN;
        udev[i]->venc_stream_param.vinMod.s32GrpId = i;
        udev[i]->venc_stream_param.vinMod.s32ChnId = 1;
        udev[i]->venc_stream_param.vencMod.enModId = AX_ID_JENC;
        udev[i]->venc_stream_param.vencMod.s32GrpId = 0;
        udev[i]->venc_stream_param.vencMod.s32ChnId = i;
        udev[i]->venc_stream_param.bThreadStart = AX_FALSE;
        udev[i]->venc_stream_param.enPayloadType = PT_MJPEG;

        udev[i]->venc_stream_param.VeChn = udev[i]->venc_stream_param.vencMod.s32ChnId;

        /* Set parameters as passed by user. */
        udev[i]->width = (default_resolution == 0) ? 1280 : 1920;
        udev[i]->height = (default_resolution == 0) ? 720 : 1080;

        switch(default_format){
        case 0:
            udev[i]->fcc = V4L2_PIX_FMT_YUYV;
            break;
        case 1:
            udev[i]->fcc = V4L2_PIX_FMT_MJPEG;
            break;
        case 2:
            udev[i]->fcc = V4L2_PIX_FMT_H264;
            break;
        case 3:
            udev[i]->fcc = V4L2_PIX_FMT_H265;
            break;
        default:
            udev[i]->fcc = V4L2_PIX_FMT_MJPEG;
        }

        pthread_mutex_init(&udev[i]->img_mutex, NULL);
        udev[i]->io = uvc_io_method;
        udev[i]->bulk = bulk_mode;
        udev[i]->nbufs = nbufs;
        udev[i]->mult = mult;
        udev[i]->burst = burst;
        udev[i]->speed = speed;
        udev[i]->is_link = link;
#ifdef SUPPORT_DSP_CSC
        udev[i]->stUvcCfgYUYV.u8DspId = dsp_id;
#endif
        if((UVC_SYS_CASE_SINGLE_DUMMY == eSysCase) || (UVC_SYS_CASE_DUAL_DUMMY == eSysCase)){
            udev[i]->imgdata = (unsigned char *)malloc(UVC_MAX_FRAME_BUFFER_SIZE);
            if(NULL == udev[i]->imgdata){
                printf("failed to allocate memory!\n");
                return -1;
            }

            if(NULL == input_pic){
                printf("no input image, use /opt/data/uvc/1280x720.jpg as default!\n");
                input_pic = "/opt/data/uvc/1280x720.jpg";
            }
        } else if ((UVC_SYS_CASE_SINGLE_OS08A20 == eSysCase) || (UVC_SYS_CASE_DUAL_OS08A20 == eSysCase)){
            if(0 != uvc_img_cache_init(udev[i], UVC_MAX_FRAME_BUFFER_SIZE)) {
                printf("Unable to allocate memory for image cache!\n");
                return -1;
            }
        } else {
            printf("uvc sys case is not supported!\n");
            return -1;
        }

        if (dummy_data_gen_mode)
            /* UVC standalone setup. */
            udev[i]->run_standalone = 1;

        switch (speed) {
        case USB_SPEED_FULL:
            /* Full Speed. */
            if (bulk_mode)
                udev[i]->maxpkt = 64;
            else
                udev[i]->maxpkt = 1023;
            break;

        case USB_SPEED_HIGH:
            /* High Speed. */
            if (bulk_mode)
                udev[i]->maxpkt = 512;
            else
                udev[i]->maxpkt = max_packet_size;
            break;

        case USB_SPEED_SUPER:
        default:
            /* Super Speed. */
            if (bulk_mode)
                udev[i]->maxpkt = max_packet_size;
            else
                udev[i]->maxpkt = max_packet_size;
            break;
        }

        /* Init UVC events. */
        uvc_events_init(udev[i]);
    }

    if((UVC_SYS_CASE_SINGLE_DUMMY == eSysCase) || (UVC_SYS_CASE_DUAL_DUMMY == eSysCase)){
        for(int i = 0; i < camera_count; i++){
            if(AX_SUCCESS != image_load(udev[i], input_pic)){
                printf("load image failed!\n");
                return -1;
            }
        }
    } else if((UVC_SYS_CASE_SINGLE_OS08A20 == eSysCase) || (UVC_SYS_CASE_DUAL_OS08A20 == eSysCase)){
        /* Init video. */
        ret = video_init(udev, eSysCase, tCommonArgs, eSysMode, eHdrMode, eSnsType, isp_tuning, aiisp_enable);
        if(AX_SUCCESS != ret){
            printf("video_init failed, ret=0x%x\n", ret);
            return -1;
        }

        /*Init video encoder*/
        ret = venc_init((V4L2_PIX_FMT_MJPEG + V4L2_PIX_FMT_H264));
        if(AX_SUCCESS != ret){
            printf("venc_init failed, ret=0x%x\n", ret);
            return -1;
        }

#ifdef SUPPORT_DSP_CSC
        /* Init DSP */
        ret = uvc_init_dsp(dsp_id, itcm, sram);
        if(AX_SUCCESS != ret){
            printf("uvc_init_dsp failed, ret=0x%x\n", ret);
            return -1;
        }
#endif
    }

    /* Start thread for uvc event */
    for(int i = 0; i < camera_count; i++){
        printf("create uvc dev %d event processing thread...\n", i);
        pthread_create(&uvc_handle_thread_id[i], NULL, uvc_handle_event, (void *)udev[i]);
    }

    /* Wait for child thread to finish task */
    while(!uvc_exit){
        sleep(1);
    }

    /* Wait for child thread to exit */
    for(int i = 0; i < camera_count; i++){
        pthread_join(uvc_handle_thread_id[i], NULL);
    }

    if((UVC_SYS_CASE_SINGLE_DUMMY == eSysCase) || (UVC_SYS_CASE_DUAL_DUMMY == eSysCase)){
        for(int i = 0; i < camera_count; i++){
            if(NULL != udev[i]->imgdata){
                free(udev[i]->imgdata);
                udev[i]->imgdata = NULL;
            }
        }
    } else if((UVC_SYS_CASE_SINGLE_OS08A20 == eSysCase) || (UVC_SYS_CASE_DUAL_OS08A20 == eSysCase)){
        printf("video_deinit ...\n");
        video_deinit(isp_tuning);
        printf("venc_deinit ...\n");
        venc_deinit();
#ifdef SUPPORT_DSP_CSC
        printf("uvc_deinit_dsp ...\n");
        uvc_deinit_dsp(dsp_id);
#endif
    }
    printf("uvc sample main thread exit!\n");
    return 0;
}
