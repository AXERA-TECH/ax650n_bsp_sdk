CUR_PATH            := $(shell pwd)
HOME_PATH           := $(CUR_PATH)/../../..
include $(HOME_PATH)/build/config.mak
SRC_PATH            := $(CUR_PATH)
THIRD_PARTY_PATH    := $(HOME_PATH)/third-party
BASE_PATH           := $(HOME_PATH)/msp/component
MSP_OUT_PATH        := $(HOME_PATH)/msp/out
LIB_PATH            := $(MSP_OUT_PATH)/lib
COMMON_PATH         := $(CUR_PATH)/../common
SAMPLE_PATH         := $(CUR_PATH)/..
ROOTFS_TARGET_PATH  ?= $(MSP_OUT_PATH)


MOD_NAME = sample_vdec_ivps_vo
OUTPUT              := .obj
# source
SRCS    += $(COMMON_PATH)/common_arg_parse.c
SRCS    += $(SAMPLE_PATH)/common/common_vdec.c
SRCS    += $(COMMON_PATH)/common_vdec_cmd.c
SRCS    += $(COMMON_PATH)/common_vdec_utils.c
SRCS    += $(SAMPLE_PATH)/common/common_vo.c
SRCS    += $(SAMPLE_PATH)/common/common_vo_pattern.c

SRCS    += $(SRC_PATH)/sample_vdec_hal.c
SRCS    += $(SRC_PATH)/sample_ivps_hal.c
SRCS    += $(SRC_PATH)/sample_vo_hal.c

SRCS    += $(SRC_PATH)/sample_vdec_ivps_vo.c


vpath %.c $(addsuffix :,$(dir $(patsubst %.c,%.o,$(SRCS)))))


CINCLUDE        += -I$(SRC_PATH)
CINCLUDE        += -I$(MSP_OUT_PATH)/include
CINCLUDE        += -I$(BASE_PATH)/codec/common
CINCLUDE        += -I$(BASE_PATH)/sys
CINCLUDE        += -I$(BASE_PATH)/sys/cmm
CINCLUDE        += -I$(HOME_PATH)/third-party/openssl/include
CINCLUDE        += -I$(SAMPLE_PATH)/common
CINCLUDE        += -I$(HOME_PATH)/header/internal
CINCLUDE        += -I$(THIRD_PARTY_PATH)/ffmpeg/include/


OBJS            := $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))
DEPS            := $(OBJS:%.o=%.d)
# exec
TARGET          := $(MOD_NAME)

# build flags
CFLAGS          := -Wall -Wl,-rpath-link=$(LIB_PATH)
debug = yes
ifeq ($(debug),yes)
CFLAGS          += -O0
CFLAGS          += -ggdb3
else
CFLAGS          += -O2 -Wno-unused
endif

CFLAGS          += -Werror
CFLAGS          += -Werror=implicit-function-declaration
CFLAGS          += -Wstack-usage=2097152
CFLAGS          += -DAX_VDEC_FFMPEG_ENABLE
CLIB            += -L$(THIRD_PARTY_PATH)/ffmpeg/lib -lswresample -lavcodec -lavutil -lavformat

# dependency
CLIB            += -L$(LIB_PATH) -lax_vdec
CLIB            += -lax_ivps
CLIB            += -lax_vo
CLIB            += -L$(HOME_PATH)/third-party/drm/lib
CLIB            += -ldrm
CLIB            += -lax_sys
CLIB            += -lm -lpthread
CLIB            += -L$(THIRD_PARTY_PATH)/openssl/lib
CLIB            += -lssl -lcrypto

# install
INSTALL_BIN     := $(TARGET)
#INSTALL_DATA    := $(CUR_PATH)/data/vdec

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak
