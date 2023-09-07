/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <ax_base_type.h>
#include <ax_cipher_api.h>
#include <string.h>
#include <ax_sys_api.h>
#include <ax_efuse_api.h>
#define HASH_BLK_START 14
struct pub_key_2048 {
    int key_n_header;
    int rsa_key_n[64];
    int key_e_header;
    int rsa_key_e;
    int reserved[32];
};
struct pub_key_3072 {
    int key_n_header;
    int rsa_key_n[96];
    int key_e_header;
    int rsa_key_e;
};

static int printBuffer(char *buf, int size)
{
    int i = 0;
    for (i = 0 ; i < size; i++) {
        if ((i % 16) == 0) {
            printf("\n");
        }
        printf("%02x ", buf[i]);
    }
    printf("\n");
    return 0;
}

int writeHashToEfuse(unsigned int *hash, int size)
{
    int ret;
    int i;
    if (size != 32) {
        return -1;
    }
    ret = AX_EFUSE_Init();
    if (ret != AX_SUCCESS) {
        printf("Efuse init failed %x\n", ret);
        return -1;
    }
    for (i = 0; i < (size / 4); i++) {
        ret = AX_EFUSE_Write(HASH_BLK_START + i, hash[i]);
        if (ret != AX_SUCCESS) {
            printf("Efuse write blk %d failed %x\n", i, ret);
            AX_EFUSE_Deinit();
            return -1;
        }
    }
    ret = AX_EFUSE_Deinit();
    if (ret < 0) {
        printf("read id 1 fail %d\n", ret);
        return -1;
    }
    return 0;
}

