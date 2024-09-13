/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "ivps_help.h"

AX_VOID IVPS_ObjHelp(AX_VOID)
{
    printf("Usage: /%s\n", SAMPLE_NAME);
    printf("\t-v             (required) : video frame input\n");
    printf("\t-g             (optional) : overlay input\n");
    printf("\t-s             (optional) : sp alpha input\n");
    printf("\t-n             (optional) : repeat number\n");
    printf("\t-r             (optional) : region config and update\n");
    printf("\t-l             (optional) : 0: no link 1. link ivps. 2: link venc. 3: link jenc\n");
    printf("\t--pipeline     (optional) : import ini file to config all the filters in one pipeline\n");
    printf("\t--pipeline_ext (optional) : import ini file to config all the filters in another pipeline\n");
    printf("\t--change       (optional) : import ini file to change parameters for one filter dynamicly\n");
    printf("\t--region       (optional) : import ini file to config region parameters\n");
    printf("\t--dewarp       (optional) : import ini file to config dewarp parameters, including LDC, perspective, fisheye, etc.\n");
    printf("\t--cmmcopy      (optional) : cmm copy API test\n");
    printf("\t--csc          (optional) : color space covert API test\n");
    printf("\t--fliprotation (optional) : flip and rotation API test\n");
    printf("\t--alphablend   (optional) : alpha blending API test\n");
    printf("\t--cropresize   (optional) : crop resize API test\n");
    printf("\t--osd          (optional) : draw osd API test\n");
    printf("\t--cover        (optional) : draw line/polygon API test\n");
    printf("\t-a             (optional) : all the sync API test\n\n");

    printf("\t    -v  <PicPath>@<Format>@<Stride>x<Height>@<CropW>x<CropH>[+<CropX0>+<CropY0>]>\n");
    printf("\t   e.g: -v /opt/bin/data/ivps/800x480car.nv12@3@800x480@600x400+100+50 \n\n");
    printf("\t   [-g] <PicPath>@<Format>@<Stride>x<Height>[+<DstX0>+<DstY0>*<Alpha>]>\n");
    printf("\t   e.g: -g /opt/bin/data/ivps/rgb400x240.rgb24@161@400x240+100+50*150 \n\n");
    printf("\t   [-n] <repeat num>]\n");
    printf("\t   [-r] <region num>]\n\n");

    printf("\t<PicPath>                     : source picture path\n");
    printf("\t<Format>                      : picture color format\n");
    printf("\t\t%4d: NV12     YYYY... UVUVUV...\n", AX_FORMAT_YUV420_SEMIPLANAR);
    printf("\t\t%4d: NV21     YYYY... VUVUVU...\n", AX_FORMAT_YUV420_SEMIPLANAR_VU);
    printf("\t\t%4d: NV16     YYYY... UVUVUV...\n", AX_FORMAT_YUV422_SEMIPLANAR);
    printf("\t\t%4d: NV61     YYYY... VUVUVU...\n", AX_FORMAT_YUV422_SEMIPLANAR_VU);
    printf("\t\t%4d: RGB888   24bpp\n", AX_FORMAT_RGB888);
    printf("\t\t%4d: BGR888   24bpp\n", AX_FORMAT_BGR888);
    printf("\t\t%4d: RGB565   16bpp\n", AX_FORMAT_RGB565);
    printf("\t\t%4d: ARGB4444 16bpp\n", AX_FORMAT_ARGB4444);
    printf("\t\t%4d: RGBA4444 16bpp\n", AX_FORMAT_RGBA4444);
    printf("\t\t%4d: ARGB8888 32bpp\n", AX_FORMAT_ARGB8888);
    printf("\t\t%4d: RGBA8888 32bpp\n", AX_FORMAT_RGBA8888);
    printf("\t\t%4d: ARGB1555 16bpp\n", AX_FORMAT_ARGB1555);
    printf("\t\t%4d: RGBA5551 16bpp\n", AX_FORMAT_RGBA5551);
    printf("\t\t%4d: ARGB8565 24bpp\n", AX_FORMAT_ARGB8565);
    printf("\t\t%4d: RGBA5658 24bpp\n", AX_FORMAT_RGBA5658);
    printf("\t\t%4d: ABGR4444 16bpp\n", AX_FORMAT_ABGR4444);
    printf("\t\t%4d: BGRA4444 16bpp\n", AX_FORMAT_BGRA4444);
    printf("\t\t%4d: ABGR8888 32bpp\n", AX_FORMAT_ABGR8888);
    printf("\t\t%4d: BGRA8888 32bpp\n", AX_FORMAT_BGRA8888);
    printf("\t\t%4d: ABGR1555 16bpp\n", AX_FORMAT_ABGR1555);
    printf("\t\t%4d: BGRA5551 16bpp\n", AX_FORMAT_BGRA5551);
    printf("\t\t%4d: ABGR8565 24bpp\n", AX_FORMAT_ABGR8565);
    printf("\t\t%4d: BGRA5658 24bpp\n", AX_FORMAT_BGRA5658);
    printf("\t\t%4d: BITMAP    1bpp\n", AX_FORMAT_BITMAP);

    printf("\t<Stride>           (required) : picture stride (16 bytes aligned)\n");
    printf("\t<Stride>x<Height>  (required) : input frame stride and height (2 aligned)\n");
    printf("\t<CropW>x<CropH>    (required) : crop rect width & height (2 aligned)\n");
    printf("\t+<CropX0>+<CropY0> (optional) : crop rect coordinates\n");
    printf("\t+<DstX0>+<DstY0>   (optional) : output position coordinates\n");
    printf("\t<Alpha>            (optional) : (0: transparent; 255: opaque)\n");

    printf("\nExample1:\n");
    printf("\t/%s -v /opt/data/ivps/1920x1088.nv12@3@1920x1088@1920x1088 -n 1\n", SAMPLE_NAME);
}

typedef AX_VOID (*pHelperFunc)(AX_VOID);

#if 0
static struct
{
    pHelperFunc pHelper;
} gHelps[SAMPLE_HELP_MAX] = {
    {IVPS_ObjHelp},
};
#endif

AX_VOID ShowUsage(AX_S32 nHelpIdx)
{
#if 0
    if (nHelpIdx > SAMPLE_HELP_MIN && nHelpIdx < SAMPLE_HELP_MAX)
    {
        pHelperFunc pHelper = gHelps[nHelpIdx].pHelper;
        if (pHelper)
        {
            (*pHelper)();
            return;
        }
    }
#endif
    IVPS_ObjHelp();
}
