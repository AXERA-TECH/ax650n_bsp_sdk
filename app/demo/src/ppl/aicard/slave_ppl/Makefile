CUR_PATH		    := $(shell pwd)
HOME_PATH		    := $(abspath $(CUR_PATH)/../../../../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME        := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH        := $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH        := $(HOME_PATH)/msp/out
SRC_PATH            := $(CUR_PATH)
LIB_PATH            := $(MSP_OUT_PATH)/lib
INC_PATH            := $(MSP_OUT_PATH)/include
SRC_RELATIVE_PATH   := $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH     := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH  := $(MSP_OUT_PATH)
DEMO_SRC_PATH       := $(HOME_PATH)/app/demo/src
DEMO_BIN_PATH       := $(HOME_PATH)/app/demo/bin/aicard/slave
THIRD_PARTY_PATH    := $(HOME_PATH)/third-party

# output
MOD_NAME            := aicard_slave
OUTPUT              := $(TARGET_OUT_PATH)/obj

# source
SRCCPPS             := $(wildcard $(SRC_PATH)/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/vdec/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/log/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/pcie/*.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/pool/AXPoolManager.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/utils/ElapsedTimer.cpp) \
                       $(wildcard $(DEMO_SRC_PATH)/config/ini/*.cpp)


CINCLUDE            := -I$(SRC_PATH) \
                       -I$(INC_PATH) \
                       -I$(DEMO_SRC_PATH)/header \
					   -I$(DEMO_SRC_PATH)/vdec \
                       -I$(DEMO_SRC_PATH)/vdec/stream \
                       -I$(DEMO_SRC_PATH)/option_helper \
                       -I$(DEMO_SRC_PATH)/utils \
                       -I$(DEMO_SRC_PATH)/log \
                       -I$(DEMO_SRC_PATH)/pcie \
                       -I$(DEMO_SRC_PATH)/pool \
                       -I$(DEMO_SRC_PATH)/config/ini \
                       -I$(THIRD_PARTY_PATH)/openssl/include


 OBJS               := $(SRCS:%.c=$(OUTPUT)/%.o)
 CPPOBJS            := $(SRCCPPS:%.cpp=$(OUTPUT)/%.o)

 DEPS               := $(OBJS:%.o=%.d)
 CPPDEPS            := $(CPPOBJS:%.o=%.d)

# exec
TARGET              := $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CPPFLAGS            := --std=c++11
CFLAGS              := $(CPPFLAGS) -Wall -Wl,-rpath-link=$(LIB_PATH):-L$(THIRD_PARTY_PATH)/openssl/lib
CFLAGS              += -DAPP_BUILD_VERSION=\"$(SDK_VERSION)\"

ifeq ($(debug),yes)
CFLAGS              += -O0 -ggdb3
else
CFLAGS              += -O2
endif

# dependency
CLIB                := -lstdc++ -pthread -lm -lrt
CLIB                += -L$(LIB_PATH)
CLIB                += -lax_sys
CLIB                += -lax_vdec
CLIB                += -lax_pcie_dma
CLIB                += -lax_pcie_msg
CLIB                += -lax_engine
CLIB                += -lax_skel
CLIB                += -L$(THIRD_PARTY_PATH)/drm/lib -ldrm
CLIB                += -L$(THIRD_PARTY_PATH)/openssl/lib -lssl -lcrypto

# install
INSTALL_TARGET      := $(TARGET)
INSTALL_TARGET      += $(DEMO_BIN_PATH)/*
INSTALL_DIR         := $(ROOTFS_TARGET_PATH)/bin/AiCard/slave

# link
LINK = $(CC)

include $(HOME_PATH)/build/rules.mak

