SUBDIRS = dma/ep dma/rc msg/ep msg/rc boot icc_trans/ep icc_trans/rc

################################################################################
#	prepare param
################################################################################
LOCAL_PATH		:= $(shell pwd)
HOME_PATH		:= $(abspath $(LOCAL_PATH)/../../..)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

PRJ_OUT_HOME		:= $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH		:= $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH		:= $(HOME_PATH)/msp/out
ROOTFS_TARGET_PATH 	:= $(MSP_OUT_PATH)

################################################################################
#	set task
################################################################################
SUBDIRS_CLEAN   = $(addsuffix .clean, $(SUBDIRS))
SUBDIRS_INSTALL = $(addsuffix .install, $(SUBDIRS))

.PHONY: $(SUBDIRS) $(SUBDIRS_INSTALL) $(SUBDIRS_CLEAN)
.NOTPARALLEL: clean all

all: $(SUBDIRS)
	@$(ECHO) -e $(GREEN)"\nBuild All sample Modules success!!\n"  $(DONE)

install: $(SUBDIRS_INSTALL)
	@$(ECHO) -e $(GREEN)"\nInstall sample success!!\n"  $(DONE)

clean:	$(SUBDIRS_CLEAN)
	@$(ECHO) -e $(GREEN)"\nClean sample success!!\n"  $(DONE)

$(SUBDIRS):
	@$(ECHO)
	@$(ECHO) -e $(CYAN)"In subdir $@ ..." $(DONE)
	@$(MAKE) -C $(basename $@ )

$(SUBDIRS_INSTALL):
	@$(ECHO)
	@$(ECHO) -e $(CYAN)"In subdir $(basename $@ )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) install

$(SUBDIRS_CLEAN):
	@$(ECHO) -e $(CYAN)"In subdir $(basename $@ )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) clean


# Export val
export ROOTFS_TARGET_PATH
