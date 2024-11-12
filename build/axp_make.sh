#!/bin/sh

set -e

echo "make axp ..."


while getopts "a:o:p:u:v:" arg
do
	case "$arg" in
		o)
			echo "support optee: $OPTARG"
			SUPPORT_OPTEE=$OPTARG
			;;
		p)
			echo "project: $OPTARG"
			PROJECT=$OPTARG
			;;
		u)
			echo "build ubuntu axp: $OPTARG"
			BUILD_UBUNTU_AXP=$OPTARG
			;;
		v)
			echo "sdk version: $OPTARG"
			VERSION=$OPTARG
			;;
		a)
			echo "enable ab part: $OPTARG"
			SUPPORT_AB_PART=$OPTARG
			;;
	esac
done

CHIP_NAME=${PROJECT%\_*}
CHIP_GROUP=${CHIP_NAME:0:5}X
VERSION_EXT=${VERSION}_$(date "+%Y%m%d%H%M%S")

AXP_NAME=${PROJECT}_${VERSION_EXT}.axp

LOCAL_PATH=$(pwd)
HOME_PATH=$LOCAL_PATH/..
OUTPUT_PATH=$LOCAL_PATH/out
IMG_PATH=$OUTPUT_PATH/$PROJECT/images
AXP_PATH=$OUTPUT_PATH/$AXP_NAME
UBUNTU_AXP_PATH=${OUTPUT_PATH}/${PROJECT}_ubuntu_rootfs_${VERSION_EXT}.axp
UBUNTU_DESKTOP_AXP_PATH=${OUTPUT_PATH}/${PROJECT}_ubuntu_rootfs_desktop_${VERSION_EXT}.axp
cp $HOME_PATH/tools/imgsign/eip.bin $IMG_PATH/eip.bin

TOOL_PATH=$HOME_PATH/tools/mkaxp/make_axp.py
if [ "$PROJECT" = "AX650_emmc" ] || [ "$PROJECT" = "AX650_xpilot_6v" ]; then
if [ "$SUPPORT_OPTEE" = "false" ]; then
XML_PATH=$HOME_PATH/tools/mkaxp/$CHIP_GROUP.xml
optee="false"
else
XML_PATH=$HOME_PATH/tools/mkaxp/${CHIP_GROUP}_optee.xml
optee="true"
fi
elif [ "$PROJECT" = "AX650_slave" ]; then
XML_PATH=$HOME_PATH/tools/mkaxp/${CHIP_GROUP}_SLAVE.xml
elif [ "$PROJECT" = "AX650_ssd" ]; then
if [ "$SUPPORT_OPTEE" = "false" ]; then
XML_PATH=$HOME_PATH/tools/mkaxp/${CHIP_GROUP}_ssd.xml
optee="false"
else
XML_PATH=$HOME_PATH/tools/mkaxp/${CHIP_GROUP}_ssd_optee.xml
optee="true"
fi
elif [ "$PROJECT" = "AX650_pipro_box" ]; then
if [ "$SUPPORT_OPTEE" = "false" ]; then
XML_PATH=$HOME_PATH/tools/mkaxp/${CHIP_GROUP}_pipro.xml
optee="false"
else
XML_PATH=$HOME_PATH/tools/mkaxp/${CHIP_GROUP}_pipro_optee.xml
optee="true"
fi
elif [ "$PROJECT" = "AX650_master" ]; then
if [ "$SUPPORT_OPTEE" = "false" ]; then
XML_PATH=$HOME_PATH/tools/mkaxp/${CHIP_GROUP}_master.xml
optee="false"
else
XML_PATH=$HOME_PATH/tools/mkaxp/${CHIP_GROUP}_master_optee.xml
optee="true"
fi
else
XML_PATH=$HOME_PATH/tools/mkaxp/$CHIP_GROUP.xml
fi

if [ "$SUPPORT_AB_PART" = "TRUE" ]; then
	XML_PATH=${XML_PATH%.*}_AB.xml
fi

# rm -f $OUTPUT_PATH/${PROJECT}_${VERSION}*.axp

