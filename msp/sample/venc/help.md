## Options for sample
```
  -i[s] --input                         Read input video sequence from file. [input.yuv]
  -o[s] --output                        Write output HEVC/H.264/jpeg/mjpeg stream to file.[stream.hevc]
  -W[n] --write                         whether write output stream to file.[1]
                                        0: do not write
                                        1: write

  -f[n] --dstFrameRate                  1..1048575 Output picture rate numerator. [30]

  -j[n] --srcFrameRate                  1..1048575 Input picture rate numerator. [30]

  -n[n] --encFrameNum                   the frame number want to encode. [0]
  -N[n] --chnNum                        total encode channel number. [0]
  -t[n] --encThdNum                     total encode thread number. [1]
  -p[n] --bLoopEncode                   enable loop mode to encode. 0: disable. 1: enable. [0]
  --dynAttrIdx                          When encode Nth frame, dynamically configure coding parameters.
  --codecType                           encoding payload type. [0]
                                        0 - SAMPLE_CODEC_H264
                                        1 - SAMPLE_CODEC_H265
                                        2 - SAMPLE_CODEC_MJPEG
                                        3 - SAMPLE_CODEC_JPEG
  --bChnCustom                          whether encode all payload type. [0]
                                        0 - encode all payload type
                                        1 - encode payload type codecType specified by codecType.
  --log                                 log info level. [2]
                                        0 : ERR
                                        1 : WARN
                                        2 : INFO
                                        3 : DEBUG

  --grpId                               group id for select group, rang in [0, 32). [0]
  --bCoreCoWork                         enable multi core. [0]
  --bStrmCached                         output stream use cached memory. [0]
  --bAttachHdr                          support attach headers(sps/pps) to PB frame for h.265. [0]

```

## Parameters affecting input frame and encoded frame resolutions and cropping:

```
  -w[n] --picW                          Width of input image in pixels.
  -h[n] --picH                          Height of input image in pixels.

  -X[n] --cropX                         image horizontal cropping offset, must be even. [0]
  -Y[n] --cropY                         image vertical cropping offset, must be even. [0]
  -x[n] --cropW                         Height of encoded image
  -y[n] --cropH                         Width of encoded image

  --maxPicW                             max width of input image in pixels.
  --maxPicH                             max height of input image in pixels.

  --bCrop                               enable crop encode, 0: disable 1: enable crop. [0]
```

## Parameters picture stride:
```
  --strideY                             y stride
  --strideU                             u stride
  --strideV                             v stride
```

## Parameters VUI:
```
  --bSignalPresent                      equal to 1 specifies that video_format、video_full_range、color_present are present. [1]
  --videoFormat                         Indicates the video format. [5]
  --bFullRange                          video range, 1: full range 0: limited range. [1]
  --bColorPresent                       equal to 1 specifies that colorPrimaries、transferCharacter、matrixCoeffs are present. [1]
  --colorPrimaries                      indicates the chromaticity coordinates of source primaries. [2]
  --transferCharacter                   indicates the opto-electronic transfer characteristics of the source. [2]
  --matrixCoeffs                        indicates the matrix coefficients used in the transformation from RGB to YUV color space. [2]
```
## dynamic change resolution
```
  --bDynRes                             enable change resolution
  --newInput                            new file path of changed resolution
  --newPicW                             new width
  --newPicH                             new height
```

## Parameters  for pre-processing frames before encoding:

```
  -l[n] --picFormat                     Input YUV format. [1]
                                        1 - AX_FORMAT_YUV420_PLANAR (IYUV/I420)
                                        3 - AX_FORMAT_YUV420_SEMIPLANAR (NV12)
                                        4 - AX_FORMAT_YUV420_SEMIPLANAR_VU (NV21)
                                        13 - AX_FORMAT_YUV422_INTERLEAVED_YUYV (YUYV/YUY2)
                                        14 - AX_FORMAT_YUV422_INTERLEAVED_UYVY (UYVY/Y422)
                                        37 - AX_FORMAT_YUV420_PLANAR_10BIT_I010
                                        42 - AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010
```
## Parameters  affecting GOP pattern, rate control and output stream bitrate:

```
  -g[n] --gopLen                        Intra-picture rate in frames. [30]
                                        Forces every Nth frame to be encoded as intra frame.
                                        0 = Do not force

  -B[n] --bitRate                       target bitrate for rate control, in kbps. [2000]
  --ltMaxBt                             the long-term target max bitrate.
  --ltMinBt                             the long-term target min bitrate.
  --ltStaTime                           the long-term rate statistic time.
  --shtStaTime                          the short-term rate statistic time.
  --minQpDelta                          Difference between FrameLevelMinQp and MinQp
  --maxQpDelta                          Difference between FrameLevelMaxQp and MaxQp
```

