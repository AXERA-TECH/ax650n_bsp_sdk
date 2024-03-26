CUR_PATH		:= $(shell pwd)
HOME_PATH		:= $(abspath $(CUR_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME		:= $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH		:= $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH		:= $(HOME_PATH)/msp/out
OUT_PATH		:= $(MSP_OUT_PATH)
SRC_PATH		:= $(CUR_PATH)
SRC_RELATIVE_PATH	:= $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH		:= $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH 	?= $(MSP_OUT_PATH)

BASE_PATH		:= $(HOME_PATH)/msp/component
LIB_PATH		:= $(MSP_OUT_PATH)/lib
COMMON_PATH		:= $(BASE_PATH)/common

include $(HOME_PATH)/build/config.mak

# output
MOD_NAME                := sample_dsp
OUTPUT                  := $(TARGET_OUT_PATH)/obj

# source
SRCS                    := $(wildcard $(SRC_PATH)/*.c)
OBJS                    := $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))
CINCLUDE                := -I$(SRC_PATH) -I$(SRC_PATH)/include -I$(HOME_PATH)/msp/out/include -I$(HOME_PATH)/msp/component/include

# exec
TARGET                  := $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CFLAGS			:= -Wall -Wl,-rpath-link=$(LIB_PATH)
ifeq ($(debug),yes)
CFLAGS			+= -O0 -ggdb3
else
CFLAGS			+= -O2
endif

# dependency
CLIB			+= -L$(LIB_PATH) -lax_sys -lax_dsp -lpthread -lax_dsp_cv

INSTALL_BIN		:= $(TARGET)
INSTALL_DATA	:= $(CUR_PATH)/data/dsp

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak

