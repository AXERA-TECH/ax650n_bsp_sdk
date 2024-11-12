CUR_PATH		:= $(shell pwd)
HOME_PATH		:= $(abspath $(CUR_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME		:= $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH		:= $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH		:= $(OBJ_OUT_PATH)/msp/out
OUT_PATH			:= $(HOME_PATH)/msp/out
LIB_PATH			:= $(OUT_PATH)/lib
INC_PATH			:= $(OUT_PATH)/include
SRC_PATH			:= $(CUR_PATH)
SRC_RELATIVE_PATH	:= $(subst $(HOME_PATH)/,,$(SRC_PATH))
THIRD_PARTY_PATH    := $(HOME_PATH)/third-party
TARGET_OUT_PATH		:= $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH 	:= $(OUT_PATH)

# output
MOD_NAME			:= sample_video_convert
OUTPUT				:= $(TARGET_OUT_PATH)/.obj

# source
SRCS				:= $(wildcard $(SRC_PATH)/*.c)

CINCLUDE			:= -I$(SRC_PATH) \
					   -I$(INC_PATH) \
					   -I$(THIRD_PARTY_PATH)/ffmpeg/include \


OBJS				:= $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))

# exec
TARGET				:= $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CFLAGS				:= -Wall -Wl,-rpath-link=$(LIB_PATH):$(THIRD_PARTY_PATH)/ffmpeg/lib
CFLAGS              += -D__BUILD_VERSION__=\"$(SDK_VERSION)\"
CFLAGS              += -D__BUILD_NAME__=\"$(MOD_NAME)\"

ifeq ($(debug),yes)
CFLAGS				+= -O0 -ggdb3
else
CFLAGS				+= -O2
endif

# dependency
CLIB				+= -L$(LIB_PATH)
CLIB				+= -lax_sys
CLIB				+= -lax_vdec
CLIB				+= -lax_venc
CLIB                += -L$(THIRD_PARTY_PATH)/ffmpeg/lib -lavcodec -lavutil -lavformat
CLIB 				+= -lpthread -lm

INSTALL_TARGET		:= $(TARGET)
INSTALL_DIR         := $(ROOTFS_TARGET_PATH)/bin/

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak