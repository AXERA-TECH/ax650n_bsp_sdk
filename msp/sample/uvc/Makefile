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

SAMPLE_COMMON_PATH			:= $(SAMPLE_PATH)/common
SAMPLE_VENC_PATH 			:= $(SAMPLE_PATH)/venc
SAMPLE_VENC_COMMON_PATH		:= $(SAMPLE_VENC_PATH)/common
################################################################################
#	set task
################################################################################
# output
MOD_NAME 		:= sample_uvc
SCRIPT_NAME		:= uvc-gadget-composite.sh
README_NAME		:= README.md

OUTPUT 			:= $(TARGET_OUT_PATH)/obj

# source
SRCS 			:= $(wildcard $(SRC_PATH)/*.c) \
				   $(wildcard $(SAMPLE_VENC_COMMON_PATH)/*.c)

SRCS			+= $(SAMPLE_COMMON_PATH)/common_cam.c
SRCS			+= $(SAMPLE_COMMON_PATH)/common_sys.c
SRCS			+= $(SAMPLE_COMMON_PATH)/common_venc.c
SRCS			+= $(SAMPLE_COMMON_PATH)/common_venc_log.c
SRCS			+= $(SAMPLE_COMMON_PATH)/common_vin.c
SRCS			+= $(SAMPLE_COMMON_PATH)/common_isp.c
SRCS			+= $(SAMPLE_COMMON_PATH)/common_nt.c
SRCS			+= $(SAMPLE_COMMON_PATH)/common_hw.c

vpath %.c $(addsuffix :,$(dir $(patsubst %.c,%.o,$(SRCS))))

CINCLUDE		+= -I$(SRC_PATH) \
			       -I$(OUT_PATH)/include \
			       -I$(SAMPLE_COMMON_PATH) \
				   -I$(SAMPLE_VENC_PATH) \
				   -I$(SAMPLE_VENC_PATH)/featureTest \
				   -I$(SAMPLE_VENC_COMMON_PATH)

OBJS 			:= $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))
DEPS			:= $(OBJS:%.o=%.d)

# exec
TARGET			:= $(MOD_NAME)

ifeq ($(debug),yes)
CFLAGS			+= -Wall -O0 -ggdb3
else
CFLAGS			+= -Wall -O2
endif

CFLAGS			+= -DISP_PIPELINE_INVOLE_NPU
CFLAGS			+= -ldl

ifeq ($(test_latency),yes)
CFLAGS			+= -DTEST_LATENCY
endif

# dependency
CLIB			+= -Wl,-rpath-link=$(LIB_PATH)
CLIB			+= -L$(LIB_PATH)
CLIB			+= -lax_venc
CLIB			+= -lax_sys
CLIB			+= -lax_3a
CLIB			+= -lax_proton
CLIB			+= -lax_mipi
CLIB			+= -lm -lpthread
CLIB			+= -lax_nt_stream
CLIB			+= -lax_nt_ctrl
CLIB			+= -lax_engine
CLIB			+= -lax_dsp
CLIB			+= -lax_dsp_cv

# install
INSTALL_TARGET	:= $(TARGET)
INSTALL_TARGET	+= $(SCRIPT_NAME)
INSTALL_TARGET  += $(README_NAME)
INSTALL_DIR		:= $(ROOTFS_TARGET_PATH)/bin/sample_uvc
INSTALL_DATA	:= $(CUR_PATH)/data/uvc

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak

export ROOTFS_TARGET_PATH
