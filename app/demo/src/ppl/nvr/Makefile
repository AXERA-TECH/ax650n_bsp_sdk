CUR_PATH		    := $(shell pwd)
HOME_PATH		    := $(abspath $(CUR_PATH)/../../../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME        := $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH        := $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH        := $(HOME_PATH)/msp/out
SRC_PATH            := $(CUR_PATH)
SRC_RELATIVE_PATH   := $(subst $(HOME_PATH)/,,$(SRC_PATH))
TARGET_OUT_PATH     := $(OBJ_OUT_PATH)/$(SRC_RELATIVE_PATH)
ROOTFS_TARGET_PATH  := $(MSP_OUT_PATH)
DEMO_BIN_PATH       := $(HOME_PATH)/app/demo/bin/nvrdemo
QT_PATH             := $(HOME_PATH)/third-party/qt-5.15.7/bin


################################################################################
# build
################################################################################

.PHONY : all clean install
all: install
	@echo -e $(GREEN)"\nBuild $@ success!!\n"  $(DONE)

install: default
	@echo -e $(GREEN)"\nInstall $@ success!!\n"  $(DONE)

clean:
ifneq ($(wildcard Makefile.qt),)
	$(VERB) $(MAKE) -f Makefile.qt clean
	$(VERB) $(RM) -rf $(CUR_PATH)/Makefile.qt
endif
	$(VERB) $(RM) -rf $(ROOTFS_TARGET_PATH)/bin/NVRDemo
	$(VERB) $(RM) -rf $(CUR_PATH)/build
	$(VERB) $(RM) -rf $(CUR_PATH)/.qmake.stash

default:
	@echo -e $(GREEN)"Build $@ ..."  $(DONE)
	$(VERB) $(MKDIR) $(ROOTFS_TARGET_PATH)/bin/NVRDemo
	$(VERB) $(CP) -rf $(DEMO_BIN_PATH)/* $(ROOTFS_TARGET_PATH)/bin/NVRDemo/
ifeq ($(debug), yes)
#   jira9258:
#        The reason why left button event is missed in eventFilter of debug compile is still unknown.
#        Considing stability, disable debug compile as a WR.
#        Debug by gdb, please restore options: CONFIG+=debug QMAKE_CFLAGS+="-O0 -ggdb3"
#	$(VERB) $(QT_PATH)/qmake -o Makefile.qt $(SRC_PATH)/nvrdemo.pro CONFIG+=debug QMAKE_CFLAGS+="-O0 -ggdb3"
#   $(VERB) $(MAKE) -f Makefile.qt
#	$(VERB) $(CP) $(CUR_PATH)/build/debug/nvrdemo $(ROOTFS_TARGET_PATH)/bin/NVRDemo/
	$(VERB) $(QT_PATH)/qmake -o Makefile.qt $(SRC_PATH)/nvrdemo.pro CONFIG+=release
	$(VERB) $(MAKE) -f Makefile.qt
	$(VERB) $(CP) $(CUR_PATH)/build/release/nvrdemo $(ROOTFS_TARGET_PATH)/bin/NVRDemo/
else
	$(VERB) $(QT_PATH)/qmake -o Makefile.qt $(SRC_PATH)/nvrdemo.pro CONFIG+=release
	$(VERB) $(MAKE) -f Makefile.qt
	$(VERB) $(CP) $(CUR_PATH)/build/release/nvrdemo $(ROOTFS_TARGET_PATH)/bin/NVRDemo/
endif


export ROOTFS_TARGET_PATH
