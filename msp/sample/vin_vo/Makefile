################################################################################
#	prepare param
################################################################################
CUR_PATH		:= $(shell pwd)
HOME_PATH		:= $(abspath $(CUR_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME		:= $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH		:= $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH		:= $(HOME_PATH)/msp/out
OUT_PATH			:= $(MSP_OUT_PATH)
SRC_PATH			:= $(CUR_PATH)
SAMPLE_PATH			:= $(CUR_PATH)/..
SRC_RELATIVE_PATH	:= $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH		:= $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH 	?= $(MSP_OUT_PATH)

LIB_PATH			:= $(OUT_PATH)/lib

################################################################################
#	set task
################################################################################
# output
MOD_NAME 		:= sample_vin_vo
OUTPUT       	:= $(TARGET_OUT_PATH)/obj

# source
SRCS 			:= $(wildcard $(SRC_PATH)/*.c)
SRCS			+= $(SAMPLE_PATH)/common/common_arg_parse.c
SRCS			+= $(SAMPLE_PATH)/common/common_cam.c
SRCS			+= $(SAMPLE_PATH)/common/common_sys.c
SRCS			+= $(SAMPLE_PATH)/common/common_vin.c
SRCS			+= $(SAMPLE_PATH)/common/common_isp.c
SRCS			+= $(SAMPLE_PATH)/common/common_nt.c
SRCS			+= $(SAMPLE_PATH)/common/common_vo.c
SRCS			+= $(SAMPLE_PATH)/common/common_vo_pattern.c
SRCS			+= $(SAMPLE_PATH)/common/common_hw.c

vpath %.c $(addsuffix :,$(dir $(patsubst %.c,%.o,$(SRCS))))

CINCLUDE		+= -I$(SRC_PATH) \
			   -I$(OUT_PATH)/include \
			   -I$(SAMPLE_PATH)/common

OBJS 			:= $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))
DEPS			:= $(OBJS:%.o=%.d)
# exec
TARGET			:= $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
ifeq ($(debug),yes)
CFLAGS			+= -Wall -O0 -ggdb3
else
CFLAGS			+= -Wall -O2
endif

CFLAGS			+= -DISP_PIPELINE_INVOLE_NPU
CFLAGS			+= -ldl

# dependency
CLIB			+= -Wl,-rpath-link=$(OUT_PATH)/lib
CLIB			+= -L$(OUT_PATH)/lib
CLIB			+= -lax_sys
CLIB			+= -lax_3a
CLIB			+= -lax_proton
CLIB			+= -lax_engine
CLIB			+= -lax_mipi
CLIB			+= -lm -lpthread
CLIB			+= -lax_nt_stream
CLIB			+= -lax_nt_ctrl
CLIB			+= -lax_vo
CLIB			+= -lax_ivps
CLIB			+= -L$(HOME_PATH)/third-party/drm/lib
CLIB			+= -ldrm

# install
INSTALL_TARGET	:= $(TARGET)
INSTALL_DIR		:= $(ROOTFS_TARGET_PATH)/bin/

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak

export ROOTFS_TARGET_PATH
