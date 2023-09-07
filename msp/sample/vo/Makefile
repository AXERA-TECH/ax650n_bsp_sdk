################################################################################
#	prepare param
################################################################################
CUR_PATH				:= $(shell pwd)
SAMPLE_PATH				:= $(CUR_PATH)/..
HOME_PATH				:= $(CUR_PATH)/../../..
SRC_PATH				:= $(CUR_PATH)
BASE_PATH				:= $(HOME_PATH)/msp/component
OUT_PATH				:= $(HOME_PATH)/msp/out
LIB_PATH				:= $(OUT_PATH)/lib
THIRD_PARTY_PATH  		:= $(HOME_PATH)/third-party
CONFIG_PATH	            := $(CUR_PATH)/config
ROOTFS_TARGET_PATH 			?= $(OUT_PATH)

include $(HOME_PATH)/build/config.mak

################################################################################
#	set task
################################################################################
MOD_NAME 			= sample_vo
OUTPUT 				:= .obj

# source
SRCS 				:= $(wildcard $(SRC_PATH)/*.c)
SRCS 				+= $(wildcard $(SRC_PATH)/common/*.c)
SRCS				+= $(SAMPLE_PATH)/common/common_vo.c
SRCS				+= $(SAMPLE_PATH)/common/common_vo_pattern.c

vpath %.c $(addsuffix :,$(dir $(patsubst %.c,%.o,$(SRCS)))))

CINCLUDE 			:= -I$(SRC_PATH) \
				   -I$(OUT_PATH)/include \
				   -I$(SAMPLE_PATH)/common \
				   -I$(HOME_PATH)/msp/out \
				   -I$(SRC_PATH)/common


OBJS 				:= $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))
DEPS				:= $(OBJS:%.o=%.d)

# exec
TARGET				:= $(MOD_NAME)

# build flags
CFLAGS				:= -Wall -Wl,-rpath-link=$(LIB_PATH)

debug = yes
ifeq ($(debug),yes)
CFLAGS				+= -O0 -ggdb3
else
CFLAGS				+= -O2
endif

# dependency
CLIB				+= -L$(LIB_PATH)
CLIB				+= -lax_vo
CLIB				+= -lax_sys
CLIB				+= -lax_ivps
CLIB				+= -lm -lpthread -lrt -ldl
CLIB                            += -L$(HOME_PATH)/third-party/drm/lib
CLIB                            += -ldrm
#CLIB				+= -laxsyslog

# install
INSTALL_BIN			:= $(TARGET)
INSTALL_ETC			:= $(CONFIG_PATH)/*.ini

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak
