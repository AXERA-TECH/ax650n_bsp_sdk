/*
 *	uvc_gadget.h  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#ifndef _UVC_GADGET_H_
#define _UVC_GADGET_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>

#include <linux/usb/video.h>
#include <linux/videodev2.h>

#include <stddef.h>
#include <stdint.h>

#include "ax_isp_api.h"
#include "common_venc.h"
#include "common_cam.h"

#define SUPPORT_DSP_CSC
#ifdef SUPPORT_DSP_CSC
#include "ax_dsp_api.h"
#include "ax_dsp_cv_api.h"
#endif

#define UVC_EVENT_FIRST			(V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_CONNECT		(V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_DISCONNECT		(V4L2_EVENT_PRIVATE_START + 1)
#define UVC_EVENT_STREAMON		(V4L2_EVENT_PRIVATE_START + 2)
#define UVC_EVENT_STREAMOFF		(V4L2_EVENT_PRIVATE_START + 3)
#define UVC_EVENT_SETUP			(V4L2_EVENT_PRIVATE_START + 4)
#define UVC_EVENT_DATA			(V4L2_EVENT_PRIVATE_START + 5)
#define UVC_EVENT_LAST			(V4L2_EVENT_PRIVATE_START + 5)

struct uvc_request_data
{
	__s32 length;
	__u8 data[60];
};

struct uvc_event
{
	union {
		enum usb_device_speed speed;
		struct usb_ctrlrequest req;
		struct uvc_request_data data;
	};
};

#define UVCIOC_SEND_RESPONSE		_IOW('U', 1, struct uvc_request_data)

#define UVC_INTF_CONTROL		0
#define UVC_INTF_STREAMING		1
#define UVC_INTF_CONTROL2		2
#define UVC_INTF_STREAMING2		3
#define UVC_INTF_CONTROL3		4
#define UVC_INTF_STREAMING3		5
#define UVC_INTF_CONTROL4		6
#define UVC_INTF_STREAMING4		7

#define UVC_IMG_CACHE_COUNT     10

#define V4L2_PIX_FMT_H265     v4l2_fourcc('H', '2', '6', '5') /* H265 with start codes */

/* ------------------------------------------------------------------------
 * Structures
 */
/* IO methods supported */
enum io_method {
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
};

/* Buffer representing one video frame */
struct buffer {
    struct v4l2_buffer buf;
    void *start;
    size_t length;
};

/* ---------------------------------------------------------------------------
 * UVC specific stuff
 */
struct uvc_frame_info {
    unsigned int width;
    unsigned int height;
    unsigned int intervals[8];
    unsigned int bitrate;  // xxx kbps
};

struct uvc_format_info {
    unsigned int fcc;
    const struct uvc_frame_info *frames;
};

struct uvc_img_info {
    unsigned int            imgusedsize;
    void *                  imgdata;
    int                     is_i_frame;
    unsigned long long int  iSerial;
    unsigned long long int  pts;
};

struct uvc_img_cache {
    struct uvc_img_info imgs[UVC_IMG_CACHE_COUNT];
    unsigned int  img_max_size;
    int           has_lost;
    long long int img_head;
    long long int img_tail;
};

typedef struct _stUVCConfigYUYV{
    AX_U32 u32Width;
    AX_U32 u32Height;
    AX_S32 s32MemSize;
    AX_U32 u32BlkID;
    AX_U64 u64Phy;
    AX_U8* pU8VirAddr;
    AX_U32 s32Chn;
    AX_CAMERA_T* pStUVCCamera;
    pthread_t stGetYUYVPid;
    AX_U8 isExit;
    AX_U8 u8DstFrameRate;
    AX_U64 u64FrameSeqNo;
#ifdef SUPPORT_DSP_CSC
    AX_DSP_CV_CVTCOLOR_PARAM_T stDspCVParam;
    AX_U8 u8DspId;
#endif
}UVCConfigYUYV;

typedef struct _stStreamInfo{
    pthread_mutex_t imgMtx;
    AX_U64 u64ResoIndex;
    AX_U64 u64FmtIndex;
    AX_U32 u64FrameRate;
    AX_S8  s8IsUVCExit;
    AX_U64 u64Serial;
}StreamInfo, *pStreamInfo;

typedef struct axMOD_INFO_S {
    AX_MOD_ID_E enModId;
    AX_S32 s32GrpId;
    AX_S32 s32ChnId;
} AX_MOD_INFO_S;

typedef struct VENC_GETSTREAM_PARA
{
    AX_BOOL bThreadStart;
    int VeChn;
    AX_MOD_INFO_T vinMod;
    AX_MOD_INFO_T vencMod;
    AX_PAYLOAD_TYPE_E enPayloadType;
} VENC_GETSTREAM_PARA_T;

/* Represents a UVC based video output device */
struct uvc_device {
    /* uvc device specific */
    int uvc_fd;
    int dev_id;
    int pipe;
    SAMPLE_SNS_TYPE_E sns_type;
    int encoder_chn;
    int is_streaming;
    int run_standalone;
    char *uvc_devname;
    unsigned char is_link;
    unsigned char vin_chn;

    pthread_t dispatch_frame_pid;
    pthread_t get_stream_pid;
    pthread_mutex_t img_mutex;
    StreamInfo stream_info;
    UVCConfigYUYV stUvcCfgYUYV;
    VENC_GETSTREAM_PARA_T venc_stream_param;

    /* uvc control request specific */
    struct uvc_streaming_control probe;
    struct uvc_streaming_control commit;
    int control;
    struct uvc_request_data request_error_code;
    unsigned int brightness_val;

    /* uvc buffer specific */
    enum io_method io;
    struct buffer *mem;
    struct buffer *dummy_buf;
    unsigned int nbufs;
    unsigned int fcc;
    unsigned int last_fcc;
    unsigned int width;
    unsigned int height;

    unsigned int bulk;
    uint8_t color;

    /* USB speed specific */
    int mult;
    int burst;
    int maxpkt;
    enum usb_device_speed speed;

    /* uvc specific flags */
    int first_buffer_queued;
    int uvc_shutdown_requested;

    /* uvc buffer queue and dequeue counters */
    unsigned long long int qbuf_count;
    unsigned long long int dqbuf_count;

    /* image cache */
    struct uvc_img_cache img_cache;

    unsigned char* imgdata;
    int imgsize;
};

int                   uvc_img_cache_init(struct uvc_device *dev, unsigned int  img_max_size);
int                   uvc_img_cache_deinit(struct uvc_device *dev);
int                   uvc_img_cache_clear(struct uvc_device *dev);
int                   uvc_img_cache_put(struct uvc_device *dev, unsigned char * pBuf, unsigned int nSize, unsigned long long int iSerial,\
                                            unsigned long long int pts, int isIFrame);
struct uvc_img_info * uvc_img_cache_pop(struct uvc_device *dev);

#endif /* _UVC_GADGET_H_ */

