#!/bin/sh

YUV_FRM0=1920_1080_nv12_9frm.yuv
YUV_FRM1=1280_720_nv12_9frm.yuv

# Test all cases with default parameters
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0}

# UT_CASE_NORMAL -- ONELTR
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 0 -M 1 -N 2 -p 1 -n 100 -g 10 --virILen 5

# UT_CASE_NORMAL -- SVC_T
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 0 -M 2 -N 2 -p 1 -n 100

# UT_CASE_FRAME_RATE -CBR
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 4 -p 1 -n 30 -r 0

# UT_CASE_FRAME_RATE -VBR
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 4 -p 1 -n 30 -r 1

# UT_CASE_FRAME_RATE -AVBR
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 4 -p 1 -n 30 -r 2

# UT_CASE_FRAME_RATE -QPMAP
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 4 -p 1 -n 30 -r 3 --qpMapQpType 2

# UT_CASE_FRAME_RATE -FIXQP
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 4 -p 1 -n 30 -r 4

# UT_CASE_CHN_ATTR
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 5 --bCrop 1 -X 0 -Y 0 -x 400 -y 400

# UT_CASE_INTRA_REFRESH
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 11 -g 30 -R 15 -n 60

# UT_CASE_INTRA_REFRESH 1920x1080 -> 1280x720
/opt/bin/sample_venc -w 1920 -h 1080 -i  ${YUV_FRM0} --bDynRes 1 --newInput ${YUV_FRM1} --newPicW 1280 --newPicH 720 --ut 12 --dynAttrIdx 30 -n 60 -p 1

# UT_CASE_INTRA_REFRESH 1280x720 -> 1920x1080
/opt/bin/sample_venc -w 1280 -h 720 -i  ${YUV_FRM1} --bDynRes 1 --newInput ${YUV_FRM0} --newPicW 1920 --newPicH 1080 --ut 12 --dynAttrIdx 30 -n 60 -p 1

# UT_CASE_SET_USR_DATA
/opt/bin/sample_venc -w 1920 -h 1080 -l 3 -i ${YUV_FRM0} --ut 15  -p 1 -n 60 -g 10