static unsigned char key_n_2048[256] = {
    0x90, 0xe1, 0x31, 0x81, 0xd2, 0x22, 0x72, 0xc3, 0x13, 0x63, 0xb4, 0x04, 0x54, 0xa4, 0xf5, 0x45,
    0x95, 0xe6, 0x36, 0x86, 0xd7, 0x27, 0x77, 0xc8, 0x18, 0x68, 0xb9, 0x09, 0x59, 0xa9, 0xfa, 0x4a,
    0x9a, 0xeb, 0x3b, 0x8b, 0xdc, 0x2c, 0x7c, 0xcd, 0x1d, 0x6d, 0xbe, 0x0e, 0x5e, 0xae, 0xff, 0x4f,
    0x9f, 0xf0, 0x40, 0x90, 0xe1, 0x31, 0x81, 0xd2, 0x22, 0x72, 0xc3, 0x13, 0x63, 0xb4, 0x04, 0x54,
    0xa4, 0xf5, 0x45, 0x95, 0xe6, 0x36, 0x86, 0xd7, 0x27, 0x77, 0xc8, 0x18, 0x68, 0xb9, 0x09, 0x59,
    0xa9, 0xfa, 0x4a, 0x9a, 0xeb, 0x3b, 0x8b, 0xdc, 0x2c, 0x7c, 0xcd, 0x1d, 0x6d, 0xbe, 0x0e, 0x5e,
    0xae, 0xff, 0x4f, 0x9f, 0xf0, 0x40, 0x90, 0xe1, 0x31, 0x81, 0xd2, 0x22, 0x72, 0xc3, 0x13, 0x63,
    0xb4, 0x04, 0x54, 0xa4, 0xf5, 0x45, 0x95, 0xe6, 0x36, 0x86, 0xd7, 0x27, 0x77, 0xc8, 0x1b, 0xd9,
    0x38, 0xe8, 0x98, 0x47, 0xf7, 0xa7, 0x57, 0x06, 0xb6, 0x66, 0x15, 0xc5, 0x75, 0x24, 0xd4, 0x84,
    0x33, 0xe3, 0x93, 0x42, 0xf2, 0xa2, 0x52, 0x01, 0xb1, 0x61, 0x10, 0xc0, 0x70, 0x1f, 0xcf, 0x7f,
    0x2e, 0xde, 0x8e, 0x3d, 0xed, 0x9d, 0x4c, 0xfc, 0xac, 0x5c, 0x0b, 0xbb, 0x6b, 0x1a, 0xca, 0x7a,
    0x29, 0xd9, 0x89, 0x38, 0xe8, 0x98, 0x47, 0xf7, 0xa7, 0x57, 0x06, 0xb6, 0x66, 0x15, 0xc5, 0x75,
    0x24, 0xd4, 0x84, 0x33, 0xe3, 0x93, 0x42, 0xf2, 0xa2, 0x52, 0x01, 0xb1, 0x61, 0x10, 0xc0, 0x70,
    0x1f, 0xcf, 0x7f, 0x2e, 0xde, 0x8e, 0x3d, 0xed, 0x9d, 0x4c, 0xfc, 0xac, 0x5c, 0x0b, 0xbb, 0x6b,
    0x1a, 0xca, 0x7a, 0x29, 0xd9, 0x89, 0x38, 0xe8, 0x98, 0x47, 0xf7, 0xa7, 0x57, 0x06, 0xb6, 0x66,
    0x15, 0xc5, 0x75, 0x24, 0xd4, 0x84, 0x33, 0xe3, 0x93, 0x42, 0xf2, 0xa2, 0x52, 0x06, 0xbe, 0x2b
};
static unsigned char key_n_3072[384] = {
    0xe7, 0x46, 0xa2, 0xb1, 0xdc, 0x35, 0x72, 0xa3, 0x94, 0xc4, 0x3c, 0x88, 0xc5, 0x85, 0xb9, 0xf7,
    0xcb, 0x80, 0x6d, 0xfc, 0x9b, 0x5b, 0x21, 0x4d, 0x5a, 0x49, 0x62, 0x35, 0x85, 0xa3, 0x20, 0xe7,
    0x93, 0xa0, 0xd9, 0x2c, 0x0b, 0x98, 0xe0, 0x6c, 0x36, 0x4d, 0xb0, 0xc1, 0x97, 0xcf, 0x7d, 0x30,
    0x22, 0x7d, 0x4b, 0x40, 0x31, 0xda, 0x1f, 0x72, 0xfd, 0x98, 0x42, 0x21, 0x2e, 0x6e, 0x0d, 0x5b,
    0xad, 0xc8, 0x95, 0x2a, 0xad, 0xc0, 0xfc, 0x93, 0x8d, 0x66, 0x27, 0x43, 0xcb, 0x4e, 0x7a, 0x55,
    0x60, 0xe4, 0xac, 0x90, 0x1d, 0xf8, 0xac, 0xc8, 0x2a, 0x51, 0x32, 0xa4, 0xa7, 0xc9, 0xaa, 0x1e,
    0xc8, 0xa7, 0x0a, 0xdd, 0xcb, 0x60, 0x4c, 0xa5, 0xac, 0x01, 0x9d, 0xe1, 0xca, 0x28, 0x06, 0x38,
    0x6a, 0x83, 0x98, 0x14, 0xef, 0x8f, 0x89, 0xc6, 0xd7, 0x3c, 0x72, 0xa7, 0x93, 0x17, 0x19, 0x07,
    0x08, 0x69, 0x49, 0x84, 0xe3, 0xa6, 0x99, 0x9b, 0x6f, 0xf7, 0x16, 0x5b, 0x0e, 0xc8, 0xf0, 0x51,
    0x25, 0xd5, 0x5c, 0x0f, 0xbc, 0x11, 0x92, 0x66, 0x6c, 0x76, 0x56, 0x96, 0xd6, 0xb4, 0x2c, 0x92,
    0x0c, 0x13, 0x15, 0x29, 0x8e, 0xdd, 0x8b, 0xcd, 0x5b, 0xe4, 0x2e, 0x3f, 0x01, 0x6c, 0x89, 0x78,
    0x2a, 0x08, 0xcd, 0xce, 0x6c, 0xb7, 0x1b, 0xd8, 0x79, 0x1a, 0x66, 0x22, 0x42, 0xcf, 0xdd, 0x26,
    0x70, 0xb2, 0x40, 0x87, 0x57, 0xb8, 0xd9, 0x44, 0x13, 0xea, 0x20, 0xcb, 0xe4, 0x7c, 0x82, 0x33,
    0x59, 0x5d, 0x34, 0x46, 0x5c, 0xbf, 0x68, 0x8f, 0xab, 0x3b, 0x9f, 0x7e, 0x6e, 0x2e, 0x3f, 0x99,
    0x58, 0x7e, 0x9c, 0xd9, 0x23, 0x2c, 0x8f, 0x62, 0xfc, 0x41, 0x3f, 0x54, 0x9f, 0x1c, 0x3b, 0xc6,
    0x81, 0x3f, 0x87, 0xfb, 0x17, 0x3c, 0xbd, 0x0f, 0x4a, 0x10, 0xbc, 0xb5, 0x9b, 0x3a, 0xb5, 0xaf,
    0xa6, 0xef, 0x7b, 0x57, 0x7b, 0x91, 0xd0, 0xeb, 0x96, 0xb7, 0x55, 0x21, 0x9b, 0x8d, 0xa8, 0x58,
    0x60, 0x11, 0x84, 0x11, 0xfb, 0xb2, 0x8b, 0x6f, 0x9b, 0x69, 0x4e, 0x73, 0x35, 0x45, 0x66, 0x0d,
    0x97, 0x1c, 0xb2, 0xcd, 0x0b, 0xad, 0x11, 0xeb, 0x7d, 0xb5, 0x3a, 0x4f, 0xca, 0x6d, 0x24, 0x7f,
    0xc6, 0xb6, 0xa2, 0x19, 0xf3, 0xd5, 0x17, 0x93, 0xc7, 0x3b, 0xb0, 0x68, 0xc4, 0x78, 0xa3, 0xfb,
    0xc0, 0x8b, 0xa9, 0x61, 0x58, 0x22, 0xda, 0x0b, 0xd7, 0x68, 0xe8, 0xd9, 0xd1, 0xb8, 0x6a, 0x44,
    0x3f, 0xfc, 0xba, 0x65, 0x8e, 0xc5, 0xc9, 0xc6, 0xe6, 0x48, 0x9a, 0x92, 0xbb, 0x2f, 0x41, 0x04,
    0xfd, 0xdb, 0x5d, 0x2c, 0x14, 0xe6, 0x60, 0xd0, 0x35, 0x80, 0x5e, 0xf8, 0x10, 0x21, 0x0c, 0x5c,
    0x32, 0xa1, 0x00, 0xeb, 0xce, 0x2c, 0xb8, 0x49, 0xa2, 0x1c, 0xd0, 0x3b, 0x84, 0x14, 0xf3, 0xed,
};

