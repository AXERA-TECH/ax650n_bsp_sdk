################################################################################
#	prepare param
################################################################################
CUR_PATH			:= $(shell pwd)
HOME_PATH			:= $(abspath $(CUR_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME        := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH        := $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH        := $(HOME_PATH)/msp/out
SRC_PATH            := $(CUR_PATH)
LIB_PATH            := $(MSP_OUT_PATH)/lib
SRC_RELATIVE_PATH   := $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH     := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH  := $(MSP_OUT_PATH)

################################################################################
#	set task
################################################################################
MOD_NAME 			= sample_ivps
OUTPUT 				:= $(TARGET_OUT_PATH)/obj

# source
SRCS 				:= $(wildcard $(SRC_PATH)/*.c) \
                                   $(wildcard $(SRC_PATH)/tools/*.c) \
                                   $(wildcard $(SRC_PATH)/tools/ini_parser/*.c) \
                                   $(wildcard $(SRC_PATH)/sync_api/*.c) \
                                   $(wildcard $(SRC_PATH)/../common/common_venc.c) \
				   $(wildcard $(SRC_PATH)/../common/common_venc_log.c) \
                                   $(wildcard $(SRC_PATH)/ivps_venc/*.c)

vpath %.c $(addsuffix :,$(dir $(patsubst %.c,%.o,$(SRCS))))

CINCLUDE 			:= -I$(SRC_PATH) \
                                   -I$(SRC_PATH)/tools \
                                   -I$(SRC_PATH)/tools/ini_parser \
                                   -I$(MSP_OUT_PATH)/include \
                                   -I$(SRC_PATH)/../common/include \
				   -I$(SRC_PATH)/../common/ \
				   -I$(SRC_PATH)/ivps_venc \
                                   -I$(SRC_PATH)/sync_api \
                                   -I$(SRC_PATH)/data \
                                   -I$(HOME_PATH)/msp/out


OBJS 				:= $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))
DEPS				:= $(OBJS:%.o=%.d)

# exec
TARGET				:= $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CFLAGS				:= -Werror -Wall -Wl,-rpath-link=$(LIB_PATH)

debug = no
ifeq ($(debug),yes)
CFLAGS				+= -O0 -ggdb3 -Wno-unused
else
CFLAGS				+= -O2 -Wno-unused
endif

# dependency
CLIB				+= -L$(LIB_PATH)
CLIB				+= -lax_ivps
CLIB				+= -lax_venc
CLIB				+= -lax_sys
CLIB				+= -lm -lpthread -lrt -ldl
# CLIB				+= -lax_syslog
# CLIB				+= -lax_venc
# install
INSTALL_TARGET			:= $(TARGET)
INSTALL_DIR			:= $(ROOTFS_TARGET_PATH)/bin/
INSTALL_DATA    		:= $(CUR_PATH)/data/ivps
INSTALL_DATA    		+= $(CUR_PATH)/config
# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak
