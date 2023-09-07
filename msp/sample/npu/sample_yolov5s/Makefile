CUR_PATH			:= $(shell pwd)
HOME_PATH			:= $(abspath $(CUR_PATH)/../../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME		:= $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH		:= $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH		:= $(HOME_PATH)/msp/out
OUT_PATH			:= $(MSP_OUT_PATH)
SRC_PATH			:= $(CUR_PATH)
SRC_RELATIVE_PATH	:= $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH		:= $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH	?= $(MSP_OUT_PATH)

BASE_PATH			:= $(HOME_PATH)/msp/component
LIB_PATH			:= $(MSP_OUT_PATH)/lib
COMMON_PATH			:= $(BASE_PATH)/common

INC_PATH			:= $(abspath $(CUR_PATH)/../include)
THIRD_PATH			:= $(HOME_PATH)/third-party

include $(HOME_PATH)/build/config.mak

# output
MOD_NAME			:= sample_npu_yolov5s
OUTPUT				:= $(TARGET_OUT_PATH)/obj

# source
SRCS				:= $(wildcard $(SRC_PATH)/*.cc)
OBJS				:= $(addprefix $(OUTPUT)/,$(notdir $(patsubst %.cc,%.o,$(SRCS))))
CINCLUDE			:= -I$(SRC_PATH) -I$(INC_PATH) -I$(HOME_PATH)/msp/out/include -I$(HOME_PATH)/msp/component/include -I$(THIRD_PATH)/opencv-4.5.5/include/opencv4 -I$(THIRD_PATH)/cmdline

# exec
TARGET				:= $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CPPFLAGS            := --std=c++11 -fopenmp
CFLAGS				:= -fopenmp -Wall -Wl,-rpath-link=$(LIB_PATH)
ifeq ($(debug),yes)
CFLAGS				+= -O0 -ggdb3
else
CFLAGS				+= -O2
endif

# dependency
CLIB				+= -L$(LIB_PATH)
CLIB				+= -L$(THIRD_PATH)/opencv-4.5.5/lib
CLIB				+= -L$(THIRD_PATH)/opencv-4.5.5/lib/opencv4/3rdparty/

CLIB				+= -lstdc++

CLIB				+= -fopenmp

CLIB				+= -lopencv_core -ldl -lm -lpthread -lrt -ltegra_hal -lzlib -littnotify -ltegra_hal
CLIB				+= -lopencv_imgproc -lopencv_core -ldl -lm -lpthread -lrt -ltegra_hal
CLIB				+= -lopencv_imgcodecs -lopencv_core -lopencv_imgproc -ldl -lm -lpthread -lrt -ltegra_hal -llibjpeg-turbo -llibwebp -llibpng -llibtiff -llibopenjp2  -lzlib

CLIB				+= -lax_sys -lax_engine

INSTALL_BIN			:= $(TARGET)
INSTALL_DATA		:= $(CUR_PATH)/data/npu

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak
