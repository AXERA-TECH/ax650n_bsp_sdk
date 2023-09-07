1. Function description:

    The IVPS(Image Video Process System) unit provided in the Axera SDK package is a video image processing subsystem
that provides functions such as cropping, scaling, rotation, streaming, CSC, OSD, mosaic, quadrilateral, etc.

    This module is an example code of IVPS unit, which is convenient for users to quickly understand
and master the use of IVPS related interfaces.

    sample_ivps bin is located in the directory of /opt/bin, and can be used for IVPS interface examples.
The input parameters are described as follows:

        -v             (required) : video frame input
        -g             (optional) : overlay input
        -n             (optional) : repeat number
        -r             (optional) : region config and update
        -l             (optional) : 0: no link 1. link ivps. 2: link venc. 3: link jenc
        --pipeline     (optional) : import ini file to config all the filters in one pipeline
        --pipeline_ext (optional) : import ini file to config all the filters in another pipeline
        --change       (optional) : import ini file to change parameters for one filter dynamicly
        --region       (optional) : import ini file to config region parameters
        --dewarp       (optional) : import ini file to config dewarp parameters, including LDC, perspective, fisheye, etc.
        --cmmcopy      (optional) : cmm copy API test
        --csc          (optional) : color space covert API test
        --fliprotation (optional) : flip and rotation API test
        --alphablend   (optional) : alpha blending API test
        --cropresize   (optional) : crop resize API test
        --osd          (optional) : draw osd API test
        --mosaic       (optional) : draw mosaic API test
        --cover        (optional) : draw line/polygon API test
        --pool_type    (optional) : select pool type (0(default): common pool; 1: private pool; 2: user pool)
        --grp_id       (optional) : set group id (0(default): [0, 256)
        -a             (optional) : all the sync API test


            -v  <PicPath>@<Format>@<Stride>x<Height>@<CropW>x<CropH>[+<CropX0>+<CropY0>]>
           e.g: -v /opt/bin/data/ivps/800x480car.nv12@3@800x480@600x400+100+50

           [-g] <PicPath>@<Format>@<Stride>x<Height>[+<DstX0>+<DstY0>*<Alpha>]>
           e.g: -g /opt/bin/data/ivps/rgb400x240.rgb24@161@400x240+100+50*150

           [-n] <repeat num>]
           [-r] <region num>]

        <PicPath>                     : source picture path
        <Format>                      : picture color format
                   3: NV12 YYYY... UVUVUV...
                   4: NV21 YYYY... VUVUVU...
                  10: NV16 YYYY... UVUVUV...
                  11: NV61 YYYY... VUVUVU...
                 161: RGB888 24bpp
                 165: BGR888 24bpp
                 160: RGB565 16bpp
                 197: ARGB4444 16bpp
                 203: RGBA4444 16bpp
                 199: ARGB8888 32bpp
                 201: RGBA8888 32bpp
                 198: ARGB1555 16bpp
                 202: RGBA5551 16bpp
                 200: ARGB8565 24bpp
                 208: BITMAP 1bpp
        <Stride>           (required) : picture stride (16 bytes aligned)
        <Stride>x<Height>  (required) : input frame stride and height (2 aligned)
        <CropW>x<CropH>    (required) : crop rect width & height (2 aligned)
        +<CropX0>+<CropY0> (optional) : crop rect coordinates
        +<DstX0>+<DstY0>   (optional) : output position coordinates
        <Alpha>            (optional) : (0: transparent; 255: opaque)

Notes：
    -v is a required item, with the input source image path and frame information.

    The cropping window should be within the height range of the source image, that is CropX0 + CropW <= Stride, CropY0 + CropH <= Height
If cropping is not performed, then CropW = Width, CropH = Height, CropX0 = 0, and CropY0 = 0.

    -n indicates that the source image is processed for a specified number of times. If the parameter is -1, it will be executed in a loop all the time.
If you want to view the proc information of the IVPS, you need to set the number of processing times to a large value or cycle all the time.
How to view IVPS proc information: cat proc/ax_proc/ivps。

    The input parameter after -r is the number of overlayed REGIONs, currently the maximum is 4.
The overlay of REGIONs on IVPS PIPELINE is an asynchronous operation, which requires several frames before it is really overlayed on the input source image.
Therefore, if you want to verify the REGION function, you need to set the parameter after -n to be larger, the value should be greater than 3.


2. Examples:

Example1: View help information.
    /opt/bin/sample_ivps -h

Example2: Process source image (3840x2160 NV12 format) once.
    /opt/bin/sample_ivps -v /opt/data/ivps/3840x2160.nv12@3@3840x2160@0x0+0+0 -n 1

Example3: Process source image (800x480 RGB 888 format) with cropping(X0=128 Y0=50 W=400 H=200) for three times.
    /opt/bin/sample_ivps -v /opt/data/ivps/800x480logo.rgb24@161@800x480@400x200+128+50  -n 3

Example4: Process the source image (3840x2160 NV12 format) for five times, with overlaying three REGIONs.
    /opt/bin/sample_ivps -v /opt/data/ivps/3840x2160.nv12@3@3840x2160@0x0+0+0 -n 5 -r 3


3. Results:

    After running successfully, the following images will be generated in the same directory as the source image (/opt/data/ivps),
which can be opened and viewed through a tool.

    FlipMirrorRotate_chn0_480x800.fmt_a1
    OSD_chn0_3840x2160.fmt_3
    AlphaBlend_chn0_3840x2160.fmt_3
    Rotate_chn0_1088x1920.fmt_3
    CSC_chn0_3840x2160.fmt_3
    CropResize_chn0_1280x720.fmt_3
    PIPELINEoutput_grp1chn0_1920x1080.fmt_3
    PIPELINEoutput_grp1chn1_2688x1520.fmt_a1
    PIPELINEoutput_grp1chn2_768x1280.fmt_a1

Notes：
    fmt_3：Represents NV12 format; fmt_a1：Represents RGB 888 format (a1 indicates hexadecimal 0xa1)

Execute Ctrl + C to exit.


4. Notes：

    The sample code is only used for API demonstration.
In actual development, users need to configure parameters in combination with specific business scenarios.

    The maximum resolution of input image and output image is 8192x8192.

