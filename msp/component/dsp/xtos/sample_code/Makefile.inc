export DSP_XTENSA_COMPILE?=/usr/xtensa/XtDevTools/install/tools/RI-2021.7-linux/XtensaTools/bin
export DSP_XTENSA_INSTALL?=/usr/xtensa/XtDevTools/install
export XTENSA_CORE=mc50_vdsp_q7
export TARGET=MC50_VDSP

# Include platform-specific commands
CONFIGDIR = $(DSP_XTENSA_INSTALL)/builds/RI-2021.7-linux/mc50_vdsp_q7

# Detect C library
XSTR2   = $(shell $(GREP) SW_CLibrary $(CONFIGDIR)$(S)config$(S)default-params)
CLIB    = $(word 3, $(XSTR2))

ifeq ($(CLIB), xclib)
CSTD    = -clang -std=c11
CPPSTD  = -clang -std=c++11
else
CSTD    = -std=c99
CPPSTD  = -std=c++98
endif

ROOTDIR = ../..
SRCDIR  = $(ROOTDIR)/src
LIBDIR  = $(ROOTDIR)/lib
INCDIR  = $(ROOTDIR)/include
CC      = $(DSP_XTENSA_COMPILE)/xt-clang $(CSTD)
DUMPELF = $(DSP_XTENSA_COMPILE)/xt-dumpelf
CPP     = $(DSP_XTENSA_COMPILE)/xt-xc++ $(CPPSTD)
MODE   ?= DBG
TARGET ?= SIM


# Common for all modes
CFLAGS  = -Wall -mlongcalls -ffunction-sections -I$(INCDIR)
ASFLAGS = -Wall -mlongcalls -ffunction-sections -Wa,--schedule -I$(INCDIR)
LDFLAGS = -Wl,--gc-sections

ifeq ($(MODE), OPT)
CFLAGS  += -O2 -ipa $(CFLAGS_EXTRA)
ASFLAGS += -O2 -ipa
endif

ifeq ($(MODE), DEV)
CFLAGS  += -O2 -Os -g  $(CFLAGS_EXTRA)
ASFLAGS += -O2 -Os -g
endif

ifeq ($(MODE), DBG)
CFLAGS  += -O2 -Os -g $(CFLAGS_EXTRA)
ASFLAGS += -O2 -Os -g
endif

ifeq ($(MODE), SIZE)
CFLAGS  += -O2 -Os -g $(OFLAGS) $(CFLAGS_EXTRA)
ASFLAGS += -O2 -Os -g $(OFLAGS)
endif

ifeq ($(MODE), FAST)
CFLAGS  += -O3 -ipa $(OFLAGS) $(CFLAGS_EXTRA)
ASFLAGS += -O3 -ipa $(OFLAGS)
endif

ifeq ($(TARGET), SIM)
LSP = sim
endif

ifeq ($(TARGET), MC40_VDSP)
CFLAGS += -DBOARD=1
LSP     = $(ROOTDIR)/lsp/ax630x-lsp
endif

ifeq ($(TARGET), MC50_VDSP)
CFLAGS += -DBOARD=1 -DXI_XV_TILE_COMPATIBILITY
LSP     = $(ROOTDIR)/lsp/ax650x-lsp
endif


