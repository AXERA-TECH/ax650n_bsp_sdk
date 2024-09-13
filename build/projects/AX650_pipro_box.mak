BUILD_BUSYBOX := FALSE
SUPPORT_UBIFS := FALSE

ROOTFS_PART_SIZE := 24576M
SOC_PART_SIZE := 512M
OPT_PART_SIZE := 4096M

#SUPPORT_OPTEE := false
ifneq ($(strip $(SUPPORT_OPTEE)),false)
OPTEE_IMAGE_ADDR  := 0x134000000
OPTEE_RESERVED_SIZE  := 0x2000000
OPTEE_SHMEM_SIZE := 0x200000
endif
#SECURE_ENABLE := true

# linux OS memory config
OS_MEM         := mem=4096M
# cmm memory config
CMM_POOL_PARAM := anonymous,0,0x200000000,4096M
# dsp mempory config

# if build with asan=yes or debugkconfig=yes, change OS memory to 3GB
ifneq ($(findstring yes, $(asan) $(debugkconfig)),)
OS_MEM         := mem=3072M
CMM_POOL_PARAM := anonymous,0,0x1C0000000,4096M
endif

# if you want use ubuntu rootfs, uncomment the statement below
# use_ubuntu_rootfs := yes
