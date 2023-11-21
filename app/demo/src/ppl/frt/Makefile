CUR_PATH			:= $(shell pwd)
HOME_PATH			:= $(abspath $(CUR_PATH)/../../../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME 		:= $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH     	:= $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH 		:= $(HOME_PATH)/msp/out
LIB_PATH            := $(MSP_OUT_PATH)/lib
INC_PATH            := $(MSP_OUT_PATH)/include
SRC_RELATIVE_PATH   := $(subst $(HOME_PATH)/,,$(CUR_PATH))
TARGET_OUT_PATH     := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH 	:= $(MSP_OUT_PATH)
SRC_PATH            := $(HOME_PATH)/app/demo/src
FRT_PATH	    	:= $(CUR_PATH)
BIN_PATH            := $(HOME_PATH)/app/demo/bin/frtdemo
3RD_PATH        	:= $(SRC_PATH)/3rd
SSL_PATH        	:= $(HOME_PATH)/third-party/openssl
MP4_PATH            := $(HOME_PATH)/third-party/mp4
FONT_USE_FREETYPE   := no


MOD_NAME 			 = frtdemo
OUTPUT 				:= $(TARGET_OUT_PATH)/obj

# source
SRCCPPS			+=	$(wildcard $(FRT_PATH)/*.cpp) \
					$(wildcard $(FRT_PATH)/cmdline_parser/*.cpp) \
					$(wildcard $(FRT_PATH)/ipc_ppl/*.cpp) \
					$(wildcard $(FRT_PATH)/ipc_ppl/test/*.cpp) \
					$(wildcard $(FRT_PATH)/its_ppl/*.cpp) \
					$(wildcard $(FRT_PATH)/its_ppl/test/*.cpp) \
					$(wildcard $(FRT_PATH)/its_ppl/detector/*.cpp) \
					$(wildcard $(SRC_PATH)/config_parser/*.cpp) \
					$(wildcard $(SRC_PATH)/log/*.cpp) \
					$(wildcard $(SRC_PATH)/pool/*.cpp) \
					$(wildcard $(SRC_PATH)/rtsp/*.cpp) \
					$(wildcard $(SRC_PATH)/sensor/*.cpp) \
					$(wildcard $(SRC_PATH)/stage/*.cpp) \
					$(wildcard $(SRC_PATH)/ivps/*.cpp) \
					$(wildcard $(SRC_PATH)/encoder/*.cpp) \
					$(wildcard $(SRC_PATH)/linkage/*.cpp) \
					$(wildcard $(SRC_PATH)/webserver/*.cpp) \
					$(wildcard $(SRC_PATH)/detector/*.cpp) \
					$(wildcard $(SRC_PATH)/ives/*.cpp) \
					$(wildcard $(SRC_PATH)/config/ini/*.cpp) \
					$(wildcard $(SRC_PATH)/option_helper/*.cpp) \
					$(wildcard $(SRC_PATH)/utils/*.cpp) \
					$(wildcard $(SRC_PATH)/utils/yuv/*.cpp) \
					$(wildcard $(SRC_PATH)/subfunc/eocw/*.cpp) \
					$(wildcard $(SRC_PATH)/ivps/osd/*.cpp) \
					$(wildcard $(SRC_PATH)/mp4enc/*.cpp) \
					$(wildcard $(SRC_PATH)/capture/*.cpp) \
					$(wildcard $(SRC_PATH)/dsp/*.cpp) \
					$(wildcard $(FRT_PATH)/pano_ppl/*.cpp) \
					$(wildcard $(FRT_PATH)/pano_ppl/test/*.cpp) \
					$(wildcard $(SRC_PATH)/audio/*.cpp) \
					$(wildcard $(SRC_PATH)/avs/*.cpp)

OBJS			:= $(SRCCPPS:%.cpp=$(OUTPUT)/%.o)

DEPS			:= $(OBJS:%.o=$(OUTPUT)/%.d)

# include
CINCLUDE 		:= 	-I$(FRT_PATH) \
					-I$(FRT_PATH)/cmdline_parser \
					-I$(FRT_PATH)/its_ppl \
					-I$(FRT_PATH)/ipc_ppl \
					-I$(FRT_PATH)/pano_ppl \
					-I$(SRC_PATH)/header \
					-I$(SRC_PATH)/config_parser \
					-I$(SRC_PATH)/log \
					-I$(SRC_PATH)/pool \
					-I$(SRC_PATH)/rtsp \
					-I$(SRC_PATH)/sensor \
					-I$(SRC_PATH)/stage \
					-I$(SRC_PATH)/ivps \
					-I$(SRC_PATH)/detector \
					-I$(SRC_PATH)/ives \
					-I$(SRC_PATH)/encoder \
					-I$(SRC_PATH)/linkage \
					-I$(SRC_PATH)/webserver \
					-I$(SRC_PATH)/config/ini \
					-I$(SRC_PATH)/config/json \
					-I$(SRC_PATH)/option_helper \
					-I$(SRC_PATH)/utils \
					-I$(SRC_PATH)/subfunc/eocw \
					-I$(SRC_PATH)/utils/yuv \
					-I$(SRC_PATH)/avs \
					-I$(3RD_PATH)/inc/appweb \
					-I$(3RD_PATH)/inc \
					-I$(3RD_PATH)/inc/live/BasicUsageEnvironment \
					-I$(3RD_PATH)/inc/live/groupsock \
					-I$(3RD_PATH)/inc/live/liveMedia \
					-I$(3RD_PATH)/inc/live/UsageEnvironment \
					-I$(3RD_PATH)/inc/live/UsageEnvironment \
					-I$(SSL_PATH)/include \
					-I$(MP4_PATH)/include \
					-I$(MSP_OUT_PATH)/include \
					-I$(SRC_PATH)/ivps/osd \
					-I$(SRC_PATH)/mp4enc \
					-I$(SRC_PATH)/capture \
					-I$(SRC_PATH)/dsp \
					-I$(SRC_PATH)/audio

ifeq ($(FONT_USE_FREETYPE),yes)
CINCLUDE 		+= 	-I$(3RD_PATH)/inc/freetype2 \
					-I$(3RD_PATH)/inc/freetype2/freetype \
					-I$(3RD_PATH)/inc/freetype2/freetype/config
endif


# exec
TARGET			:= $(TARGET_OUT_PATH)/$(MOD_NAME)

# build flags
CFLAGS			:= --std=c++11 -Wall -Wl,-rpath-link=$(LIB_PATH):$(BIN_PATH)/lib:$(3RD_PATH)/lib:$(MP4_PATH)/lib
debug = no
ifeq ($(debug),yes)
CFLAGS			+= -O0 -ggdb3
else
CFLAGS			+= -O2
endif

ifeq ($(mtrace),yes)
CFLAGS			+= -DAX_MEM_CHECK
endif

ifeq ($(FONT_USE_FREETYPE),yes)
CFLAGS          += -DFONT_USE_FREETYPE
endif

# CFLAGS			+= -DTUNING_CTRL
CFLAGS          += -DAPP_BUILD_VERSION=\"$(SDK_VERSION)\"

# dependency
CLIB			+= -L$(LIB_PATH)

CLIB			+= -lax_sys
CLIB			+= -lax_proton
CLIB			+= -lax_mipi
CLIB			+= -lax_venc
CLIB			+= -lax_ivps
CLIB			+= -lax_3a
CLIB			+= -lax_engine
CLIB			+= -lrt
CLIB			+= -lm
CLIB			+= -ldl
CLIB			+= -lpthread
CLIB			+= -L$(BIN_PATH)/lib -L$(3RD_PATH)/lib -L$(SSL_PATH)/lib -lssl -lcrypto -lappweb -lmpr -lhttp -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment
CLIB			+= -lstdc++
CLIB			+= -L$(MP4_PATH)/lib -l:libmp4.a
CLIB			+= -lax_nt_stream
CLIB			+= -lax_nt_ctrl
CLIB			+= -lax_skel
CLIB			+= -lax_ives
CLIB			+= -L$(HOME_PATH)/third-party/tinyalsa/lib
CLIB			+= -ltinyalsa
CLIB			+= -L$(HOME_PATH)/third-party/fdk-aac/lib
CLIB			+= -lfdk-aac
CLIB			+= -L$(HOME_PATH)/third-party/libsamplerate/lib
CLIB			+= -lsamplerate
CLIB			+= -L$(HOME_PATH)/third-party/opus/lib
CLIB			+= -lopus
CLIB			+= -lax_audio
CLIB			+= -lax_audio_3a
CLIB			+= -lax_dsp -lpthread -lax_dsp_cv
CLIB			+= -lax_avs -lax_avscali
CLIB            += -lax_dmadim

ifeq ($(FONT_USE_FREETYPE),yes)
CLIB			+= -L$(3RD_PATH)/lib -l:libfreetype.a
endif



INSTALL_TARGET  := $(TARGET)
INSTALL_TARGET  += $(BIN_PATH)/*
INSTALL_DIR		:= $(ROOTFS_TARGET_PATH)/bin/FRTDemo

# link
LINK = $(CPP)

include $(HOME_PATH)/build/rules.mak
