################################################################################
#	prepare param
################################################################################
CUR_PATH			:= $(shell pwd)
HOME_PATH			:= $(abspath $(CUR_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME                    := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH                    := $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH                    := $(HOME_PATH)/msp/out
SRC_PATH                        := $(CUR_PATH)
OUT_PATH                        := $(MSP_OUT_PATH)
SAMPLE_PATH                     := $(CUR_PATH)/..
LIB_PATH                        := $(MSP_OUT_PATH)/lib
SRC_RELATIVE_PATH               := $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH                 := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH              := $(MSP_OUT_PATH)
SSL_PATH                        := $(HOME_PATH)/third-party/openssl
LIVE_PATH                       := $(HOME_PATH)/third-party/live/out

################################################################################
#	set task
################################################################################
MOD_NAME 			= sample_vin_ivps_venc_rtsp
OUTPUT 				:= $(TARGET_OUT_PATH)/obj

# source
SRCS 				:= $(wildcard $(SRC_PATH)/*.c) \
                                   $(wildcard $(SRC_PATH)/../common/common_venc.c) \
				   $(wildcard $(SRC_PATH)/../common/common_venc_log.c)
SRCS			+= $(SAMPLE_PATH)/common/common_arg_parse.c
SRCS			+= $(SAMPLE_PATH)/common/common_cam.c
SRCS			+= $(SAMPLE_PATH)/common/common_sys.c
SRCS			+= $(SAMPLE_PATH)/common/common_vin.c
SRCS			+= $(SAMPLE_PATH)/common/common_isp.c
SRCS			+= $(SAMPLE_PATH)/common/common_nt.c
SRCS			+= $(SAMPLE_PATH)/common/common_hw.c
SRCCPPS 			:= $(wildcard $(SAMPLE_PATH)/rtsp/*.cpp) \
                                   $(wildcard $(SRC_PATH)/*.cpp)

vpath %.c $(addsuffix :,$(dir $(patsubst %.c,%.o,$(SRCS))))

CINCLUDE 			:= -I$(SRC_PATH) \
                                   -I$(MSP_OUT_PATH)/include \
                                   -I$(SRC_PATH)/../common/include \
				   -I$(SRC_PATH)/../common/ \
                                   -I$(HOME_PATH)/msp/out \
                                   -I$(SAMPLE_PATH)/rtsp \
                                   -I$(LIVE_PATH)/include/BasicUsageEnvironment \
				   -I$(LIVE_PATH)/include/groupsock \
				   -I$(LIVE_PATH)/include/liveMedia \
				   -I$(LIVE_PATH)/include/UsageEnvironment \
				   -I$(SSL_PATH)/include \
			       -I$(OUT_PATH)/include \
			       -I$(SAMPLE_PATH)/common


OBJS 				:= $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))
OBJS                            += $(SRCCPPS:%.cpp=$(OUTPUT)/%.o)
DEPS				:= $(OBJS:%.o=%.d)

# exec
TARGET				:= $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CFLAGS				:=  -Wall -Wl,-rpath-link=$(LIB_PATH)

debug = no
ifeq ($(debug),yes)
CFLAGS				+= -O0 -ggdb3 -Wno-unused
else
CFLAGS				+= -O2 -Wno-unused
endif

# dependency
CLIB                += -L$(LIB_PATH)
CLIB                += -L$(OUT_PATH)/lib
CLIB                += -lax_sys
CLIB                += -lax_3a
CLIB                += -lax_proton
CLIB                += -lax_engine
CLIB                += -lax_mipi
CLIB                += -lax_nt_stream
CLIB                += -lax_nt_ctrl
CLIB				+= -lax_ivps
CLIB				+= -lax_venc
CLIB				+= -lm -lpthread -lrt -ldl -lstdc++
CLIB				+= -L$(SSL_PATH)/lib -L$(LIVE_PATH)/lib -lssl -lcrypto -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment

# install
INSTALL_TARGET			:= $(TARGET)
INSTALL_DIR			:= $(ROOTFS_TARGET_PATH)/bin/

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak
