
CUR_PATH		:= $(shell pwd)
HOME_PATH		:= $(CUR_PATH)/../../..

SRC_PATH		:= $(CUR_PATH)
SAMPLE_PATH		:= $(CUR_PATH)/..

SDK_BASE_PATH		:= $(HOME_PATH)/msp/component

OUT_PATH		:= $(HOME_PATH)/msp/out

LIB_PATH			:= $(HOME_PATH)/msp/out/lib

SAMPLE_COMMON_PATH			:= $(SAMPLE_PATH)/common
SAMPLE_VENC_COMMON_PATH			:= $(CUR_PATH)/common
SAMPLE_FEATURE_TEST_PATH		:= $(CUR_PATH)/featureTest


ROOTFS_TARGET_PATH	?= $(OUT_PATH)

include $(HOME_PATH)/build/config.mak

# output
MOD_NAME 		:= sample_venc
OUTPUT 			:= .obj

# source
SRCS 			:= $(wildcard $(SAMPLE_VENC_COMMON_PATH)/*.c) \
			   $(wildcard $(SRC_PATH)/*.c) \
			   $(wildcard $(SAMPLE_FEATURE_TEST_PATH)/*.c)
SRCS			+= $(SAMPLE_COMMON_PATH)/common_venc.c
SRCS			+= $(SAMPLE_COMMON_PATH)/common_venc_log.c

vpath %.c $(addsuffix :,$(dir $(patsubst %.c,%.o,$(SRCS)))))

CINCLUDE		+= -I$(SRC_PATH) \
			   -I$(OUT_PATH)/include \
			   -I$(SAMPLE_PATH) \
			   -I$(SAMPLE_VENC_COMMON_PATH) \
			   -I$(SAMPLE_FEATURE_TEST_PATH) \
			   -I$(SAMPLE_COMMON_PATH)

OBJS 			:= $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))
DEPS			:= $(OBJS:%.o=%.d)
DEPS			+= $(shell awk '{gsub(/```/,"");print}' help.md | xxd -i > help.dat)
# exec
TARGET			:= $(MOD_NAME)

# build flags
CFLAGS			:= -Wall -Wl,-rpath-link=$(LIB_PATH)

ifeq ($(debug),yes)
CFLAGS			+= -O0 -ggdb3
else
CFLAGS			+= -O2
endif

CLIB			:= -L$(LIB_PATH) -lax_venc \
				-lax_sys \
				-lm -lpthread \
				#-lax_syslog

CFLAGS			+= -Werror

# install
INSTALL_BIN	:= $(TARGET)
#INSTALL_DATA	:= $(CUR_PATH)/data/venc

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak
