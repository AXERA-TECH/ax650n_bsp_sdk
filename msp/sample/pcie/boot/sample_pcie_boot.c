/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "zlib/unzip.h"
#include "zlib/zip.h"

#define BOOT_DEVICE_NAME        "/dev/ax_pcie_boot"
#define AX_GET_ALL_DEVICES      _IOW('P', 0x1, unsigned long)
#define AX_PCIE_BOOT            _IOW('P', 0x2, unsigned long)
#define AX_START_DEVICES        _IOW('P', 0x3, unsigned long)
#define AX_PCIE_DUMP            _IOW('P', 0x4, unsigned long)
#define AX_PCIE_RESET_DEVICE        _IOW('P', 0x5, unsigned long)
#define AX_PCIE_BOOT_REASON	_IOW('P', 0x6, unsigned long)

#define MAX_DEVICE_NUM      32
#define MAX_TRANSFER_SIZE   0x300000

#define BIT(a) (1 << (a))
#define STATUS_READ_SUCCESS     BIT(0)
#define STATUS_READ_FAIL        BIT(1)
#define STATUS_WRITE_SUCCESS        BIT(13)
#define STATUS_WRITE_FAIL       BIT(14)
#define HDMA_RD_ORG_HDR_FL      BIT(2)
#define HDMA_RD_BAK_HDR_FL      BIT(3)
#define HDR_CHKSUM_FL           BIT(4)
#define HDMA_RD_CE_FW_FL        BIT(5)
#define CE_FW_CHKSUM_FL         BIT(6)
#define EIP_INIT_FL         BIT(7)
#define PUB_KEY_CHK_FL          BIT(8)
#define HDMA_RD_IMG_FL          BIT(9)
#define IMG_CHKSUM_FL           BIT(10)
#define CE_SHA_FL           BIT(11)
#define CE_AES_FL           BIT(12)

struct ax_device_info {
    unsigned int id;
    unsigned int dev_type;
};

struct boot_attr {
    unsigned int id;
    unsigned int type;
    unsigned long int src;
    unsigned long int dest;
    unsigned int len;
    unsigned int image_len;

    struct ax_device_info remote_devices[MAX_DEVICE_NUM];
};
#if 0
static void boot_find_err_type(int ret)
{
    switch (ret & ~STATUS_READ_FAIL) {
    case HDMA_RD_ORG_HDR_FL:
        printf("HDMA read original header fail\n");
        break;
    case HDMA_RD_BAK_HDR_FL:
        printf("HDMA read backup header fail\n");
        break;
    case HDR_CHKSUM_FL:
        printf("Header checksum fail\n");
        break;
    case HDMA_RD_CE_FW_FL:
        printf("HDMA read ce fw fail\n");
        break;
    case CE_FW_CHKSUM_FL:
        printf("CE fw checksum fail\n");
        break;
    case EIP_INIT_FL:
        printf("Eip init fail\n");
        break;
    case PUB_KEY_CHK_FL:
        printf("Public key verify fail\n");
        break;
    case HDMA_RD_IMG_FL:
        printf("HDMA read image fail\n");
        break;
    case IMG_CHKSUM_FL:
        printf("Image checksum fail\n");
        break;
    case CE_SHA_FL:
        printf("CE hash verification fail\n");
        break;
    case CE_AES_FL:
        printf("Image decrypto fail\n");
        break;
    default:
        printf("Other err type --> 0x%x\n", ret);
        break;
    }

}


static int get_all_devices(int fd, struct boot_attr *attr)
{
    int ret;
    int i;

    ret = ioctl(fd, AX_GET_ALL_DEVICES, attr);
    if (ret) {
        printf("get pcie devices information failed!\n");
        return -1;
    }

    for (i = 0; i < MAX_DEVICE_NUM; i++) {
        if (0 == attr->remote_devices[i].id) {
            if (0 == i) {
                printf("no slave device connect to host\n");
                return -1;
            }
            break;
        }

        printf("get device %d[%x].\n", attr->remote_devices[i].id, attr->remote_devices[i].dev_type);
    }
    return 0;
}
#endif

static int ax_pcie_start_devices(int fd, struct boot_attr *attr)
{
    int ret = 0;

    ret = ioctl(fd, AX_START_DEVICES, attr);
    if (ret < 0) {
        printf("pcie start device[%d] failed!\n", attr->id);
    }
    return ret;
}

int pcie_reset_device(int fd, struct boot_attr *attr)
{
    int ret = 0;

    ret = ioctl(fd, AX_PCIE_RESET_DEVICE, attr);
    if (ret < 0) {
        printf("pcie reset device[%d] failed!\n", attr->id);
    }
    return ret;
}


