#  -----------------------------------------------------------------------------
#  Desc: TARGET
#  -----------------------------------------------------------------------------
MKFILE_PATH     := $(abspath $(lastword $(MAKEFILE_LIST)))
HOME_PATH       ?= $(abspath $(dir $(MKFILE_PATH))..)
SCRIPTS_PATH    := $(HOME_PATH)/kernel/linux/$(KERNEL_DIR)/scripts/axera
GENERATED_PATH  ?= $(KBUILD_DIR)/generated
KBUILD_OUTDIR   ?= $(HOME_PATH)/build/out/$(PROJECT)/objs/kernel/linux/$(KERNEL_DIR)
EXT_FLAG        ?= O=$(KBUILD_OUTDIR)  HOME_PATH=$(HOME_PATH)

ccflags-y += -DBUILD_AXVERSION=\"$(SDK_VERSION)\"

EXTRA_CFLAGS += -Wno-error=date-time -Wno-date-time
EXTRA_CFLAGS += -D$(CHIP_NAME)

DEBUG_OUT_PATH := $(ROOTFS_TARGET_PATH)/debug_ko
all: modules

.PHONY: modules install clean
.NOTPARALLEL: clean install

modules: $(KBUILD_MAKEFILE)
ifeq ($(koversion),yes)
	@mkdir -p $(GENERATED_PATH)
	@bash $(SCRIPTS_PATH)/axera_module_version.sh $(GENERATED_PATH)/ax_module_version.h $(MODULE_NAME) $(SDK_VERSION)
endif
	@$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) KCFLAGS=$(KCFLAGS) -C $(KDIR) M=$(KBUILD_DIR) src=$(KSRC) $@ $(EXT_FLAG)

$(KBUILD_MAKEFILE): $(KBUILD_DIR)
	@$(TOUCH) $@

$(KBUILD_DIR):
	@$(MKDIR) -p $@

install: modules
ifeq ($(debug),yes)
	@$(CP) $(KBUILD_DIR)/$(MODULE_NAME).ko $(KBUILD_DIR)/$(MODULE_NAME).debug
ifneq ($(DEBUG_OUT_PATH), $(wildcard $(DEBUG_OUT_PATH)))
	$(VERB) $(MKDIR) $(DEBUG_OUT_PATH)
endif
	@$(CP) $(KBUILD_DIR)/$(MODULE_NAME).debug $(DEBUG_OUT_PATH)/$(MODULE_NAME).ko -rf
endif
	$(CROSS)strip --strip-debug $(KBUILD_DIR)/$(MODULE_NAME).ko
ifneq ($(ROOTFS_TARGET_PATH)/ko, $(wildcard $(ROOTFS_TARGET_PATH)/ko))
	$(VERB) $(MKDIR) $(ROOTFS_TARGET_PATH)/ko
endif
	@cp $(KBUILD_DIR)/$(MODULE_NAME).ko $(ROOTFS_TARGET_PATH)/ko/ -rf
	@echo -e "\e[36;1m" "INSTALL  $(KBUILD_DIR)/$(MODULE_NAME).ko to $(ROOTFS_TARGET_PATH)/ko" "\033[0m"

clean:
	@rm -rf $(clean-objs) *.o *~ .depend .*.cmd  *.mod.c .tmp_versions *.ko *.symvers modules.order
	@rm -rf $(ROOTFS_TARGET_PATH)/ko/$(MODULE_NAME).ko
	@rm -rf $(KBUILD_DIR)
ifeq ($(debug),yes)
	@rm -rf $(DEBUG_OUT_PATH)
endif