CUR_PATH		    := $(shell pwd)
HOME_PATH		    := $(abspath $(CUR_PATH)/../../../../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME        := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH        := $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH        := $(HOME_PATH)/msp/out
SRC_PATH            := $(CUR_PATH)
LIB_PATH            := $(MSP_OUT_PATH)/lib
INC_PATH            := $(MSP_OUT_PATH)/include
SRC_RELATIVE_PATH   := $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH     := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH  := $(MSP_OUT_PATH)
DEMO_SRC_PATH       := $(HOME_PATH)/app/demo/src
DEMO_BIN_PATH       := $(HOME_PATH)/app/demo/bin/boxdemo
THIRD_PARTY_PATH    := $(HOME_PATH)/third-party
3RD_PATH        	:= $(DEMO_SRC_PATH)/3rd

# output
MOD_NAME            := boxdemo
OUTPUT              := $(TARGET_OUT_PATH)/obj

# source
SRCS                :=
SRCCPPS             := $(wildcard $(SRC_PATH)/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/rtsp/AXRTSPClient.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/vdec/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/vdec/stream/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/log/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/stage/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/vo/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/encoder/VideoEncoder.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/mp4enc/Mpeg4Encoder.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/pool/AXPoolManager.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/utils/ElapsedTimer.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/utils/yuv/BmpOSD.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/utils/yuv/YuvHandler.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/utils/SpsParser.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/config/ini/*.cpp) \
                       $(DEMO_SRC_PATH)/ppl/nvr/hal/rtspdamon.cpp \
                       $(DEMO_SRC_PATH)/ppl/nvr/hal/linker.cpp \
                       $(DEMO_SRC_PATH)/utils/ping4.cpp \
                       $(DEMO_SRC_PATH)/utils/DiskHelper.cpp \
                       $(DEMO_SRC_PATH)/utils/TimestampHelper.cpp

CINCLUDE            := -I$(SRC_PATH) \
                       -I$(INC_PATH) \
                       -I$(DEMO_SRC_PATH)/header \
                       -I$(DEMO_SRC_PATH)/rtsp \
					   -I$(DEMO_SRC_PATH)/vdec \
                       -I$(DEMO_SRC_PATH)/encoder \
                       -I$(DEMO_SRC_PATH)/vdec/stream \
                       -I$(DEMO_SRC_PATH)/vdec/drv \
                       -I$(DEMO_SRC_PATH)/utils \
                       -I$(DEMO_SRC_PATH)/utils/yuv \
                       -I$(DEMO_SRC_PATH)/log \
                       -I$(DEMO_SRC_PATH)/stage \
                       -I$(DEMO_SRC_PATH)/vo \
                       -I$(DEMO_SRC_PATH)/pool \
                       -I$(DEMO_SRC_PATH)/encoder\
                       -I$(DEMO_SRC_PATH)/mp4enc \
                       -I$(DEMO_SRC_PATH)/config/ini \
                       -I$(DEMO_SRC_PATH)/ppl/nvr/hal \
                       -I$(THIRD_PARTY_PATH)/ffmpeg/include \
                       -I$(THIRD_PARTY_PATH)/openssl/include \
                       -I$(THIRD_PARTY_PATH)/mp4/include \
                       -I$(3RD_PATH)/inc/live/BasicUsageEnvironment \
                       -I$(3RD_PATH)/inc/live/groupsock \
                       -I$(3RD_PATH)/inc/live/liveMedia \
                       -I$(3RD_PATH)/inc/live/UsageEnvironment

 OBJS               := $(SRCS:%.c=$(OUTPUT)/%.o)
 CPPOBJS            := $(SRCCPPS:%.cpp=$(OUTPUT)/%.o)

 DEPS               := $(OBJS:%.o=%.d)
 CPPDEPS            := $(CPPOBJS:%.o=%.d)

# exec
TARGET              := $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CPPFLAGS            := --std=c++11
CFLAGS              := $(CPPFLAGS) -Wall -Wl,-rpath-link=$(LIB_PATH):$(THIRD_PARTY_PATH)/ffmpeg/lib:-L$(THIRD_PARTY_PATH)/openssl/lib:-L$(THIRD_PARTY_PATH)/mp4/lib
CFLAGS              += -DAPP_BUILD_VERSION=\"$(SDK_VERSION)\"

# internal debug
# CFLAGS            += -D__BOX_DEBUG__

# dummy vo: not send to vo
# CFLAGS            += -D__DUMMY_VO__

# dump vdec decoded yuv saved in ./dump_stridexh_grpx_chnx.nv12.img
# CFLAGS            += -D__DUMP_VDEC_FRAME__

# dump nalu before send to vdec
# CFLAGS            += -D__DUMP_VDEC_NALU__

CFLAGS              += -D__VDEC_PP_FRAME_CTRL__

CFLAGS  += -D__LINUX_IO_API__
CFLAGS  += -D__RECORD_VB_TIMESTAMP__

ifeq ($(debug),yes)
CFLAGS              += -O0 -ggdb3
else
CFLAGS              += -O2
endif

# dependency
CLIB                := -lstdc++ -pthread -lm -lrt
CLIB                += -L$(LIB_PATH)
CLIB                += -lax_sys
CLIB                += -lax_vdec
CLIB                += -lax_ivps
CLIB                += -lax_venc
CLIB                += -lax_engine
CLIB                += -lax_skel
CLIB                += -lax_vo
CLIB                += -L$(THIRD_PARTY_PATH)/drm/lib -ldrm
CLIB                += -L$(THIRD_PARTY_PATH)/ffmpeg/lib -lavcodec -lavutil -lavformat
CLIB                += -L$(THIRD_PARTY_PATH)/openssl/lib -lssl -lcrypto
CLIB                += -L$(THIRD_PARTY_PATH)/mp4/lib -l:libmp4.a
ifeq ($(debug),yes)
CLIB                += -L$(3RD_PATH)/lib/dbg -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment
else
CLIB                += -L$(3RD_PATH)/lib -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment
endif


# install
INSTALL_TARGET      := $(TARGET)
INSTALL_TARGET      += $(DEMO_BIN_PATH)/*
INSTALL_DIR         := $(ROOTFS_TARGET_PATH)/bin/BoxDemo

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak

