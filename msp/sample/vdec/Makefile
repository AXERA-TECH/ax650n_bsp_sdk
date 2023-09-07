CUR_PATH            := $(shell pwd)
HOME_PATH           := $(abspath $(CUR_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME        := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH        := $(PRJ_OUT_HOME)/objs

SRC_PATH            := $(CUR_PATH)
THIRD_PARTY_PATH    := $(HOME_PATH)/third-party
BASE_PATH           := $(HOME_PATH)/msp/component
MSP_OUT_PATH        := $(HOME_PATH)/msp/out
LIB_PATH            := $(MSP_OUT_PATH)/lib
COMMON_PATH         := $(CUR_PATH)/../common
SAMPLE_PATH         := $(CUR_PATH)/..

SRC_RELATIVE_PATH   := $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH     := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH  ?= $(MSP_OUT_PATH)


# output
MOD_NAME        := sample_vdec
OUTPUT          := $(TARGET_OUT_PATH)/obj
# source
SRCS    += $(COMMON_PATH)/common_arg_parse.c
SRCS    += $(SAMPLE_PATH)/common/common_vdec.c
SRCS    += $(COMMON_PATH)/common_vdec_cmd.c
SRCS    += $(COMMON_PATH)/common_vdec_utils.c

SRCS    += $(SRC_PATH)/common/sample_vdec.c
SRCS    += $(SRC_PATH)/sample_main.c



vpath %.c $(addsuffix :,$(dir $(patsubst %.c,%.o,$(SRCS))))


CINCLUDE        += -I$(SRC_PATH)
CINCLUDE        += -I$(MSP_OUT_PATH)/include
CINCLUDE        += -I$(BASE_PATH)/codec/common
CINCLUDE        += -I$(COMMON_PATH)
CINCLUDE        += -I$(CUR_PATH)/common
CINCLUDE        += -I$(BASE_PATH)/sys
CINCLUDE        += -I$(BASE_PATH)/sys/cmm
CINCLUDE        += -I$(SAMPLE_PATH)/common
CINCLUDE        += -I$(HOME_PATH)/header/internal
CINCLUDE        += -I$(THIRD_PARTY_PATH)/ffmpeg/include/


OBJS            := $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))

DEPS            := $(OBJS:%.o=%.d)
# exec
TARGET          := $(MOD_NAME)

# build flags
CFLAGS          := -Wall -Wl,-rpath-link=$(LIB_PATH)
debug=yes
ifeq ($(debug),yes)
CFLAGS          += -O2
CFLAGS          += -ggdb3
else
CFLAGS          += -O2
endif

CFLAGS          += -Werror
CFLAGS          += -Werror=implicit-function-declaration
CFLAGS          += -Wstack-usage=2097152
CFLAGS          += -DAX_VDEC_FFMPEG_ENABLE

CLIB            += -L$(THIRD_PARTY_PATH)/ffmpeg/lib -lswresample -lavcodec -lavutil -lavformat

# dependency
CLIB            += -L$(LIB_PATH) -lax_vdec
CLIB            += -lax_sys
CLIB            += -lm -lpthread
CLIB            += -g


# install
INSTALL_BIN     := $(TARGET)
INSTALL_TARGET  := $(TARGET)
INSTALL_DIR     := $(ROOTFS_TARGET_PATH)/bin/
INSTALL_DATA    := $(CUR_PATH)/data/vdec

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak

