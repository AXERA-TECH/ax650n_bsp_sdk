TOPDIR 			:= $(PWD)/../../
BASEDIR			:= $(TOPDIR)src/
INCLUDE_PATH 	:= $(TOPDIR)include
LIBS_PATH		:= $(TOPDIR)libs
KERNEL_DIR		:= linux-5.15.73
UBOOT_DIR               := u-boot-2020.04
ATF_DIR                 := arm-trusted-firmware
OPTEE_DIR               := optee_os-3.5.0

CROSS	:= aarch64-none-linux-gnu-
CC	= $(CROSS)gcc
CPP	= $(CROSS)g++
LD	= $(CROSS)ld
AR	= $(CROSS)ar -rcs
OBJCPOY	= $(CROSS)objcopy
STRIP	= $(CROSS)strip

VERB	= @
RM	= rm -f
MKDIR	= mkdir -p
ECHO	= echo
MV	= mv
LN	= ln -sf
CP	= cp -f
TAR	= tar
TOUCH	= touch
ARCH	= arm64
STATIC_FLAG  := -fPIC
DYNAMIC_FLAG := -shared -fPIC

include $(shell dirname $(lastword $(MAKEFILE_LIST)))/project.mak
include $(shell dirname $(lastword $(MAKEFILE_LIST)))/version.mak

-include $(shell dirname $(lastword $(MAKEFILE_LIST)))/projects/$(PROJECT).mak
export SDK_VERSION
export OS_MEM
