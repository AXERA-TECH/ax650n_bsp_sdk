CUR_PATH		    := $(shell pwd)
HOME_PATH		    := $(abspath $(CUR_PATH)/../../../../../../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME        := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH        := $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH        := $(HOME_PATH)/msp/out
MSP_LIB_PATH        := $(MSP_OUT_PATH)/lib
MSP_INC_PATH        := $(MSP_OUT_PATH)/include
SRC_PATH            := $(CUR_PATH)
SRC_RELATIVE_PATH   := $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH     := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH  := $(MSP_OUT_PATH)
THIRD_PARTY_PATH    := $(HOME_PATH)/third-party
APP_SRC_PATH        := $(HOME_PATH)/app/demo/src
APP_3RD_PATH        := $(APP_SRC_PATH)/3rd
NVR_SRC_PATH        := $(APP_SRC_PATH)/ppl/nvr
NVR_BIN_PATH        := $(HOME_PATH)/app/demo/bin/nvrdemo
NVR_HAL_PATH        := $(NVR_SRC_PATH)/hal

# output
MOD_NAME            := nvrhal
OUTPUT              := $(TARGET_OUT_PATH)/obj

# source
SRCS                :=
SRCCPPS             := $(wildcard $(SRC_PATH)/*.cpp) \
					   $(wildcard $(APP_SRC_PATH)/log/*.cpp) \
                       $(NVR_HAL_PATH)/ut/help.cpp \
                       $(NVR_HAL_PATH)/linker.cpp \
                       $(NVR_HAL_PATH)/ivps.cpp \
                       $(NVR_HAL_PATH)/vo.cpp \
                       $(NVR_HAL_PATH)/vdec.cpp \
                       $(NVR_HAL_PATH)/istream.cpp \
                       $(NVR_HAL_PATH)/ffmpegstream.cpp \
                       $(NVR_HAL_PATH)/rtspstream.cpp \
                       $(NVR_HAL_PATH)/rtspdamon.cpp \
                       $(APP_SRC_PATH)/rtsp/AXRTSPClient.cpp \
                       $(APP_SRC_PATH)/utils/ElapsedTimer.cpp \
                       $(APP_SRC_PATH)/utils/SpsParser.cpp \
                       $(APP_SRC_PATH)/utils/ping4.cpp \
                       $(APP_SRC_PATH)/utils/fpsctrl.cpp


CINCLUDE            := -I$(SRC_PATH) \
                       -I$(MSP_INC_PATH) \
					   -I$(NVR_HAL_PATH) \
                       -I$(NVR_HAL_PATH)/ut \
                       -I$(APP_SRC_PATH)/header \
                       -I$(APP_SRC_PATH)/utils \
                       -I$(APP_SRC_PATH)/log \
					   -I$(APP_SRC_PATH)/pool \
                       -I$(APP_SRC_PATH)/rtsp \
                       -I$(NVR_SRC_PATH)/frm \
                       -I$(THIRD_PARTY_PATH)/openssl/include \
                       -I$(THIRD_PARTY_PATH)/cmdline \
                       -I$(THIRD_PARTY_PATH)/ffmpeg/include \
                       -I$(APP_3RD_PATH)/inc/live/BasicUsageEnvironment \
                       -I$(APP_3RD_PATH)/inc/live/groupsock \
                       -I$(APP_3RD_PATH)/inc/live/liveMedia \
                       -I$(APP_3RD_PATH)/inc/live/UsageEnvironment


 OBJS               := $(SRCS:%.c=$(OUTPUT)/%.o)
 CPPOBJS            := $(SRCCPPS:%.cpp=$(OUTPUT)/%.o)

 DEPS               := $(OBJS:%.o=%.d)
 CPPDEPS            := $(CPPOBJS:%.o=%.d)

# exec
TARGET              := $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CPPFLAGS            := --std=c++14
CFLAGS              := $(CPPFLAGS) -Wall -Wl,-rpath-link=$(MSP_LIB_PATH):$(THIRD_PARTY_PATH)/ffmpeg/lib:-L$(THIRD_PARTY_PATH)/openssl/lib
CFLAGS              += -DAPP_BUILD_VERSION=\"$(SDK_VERSION)\"
CFLAGS              += -D__DEBUG_STOP__


ifeq ($(debug),yes)
CFLAGS              += -O0 -ggdb3
else
CFLAGS              += -O2
endif

# dependency
CLIB                := -lstdc++ -pthread -lm -lrt
CLIB                += -L$(MSP_LIB_PATH)
CLIB                += -lax_sys
CLIB                += -lax_vdec
CLIB                += -lax_ivps
CLIB                += -lax_vo
CLIB                += -L$(THIRD_PARTY_PATH)/drm/lib -ldrm
CLIB                += -L$(THIRD_PARTY_PATH)/ffmpeg/lib -lavcodec -lavutil -lavformat
CLIB                += -L$(THIRD_PARTY_PATH)/openssl/lib -lssl -lcrypto
CLIB                += -L$(APP_3RD_PATH)/lib -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment


# install
INSTALL_TARGET      := $(TARGET)
INSTALL_DIR         := $(ROOTFS_TARGET_PATH)/bin/NVRDemo

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak

