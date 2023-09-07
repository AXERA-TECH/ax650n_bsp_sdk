include color.mk
include config.mak
############################################################################
#	prepare param
############################################################################
HOME_PATH 			:= $(shell pwd)/..

COMPILE_TIME 		 = $(shell date +"%Y%m%d%H%M")
ROOTFS_TARGET_PATH  ?= $(HOME_PATH)/rootfs/rootfs/opt

# Base dir
BL1_PATH			:= $(HOME_PATH)/boot/bl1
UBOOT_PATH			:= $(HOME_PATH)/boot/uboot
ATF_PATH			:= $(HOME_PATH)/boot/atf
ifneq ($(strip $(SUPPORT_OPTEE)),false)
OPTEE_PATH			:= $(HOME_PATH)/boot/optee
SUPPORT_OPTEE			?= TRUE
endif
KERNEL_PATH			:= $(HOME_PATH)/kernel/linux
OSDRV_PATH			:= $(HOME_PATH)/kernel/osdrv
MSP_PATH			:= $(HOME_PATH)/msp/component/isp_proton/sensor \
						$(HOME_PATH)/msp/component/drv/modules/pcie
SAMPLE_PATH			:= $(HOME_PATH)/msp/sample
APP_PATH			:= $(HOME_PATH)/app
BUILD_PATH			:= $(HOME_PATH)/build
TOOLS_PATH			:= $(HOME_PATH)/tools
THIRD_PARTY_PATH		:= $(HOME_PATH)/third-party
ROOTFS_PATH			:= $(HOME_PATH)/rootfs

ifneq ($(SUPPORT_AB_PART), TRUE)
SUPPORT_AB_PART			:= FALSE
endif
ifeq ($(SUPPORT_RAMDISK), TRUE)
ROOTFS_PATH			:= $(HOME_PATH)/rootfs/ramdisk
endif
ifneq ($(strip $(SUPPORT_OPTEE)),false)
SUBDIRS			:= $(OPTEE_PATH)
endif
SUBDIRS			+= $(UBOOT_PATH)  \
				$(ATF_PATH) \
				$(MSP_PATH) \
				$(THIRD_PARTY_PATH) \
				$(ROOTFS_PATH) \
				$(KERNEL_PATH) \
				$(SAMPLE_PATH) \
				$(OSDRV_PATH)/ext_drv

ifeq ($(SUPPORT_bl1), TRUE)
SUBDIRS += $(BL1_PATH)
endif
############################################################################
#	build
############################################################################
SUBDIRS_ALL     = $(addsuffix .all, $(SUBDIRS))
SUBDIRS_CLEAN   = $(addsuffix .clean, $(SUBDIRS))
SUBDIRS_INSTALL = $(addsuffix .install, $(SUBDIRS))

.PHONY: $(SUBDIRS) $(SUBDIRS_ALL) $(SUBDIRS_INSTALL) $(SUBDIRS_CLEAN)

default: $(SUBDIRS)

all: $(SUBDIRS_ALL)
	@echo -e $(GREEN)"\nBuild All Modules success!!\n"  $(DONE)

install: $(SUBDIRS_INSTALL)
	@echo -e $(CYAN)"\nInstall MSP and OSDRV out to rootfs!!\n"  $(DONE)


	@$(MAKE) -C $(ROOTFS_PATH) image
	@echo -e $(GREEN)"\nInstall success!!\n"  $(DONE)

clean:	$(SUBDIRS_CLEAN)
	@rm -rf $(BUILD_PATH)/out/$(PROJECT)
	@echo -e $(GREEN)"\nClean success!!\n"  $(DONE)


axp:
	@bash axp_make.sh $(PROJECT) $(SDK_VERSION) $(SUPPORT_OPTEE) $(SUPPORT_AB_PART)
ifeq ($(ENABLE_SWUPDATE), TRUE)
	@bash make_ota_pkg.sh $(PROJECT) $(SDK_VERSION)
endif
	@echo -e $(GREEN)"\nMake AXP success!!\n"  $(DONE)

$(SUBDIRS):
	@$(ECHO)
	@$(ECHO) -e $(CYAN)"In subdir $@ ..." $(DONE)
	@$(MAKE) -C $(basename $@ )

$(SUBDIRS_ALL):
	@$(ECHO) -e $(CYAN)"Making $(notdir $@)" $(DONE)
	@$(ECHO) -e $(CYAN)"In subdir $(basename $(notdir $@) )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) all

$(SUBDIRS_INSTALL):
	@$(ECHO)
	@$(ECHO) -e $(CYAN)"In subdir $(basename $(notdir $@) )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) install

$(SUBDIRS_CLEAN):
	@$(ECHO) -e $(CYAN)"In subdir $(basename $(notdir $@) )..." $(DONE)
	@$(MAKE) -C $(basename $@ ) clean



