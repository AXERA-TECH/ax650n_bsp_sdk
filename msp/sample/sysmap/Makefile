CUR_PATH		:= $(shell pwd)
HOME_PATH		:= $(CUR_PATH)/../../..
ROOTFS_TARGET_PATH	?= $(OUT_PATH)

include $(HOME_PATH)/build/color.mk
include $(HOME_PATH)/build/config.mak

################################################################################
# build
################################################################################

.PHONY : all clean install bin
.NOTPARALLEL: clean all

all: bin
	@echo -e $(GREEN)"\nBuild $@ success!!\n"  $(DONE)

clean:
	$(VERB) $(MAKE) -f Makefile.dynamic clean
	@echo -e $(GREEN)"\nClean $@ success!!\n"  $(DONE)

install:
	@echo -e $(GREEN)"install $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic install

bin:
	@echo -e $(GREEN)"build dynamic $@ ..."  $(DONE)
	$(VERB) $(MAKE) -f Makefile.dynamic
