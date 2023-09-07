CUR_PATH		:= $(shell pwd)
SRC_PATH		:= $(CUR_PATH)
HOME_PATH		:= $(abspath $(CUR_PATH)/../../../../..)

include $(HOME_PATH)/build/config.mak

OUT_DIR			:= $(HOME_PATH)/build/out/$(PROJECT)/objs
OUT_PATH		:= $(HOME_PATH)/msp/out
ROOTFS_TARGET_PATH	:= $(OUT_PATH)

# output
MOD_NAME 		:=
OUTPUT 			:= .obj

# source
SRCS 			:=
CINCLUDE		:=	-I$(OUT_PATH)/include

OBJS 			:= $(SRCS:%.c=$(OUTPUT)/%.o)
DEPS			:= $(OBJS:%.o=%.d)

# build flags
CFLAGS			:= -shared -fPIC
ifeq ($(debug),yes)
CFLAGS			+= -Werror -Wall -O0 -g
else
CFLAGS			+= -Werror -Wall -O2
endif

# dependency

# install
INSTALL_INC		+= $(SRC_PATH)/ax_sensor_struct.h
# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak
