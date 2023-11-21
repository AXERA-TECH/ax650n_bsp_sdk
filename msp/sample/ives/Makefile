CUR_PATH			:= $(shell pwd)
HOME_PATH		:= $(abspath $(CUR_PATH)/../../..)
include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak
PRJ_OUT_HOME		:= $(HOME_PATH)/build/out/$(PROJECT)
OBJ_OUT_PATH		:= $(PRJ_OUT_HOME)/objs
MSP_OUT_PATH		:= $(OBJ_OUT_PATH)/msp/out
OUT_PATH		:= $(MSP_OUT_PATH)
SRC_PATH			:= $(CUR_PATH)
BASE_PATH			:= $(HOME_PATH)/msp/component
LIB_PATH			:= $(MSP_OUT_PATH)/lib
THIRD_PARTY_PATH  	:= $(HOME_PATH)/third-party
ROOTFS_TARGET_PATH 	:= $(MSP_OUT_PATH)
include $(HOME_PATH)/build/config.mak

################################################################################
# build
################################################################################

.PHONY : all clean install lib
.NOTPARALLEL: clean all

all: lib
	@echo -e $(GREEN)"\nBuild $@ success!!\n"  $(DONE)

clean:
	$(VERB) $(MAKE) -f Makefile.dynamic clean
	$(VERB) $(MAKE) -f Makefile.static clean
	@echo -e $(GREEN)"\nClean $@ success!!\n"  $(DONE)

install:
	@echo -e $(GREEN)"Install $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic install
	$(VERB) $(MAKE) -f Makefile.static install

lib:
	@echo -e $(GREEN)"Build dynamic $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic

	@echo -e $(GREEN)"Build static $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.static

export ROOTFS_TARGET_PATH