## Parameters  qp:
```
  -q[n] --qFactor                       0..99, Initial target QP of jenc. [90]

  --startQp                             -1..51, start qp for first frame. [16]
  --minQp                               0..51, Minimum frame header qp for any picture. [16]
  --maxQp                               0..51, Maximum frame header qp for any picture. [51]
  --minIqp                              0..51, Minimum frame header qp for I picture. [16]
  --maxIqp                              0..51, Maximum frame header qp for I picture. [51]
  --chgPos                              vbr/avbr chgpos 20-100, def 90
  --stillPercent                        avbr still percent 10-100 def 25
  --stillQp                             0..51, the max QP value of I frame for still scene. [0]

  --deltaQpI                            -51..51, QP adjustment for intra frames. [-2]
  --maxIprop                            1..100, the max I P size ratio. [100]
  --minIprop                            1..maxIprop, the min I P size ratio. [1]
  --IQp                                 0..51, qp of the i frame. [25]
  --PQp                                 0..51, qp of the p frame. [30]
  --BQp                                 0..51, qp of the b frame. [32]
  --fixedQp                             -1..51, Fixed qp for every frame(only for jpeg)
                                          -1 : disable fixed qp mode.
                                          [0, 51] : value of fixed qp.

  --ctbRcMode                           0: diable ctbRc; 1: quality first; 2: bitrate first 3: quality and bitrate balance
  --qpMapQpType                         0: disable qpmap; 1: deltaQp; 2: absQp
  --qpMapBlkUnit                        0: 64x64, 1: 32x32, 2: 16x16;
  --qpMapBlkType                        0: disable; 1: skip mode; 2: Ipcm mode
```

  -r[n] --rcMode                        0: CBR 1: VBR 2: AVBR 3: QPMAP 4:FIXQP 5:CVBR. [0]

  --dynRc                               change rcMode dynamically.
                                        0: CBR 1: VBR 2: AVBR 3: QPMAP 4:FIXQP 5:CVBR. [4]

  -R[n] --refreshNum                    how many frames it will take to do GDR [0]
                                        0 : disable GDR (Gradual decoder refresh),
                                        >0: enable GDR
                                        The starting point of GDR is the frame with type set to VCENC_INTRA_FRAME.
                                        intraArea and roi1Area are used to implement the GDR function. The GDR
                                        begin to work from the second IDR frame.

## Parameters compress(fbc):
```
  --fbcType                             compress mode. [0]
                                        0 - AX_COMPRESS_MODE_NONE
                                        1 - AX_COMPRESS_MODE_LOSSLESS
                                        2 - AX_COMPRESS_MODE_LOSSY
  --bitDepth                            frame bit width. [8]
                                        8 - VENC_FRAME_8BIT
                                        10 - VENC_FRAME_10BIT

  --compLevel                           0..10. compress level. [0]
  --yHdrSize                            luma header size(AX_COMPRESS_MODE_LOSSLESS). [0]
  --yPadSize                            luma payload size(AX_COMPRESS_MODE_LOSSLESS). [0]
  --uvHdrSize                           chroma header size(AX_COMPRESS_MODE_LOSSLESS). [0]
  --uvPadSize                           chroma payload size(AX_COMPRESS_MODE_LOSSLESS). [0]
```

## Parameters roi:
```
  --roiEnable                           enable roi. 0: disable. 1: enable. [0]
  --vencRoiMap                          h264/h265 roi map file. [venc_map.roi]
  --jencRoiMap                          jpeg/mjpeg roi map file. [jenc_map.roi]
  --qRoiFactor                          roi rigion qp. [0]
```

```
  -M[n] --gopMode                       gopmode. 0: normalP. 1: oneLTR. 2: svc-t. [0]

  --temporalID                          filter the bit streams at different layer by temporalID in svc-t mode. [2]
                                          0 : save the (0) layer bit stream.
                                          1 : save the (0、1) layer bit stream.
                                          2 : save the (0、1、2) layer bit stream.
```
## other:
```
  --bPerf                               enable encoder performance test. [0]
  --vbCnt                               total frame buffer number of pool [1, 100]. [10]
  --inFifoDep                           input fifo depth. [4]
  --outFifoDep                          output fifo depth. [4]

  --bLinkMode
  --strmBitDep                          encode stream bit depth. [8]
                                         8 : encode 8bit
                                         10: encode 10bit

  --strmBufSize                         output stream buffer size. [0]
                                        0: use default memory setting in sdk.
                                        >0：alloc some memory by user.

  --virILen                             virtual I frame duration. should less than gop length.
  --bIDR                                Request IDR.
                                          0: insert IDR consider of frame rate control, not instant.
                                          1: insert IDR instant

  --sliceNum                           0 : one frame encode in one slice
                                        n(> 0) ：n macro line make a slice
  --uDataSize                          user data size. h264/h265 [1, 2048]; jpeg [1, 4096]. [10]
```
## feature test
```
  --ut                                  encoder feature test. [0]
                                        1024: run all case below.
                                        0: UT_CASE_NORMAL               1: UT_CASE_BIT_RATE
                                        2: UT_CASE_RESET_CHN            3: UT_CASE_VENC_ROI
                                        4: UT_CASE_FRAME_RATE           5: UT_CASE_CHN_ATTR
                                        6: UT_CASE_RC_MODE              7: UT_CASE_VUI
                                        8: UT_CASE_JPEG_ENCODE_ONCE     9: UT_CASE_JPEG_PARAM
                                        10:UT_CASE_VIR_INTRA_INTERVAL   11:UT_CASE_INTRA_REFRESH
                                        12:UT_CASE_RESOLUTION           13:UT_CASE_REQUEST_IDR
                                        14:UT_CASE_SELECT_CHN           15:UT_CASE_SET_USR_DATA
                                        16:UT_CASE_RATE_JAM             17:UT_CASE_SUPER_FRAME
                                        18:UT_CASE_SLICE_SPLIT          19:UT_CASE_CREATE_DESTROY
```