static int transfer_image_to_devices(char *name, unsigned long int dest, int fd, struct boot_attr *attr)
{
    int data_fd;
    int count = 1;
    int ret = 0;
    off_t image_len;
    void *buf = NULL;
    struct boot_attr attr_arg;
    unsigned long int offset_addr = 0;

    memset(&attr_arg, 0, sizeof(attr_arg));

    data_fd = open(name, O_RDWR);
    if (-1 == data_fd) {
        printf("open %s failed!\n", name);
        return -1;
    }

    image_len = lseek(data_fd, 0, SEEK_END);
    lseek(data_fd, 0, SEEK_SET);
    printf("size of the image:%ld\n", image_len);

    buf = malloc(MAX_TRANSFER_SIZE);
    if (NULL == buf) {
        printf("malloc for uboot failed!\n");
        ret = -1;
        goto transfer_err;
    }

    while (count) {
        memset(buf, 0, MAX_TRANSFER_SIZE);
        count = read(data_fd, buf, MAX_TRANSFER_SIZE);
        if (count == -1) {
            printf("read error\n");
            break;
        } else if (count == 0) {
            break;
        }

        attr_arg.id = attr->id;
        attr_arg.image_len = image_len;
        attr_arg.len = count;
        attr_arg.src = (unsigned long int)buf;
        attr_arg.dest = dest + offset_addr;
        ret = ioctl(fd, AX_PCIE_BOOT, &attr_arg);
        if (ret < 0) {
            printf("pcie transfer %s to device[%d] failed!\n", name, attr_arg.id);
            break;
        }
        offset_addr += count;
    }

    free(buf);
transfer_err:
    close(data_fd);
    return ret;
}

static int pcie_get_slave_boot_reason(int fd, struct boot_attr *attr)
{
    struct boot_attr attr_arg;
    memset(&attr_arg, 0, sizeof(attr_arg));

    attr_arg.id = attr->id;
    return ioctl(fd, AX_PCIE_BOOT_REASON, &attr_arg);
}

static int transfer_devices_memory_to_dumpfile(unsigned long int src_addr, unsigned long int src_size, int fd,
        struct boot_attr *attr, bool fzip)
{

    int count = 1;
    int ret = 0;
    void *buf = NULL;
    struct boot_attr attr_arg;
    unsigned long int offset_addr = 0;
    unsigned long tmp = 0;
    int dump_file;
    char dump_file_name[64];
    char dump_file_name_zip[64];
    struct tm  *ltm;
    zipFile zf = 0;

    time_t t = time(NULL);
    ltm = localtime(&t);