if [ "$CHIP_GROUP" = "AX650X" ]; then
if [ "$PROJECT" = "AX650_emmc" ] || [ "$PROJECT" = "AX650_pipro_box" ] || [ "$PROJECT" = "AX650_ssd" ] || [ "$PROJECT" = "AX650_master" ] || [ "$PROJECT" = "AX650_xpilot_6v" ]; then
	DTB_PATH=$IMG_PATH/${PROJECT}_signed.dtb
	EIP_PATH=$IMG_PATH/eip.bin
	FDL2_PATH=$IMG_PATH/fdl2_signed.bin
	UBOOT_PATH=$IMG_PATH/u-boot_signed.bin
	UBOOTBK_PATH=$IMG_PATH/uboot_bk.bin
	KERNEL_PATH=$IMG_PATH/Image
	ROOTFS_PATH=$IMG_PATH/rootfs_sparse.ext4
	UBUNTU_ROOTFS_PATH=$IMG_PATH/ubuntu_rootfs_sparse.ext4
	UBUNTU_ROOTFS_DESKTOP_PATH=$IMG_PATH/ubuntu_rootfs_desktop_sparse.ext4
	PARAM_PATH=$IMG_PATH/param_sparse.ext4
	SOC_PATH=$IMG_PATH/soc_sparse.ext4
	OPT_PATH=$IMG_PATH/opt_sparse.ext4
	OPT_UBUNTU_ROOTFS_PATH=$IMG_PATH/opt_ubuntu_rootfs_sparse.ext4
	BOOT_PATH=$IMG_PATH/boot_signed.bin
	ATF_PATH=$IMG_PATH/atf_bl31_signed.bin
	LOGO_PATH=$IMG_PATH/axera_logo.bmp
	if [ ! "$SUPPORT_OPTEE" = "false" ]; then
	OPTEE_PATH=$IMG_PATH/optee_signed.bin
	fi
	if [ ! -f $BOOT_PATH ];then
		echo "make boot.img file..."
		python3 $HOME_PATH/tools/imgsign/boot_AX650_sign.py -i $KERNEL_PATH -o $BOOT_PATH
	fi

	BIN_LIST="$EIP_PATH $FDL2_PATH $UBOOT_PATH $UBOOTBK_PATH"
	if [ "$SUPPORT_AB_PART" = "TRUE" ]; then
		BIN_LIST="$BIN_LIST $LOGO_PATH $LOGO_PATH $DTB_PATH $DTB_PATH $BOOT_PATH $BOOT_PATH $ATF_PATH $ATF_PATH"
		if [ ! "$SUPPORT_OPTEE" = "false" ]; then
			BIN_LIST="$BIN_LIST $OPTEE_PATH $OPTEE_PATH"
		fi
		BIN_LIST="$BIN_LIST $ROOTFS_PATH $ROOTFS_PATH $PARAM_PATH $PARAM_PATH $SOC_PATH $SOC_PATH $OPT_PATH"
	else
		BIN_LIST="$BIN_LIST $LOGO_PATH $DTB_PATH $BOOT_PATH $ATF_PATH"
		if [ ! "$SUPPORT_OPTEE" = "false" ]; then
			BIN_LIST="$BIN_LIST $OPTEE_PATH"
		fi
		if [ "$PROJECT" = "AX650_master" ]; then
			UBUNTU_BIN_LIST="$BIN_LIST $UBUNTU_ROOTFS_PATH $PARAM_PATH $SOC_PATH $OPT_UBUNTU_ROOTFS_PATH"
		else
			UBUNTU_BIN_LIST="$BIN_LIST $UBUNTU_ROOTFS_PATH $PARAM_PATH $SOC_PATH $OPT_PATH"
			if [ -e $UBUNTU_ROOTFS_DESKTOP_PATH ]; then
				UBUNTU_DESKTOP_BIN_LIST="$BIN_LIST $UBUNTU_ROOTFS_DESKTOP_PATH $PARAM_PATH $SOC_PATH $OPT_PATH"
			fi
		fi
		BIN_LIST="$BIN_LIST $ROOTFS_PATH $PARAM_PATH $SOC_PATH $OPT_PATH"
	fi
	ENABLE_SD_PACK=TRUE

elif [ "$PROJECT" = "AX650_slave" ]; then
	SPL_PATH=$IMG_PATH/spl_${PROJECT}_signed.bin
	EIP_PATH=$IMG_PATH/eip.bin
	FDL2_PATH=$IMG_PATH/fdl2_signed.bin
	BIN_LIST="$EIP_PATH $FDL2_PATH $SPL_PATH"
else
	DTB_PATH=$IMG_PATH/${PROJECT}.dtb
	EIP_PATH=$IMG_PATH/eip.bin
	FDL2_PATH=$IMG_PATH/fdl2_signed.bin
	UBOOT_PATH=$IMG_PATH/u-boot_signed.bin
	UBOOTBK_PATH=$IMG_PATH/uboot_bk.bin
	KERNEL_PATH=$IMG_PATH/Image
	ROOTFS_PATH=$IMG_PATH/rootfs_sparse.ext4
	PARAM_PATH=$IMG_PATH/param_sparse.ext4
	SOC_PATH=$IMG_PATH/soc_sparse.ext4
	OPT_PATH=$IMG_PATH/opt_sparse.ext4
	BOOT_PATH=$IMG_PATH/boot.img
	ATF_PATH=$IMG_PATH/atf_bl31.img
	if [ ! -f $BOOT_PATH ];then
		echo "make boot.img file..."
		python3 $HOME_PATH/tools/imgsign/boot_AX650_sign.py -i $KERNEL_PATH -o $BOOT_PATH
	fi
	BIN_LIST="$EIP_PATH $FDL2_PATH $UBOOT_PATH $UBOOTBK_PATH $DTB_PATH $BOOT_PATH $ATF_PATH $ROOTFS_PATH $PARAM_PATH $SOC_PATH $OPT_PATH"
fi
else
	echo "make axp failed"
fi

echo "make boot.img 2 file..."
python3 $TOOL_PATH -p $CHIP_GROUP -v $VERSION_EXT -x $XML_PATH -o $AXP_PATH $BIN_LIST

if [[ "$BUILD_UBUNTU_AXP" = "yes" ]]; then
if [ "$UBUNTU_BIN_LIST" ]; then
	python3 $TOOL_PATH -p $CHIP_GROUP -v $VERSION_EXT -x $XML_PATH -o $UBUNTU_AXP_PATH $UBUNTU_BIN_LIST
fi
if [ "$UBUNTU_DESKTOP_BIN_LIST" ]; then
	python3 $TOOL_PATH -p $CHIP_GROUP -v $VERSION_EXT -x $XML_PATH -o $UBUNTU_DESKTOP_AXP_PATH $UBUNTU_DESKTOP_BIN_LIST
fi
fi

if [ "$ENABLE_SD_PACK" ]; then
	python3 $HOME_PATH/tools/mkaxp/sd_upgrade_pack.py -type sd_update -path $HOME_PATH/build/out/$PROJECT/images -xml $XML_PATH -optee $optee
	python3 $HOME_PATH/tools/mkaxp/sd_upgrade_pack.py -type sd_boot -path $HOME_PATH/build/out/$PROJECT/images -xml $XML_PATH -optee $optee
fi

echo "make axp done"