static unsigned char key_e[4] = {0x00, 0x01, 0x00, 0x01};
static void *ReverseMemCpy(void *Dest, const void *Src, size_t Size)
{
    unsigned char *dp = Dest;
    const unsigned char *sp = Src;
    sp += (Size - 1);
    while (Size--) {
        *dp++ = *sp--;
    }
    return Dest;
}
int main()
{
    struct pub_key_2048 key_2048;
    struct pub_key_3072 key_3072;
    int ret;
    AX_CIPHER_HASH_CTL_T hashCtl_2048;
    AX_CIPHER_HANDLE handle_2048;

    AX_CIPHER_HASH_CTL_T hashCtl_3072;
    AX_CIPHER_HANDLE handle_3072;

    unsigned int digest_2048[256 / 32];
    unsigned int digest_3072[384 / 48];

    AX_SYS_Init();
    if (AX_CIPHER_Init() < 0) {
        printf("Cipher Init failed\n");
        return -1;
    }

    memset(&key_2048, 0, sizeof(key_2048));
    memset(&key_3072, 0, sizeof(key_3072));

    key_2048.key_n_header = 0x02000800;
    key_2048.key_e_header = 0x02010020;

    key_3072.key_n_header = 0x02000C00;
    key_3072.key_e_header = 0x02010020;

    hashCtl_2048.hashType = AX_CIPHER_ALGO_HASH_SHA256;
    hashCtl_3072.hashType = AX_CIPHER_ALGO_HASH_SHA256;

    /*change the key from big endian to little endian*/
    ReverseMemCpy(key_2048.rsa_key_n, key_n_2048, sizeof(key_n_2048));
    ReverseMemCpy(&key_2048.rsa_key_e, key_e, sizeof(key_e));
    printf("RSA2048 key is:\n");
    printBuffer((char *)&key_2048, sizeof(key_2048));

    ReverseMemCpy(key_3072.rsa_key_n, key_n_3072, sizeof(key_n_3072));
    ReverseMemCpy(&key_3072.rsa_key_e, key_e, sizeof(key_e));
    printf("RSA3072 key is:\n");
    printBuffer((char *)&key_3072, sizeof(key_3072));

    ret = AX_CIPHER_HashInit(&hashCtl_2048, &handle_2048);
    if (ret < 0) {
        printf("hash init fail %d\n", ret);
        AX_CIPHER_DeInit();
        return -1;
    }
    ret = AX_CIPHER_HashFinal(handle_2048, (unsigned char *)&key_2048, sizeof(key_2048), (unsigned char *)digest_2048);
    if (ret < 0) {
        printf("hash final fail %d\n", ret);
        AX_CIPHER_DeInit();
        return -1;
    }

    printf("RSA2048 hash value is:\n");
    printf("%08x, %08x, %08x,%08x\n", digest_2048[0], digest_2048[1], digest_2048[2], digest_2048[3]);
    printf("%08x, %08x, %08x,%08x\n", digest_2048[4], digest_2048[5], digest_2048[6], digest_2048[7]);

    ret = AX_CIPHER_HashInit(&hashCtl_3072, &handle_3072);
    if (ret < 0) {
        printf("hash init fail %d\n", ret);
        AX_CIPHER_DeInit();
        return -1;
    }

    ret = AX_CIPHER_HashFinal(handle_3072, (unsigned char *)&key_3072, sizeof(key_3072), (unsigned char *)digest_3072);
    if (ret < 0) {
        printf("hash final fail %d\n", ret);
        AX_CIPHER_DeInit();
        return -1;
    }
    printf("RSA3072 hash value is:\n");
    printf("%08x, %08x, %08x,%08x\n", digest_3072[0], digest_3072[1], digest_3072[2], digest_3072[3]);
    printf("%08x, %08x, %08x,%08x\n", digest_3072[4], digest_3072[5], digest_3072[6], digest_3072[7]);
    //writeHashToEfuse(digest, 32);
    AX_CIPHER_DeInit();
    AX_SYS_Deinit();
}