    sprintf(dump_file_name, "/opt/vmcore.dump.%4d%02d%02d%02d%02d%02d", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday,
            ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    sprintf(dump_file_name_zip, "/opt/vmcore.dump.%4d%02d%02d%02d%02d%02d.zip", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday,
            ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

    memset(&attr_arg, 0, sizeof(attr_arg));

    dump_file = open(dump_file_name, O_RDWR | O_CREAT);
    if (dump_file < 0) {
        perror("open error");
        return -1;
    }

    if (fzip == 1) {
        zf = zipOpen(dump_file_name_zip, APPEND_STATUS_CREATE);
        if (zf == NULL) {
            printf("zipOpen failed\n");
            return 1;
        }

        int err = zipOpenNewFileInZip(zf, dump_file_name, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);
        if (err != ZIP_OK) {
            zipClose(zf, NULL);
            printf("zipOpenNewFileInZip failed\n");
            return 1;
        }
    }

    buf = malloc(MAX_TRANSFER_SIZE);
    if (NULL == buf) {
        printf("malloc for uboot failed!\n");
        ret = -1;
        goto transfer_err;
    }

    while (src_size) {
        memset(buf, 0, MAX_TRANSFER_SIZE);

        //  if (0 == attr->remote_devices[0].id)
        //      break;

        attr_arg.id = attr->id;
        attr_arg.image_len = src_size;
        if (src_size >= MAX_TRANSFER_SIZE) {
            tmp = MAX_TRANSFER_SIZE;
        } else if (src_size < MAX_TRANSFER_SIZE) {
            tmp = src_size;
        }

        attr_arg.len = tmp;
        attr_arg.src = src_addr + offset_addr;
        attr_arg.dest = (unsigned long int)buf;

        //printf("pcie dump src_addr = %lx src_size = %lx\n",src_addr+offset_addr,src_size);

        ret = ioctl(fd, AX_PCIE_DUMP, &attr_arg);
        if (ret < 0) {
            printf("pcie transfer to device[%d] failed!\n", attr_arg.id);
            break;
        }

        count = write(dump_file, buf, tmp);
        if (count < 0) {
            printf("write dump file error\n");
        } else if (count != tmp) {
            printf("dump date error\n");
        }
        if (fzip == 1)
            zipWriteInFileInZip(zf, buf, count);//write zip file

        offset_addr += count;
        src_size -= count;
    }

    free(buf);
transfer_err:
    close(dump_file);

    if (fzip == 1) {
        zipCloseFileInZip(zf);
        zipClose(zf, NULL);
    }
    return ret;
}

void help()
{
    printf("input parameter err!\n");
    printf("boot usage:\n");
    printf("-t : <targetid> Specify device id\n");
    printf("-n : <image bin> Specify image path\n");
    printf("-d : <dest address> Specify dest address\n");
    printf("-b : <src address> Specify src address\n");
    printf("-z : <size> Specify read size\n");
    printf("-p : dump ddr\n");
    printf("-r : pcie reset\n");
    printf("-s : pcie boot start\n");
    printf("-f : dump ddr zip\n");
    printf("example:\n");
    printf("	transfer img: ./sample_pcie_boot -t <targetid> -n <image bin> -d <dest addr>\n");
    printf("	pcie reset:   ./sample_pcie_boot -t <targetid> -r\n");
    printf("	pcie start:   ./sample_pcie_boot -t <targetid> -s\n");
    printf("	pcie dump:    ./sample_pcie_boot -t <targetid> -f -b <src address> -z <size> -p\n");
    return;
}

int main(int argc, char *argv[])
{
    int Opt;
    int TargetId = -1;
    int dev_fd;
    int ret;
    bool start = false;
    bool reset = false;
    bool fzip = false;
    char *name = NULL;
    unsigned long int dest = 0;
    struct boot_attr attr;
    unsigned long int src_addr = 0;
    unsigned long int src_size = 0;
    printf("ax650 pcie boot application\n");

    memset(&attr, 0, sizeof(attr));
    dev_fd = open(BOOT_DEVICE_NAME, O_RDWR);
    if (dev_fd < 0) {
        perror("open /dev/ax_pcie_boot fail!\n");
        return -1;
    }

    while ((Opt = getopt(argc, argv, "fpsrn:t:d:b:z:h")) != -1) {
        switch (Opt) {
        case 'n':
            if (optarg != NULL) {
                name = optarg;
            } else {
                printf("file name is null\n");
                goto out;
            }
            break;
        case 't':
            if (optarg != NULL) {
                TargetId = strtol(optarg, 0, 16);
                attr.id = TargetId;
                printf("TargetId = %d\n", TargetId);
            } else {
                printf("TargetId is NULL\n");
                goto out;
            }
            break;
        case 'r':
            printf("pcie reset device %d\n", TargetId);
            reset = true;
            pcie_reset_device(dev_fd, &attr);
            break;
        case 'b':
            if (optarg != NULL) {
                src_addr = (unsigned long int)strtol(optarg, 0, 16);
                printf("src_addr = %lx\n", src_addr);
            } else {
                printf("src_addr is NULL\n");
                goto out;
            }
            break;
        case 'd':
            if (optarg != NULL) {
                dest = (unsigned long int)strtol(optarg, 0, 16);
                printf("dest = %lx\n", dest);
            } else {
                printf("dest is NULL\n");
                goto out;
            }
            break;
        case 'z':
            if (optarg != NULL) {
                src_size = (unsigned long int)strtol(optarg, 0, 16);
                printf("src_size = %lx\n", src_size);
            } else {
                printf("src_size is NULL\n");
                goto out;
            }
            break;
        case 'f':
            fzip = true;
            break;
        case 'p':
            ret = pcie_get_slave_boot_reason(dev_fd, &attr);
            if (ret > 0) {
                printf("pcie dump boot reason = %x\n", ret);
                transfer_devices_memory_to_dumpfile(src_addr, src_size, dev_fd, &attr, fzip);
            }
            goto out;
            break;
        case 's':
            start = true;
            ax_pcie_start_devices(dev_fd, &attr);
            break;
        case 'h' :
            help();
            break;
        default:
            printf("unknown option.\n");
            help();
            break;
        }
    }

    if (!start && !reset) {
        ret = transfer_image_to_devices(name, dest, dev_fd, &attr);
        if (ret < 0) {
            printf("tranfer %s to devices failed\n", name);
        } else {
            printf("tranfer %s to devices success\n", name);
        }
    }

out:
    close(dev_fd);

    return 0;
}