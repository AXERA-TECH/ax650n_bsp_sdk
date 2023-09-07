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
typedef struct {
    AX_CIPHER_ALGO_E algorithm;
    unsigned char *msg;
    unsigned char *digest;
    unsigned char *key;
    unsigned int msgLen;
    unsigned int digestLen;
    unsigned int keyLen;
} HASH_Vector;

static  unsigned char msg_3bytes [] = {
    "abc"
};

static  unsigned char msg_33bytes [] = {
    "abcdddddddjjjjjjjmmmmmmnnnnnnoooo"
};
static  unsigned char msg_160bytes [] = {
    "abcdddddddjjjjjjjmmmmmmnnnnnnooooabcdddddddjjjjjjjmmmmmmnnnnnnoooojjjjzzzzzzqelcabcdddddddjjjjjjjmmmmmmnnnnnnooooabcdddddddjjjjjjjmmmmmmnnnnnnoooojjjjzzzzzzqelc"
};
static  unsigned char hmac_key [] = {
    "key"
};
static  unsigned char SHA1_digest [3][20] = {
    {0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d},
    {0x41, 0x2d, 0x95, 0x51, 0xbe, 0x6f, 0x75, 0xae, 0x8e, 0xd6, 0x3b, 0xd5, 0x57, 0x96, 0x35, 0x00, 0xbc, 0x05, 0xd9, 0x15},
    {0x4d, 0x47, 0xc8, 0x16, 0x28, 0xa4, 0x94, 0x64, 0x42, 0xb0, 0xbb, 0x0c, 0x31, 0x14, 0x5d, 0x65, 0x60, 0xe0, 0x29, 0x2d},
};
static  unsigned char SHA224_digest [3][28] = {
    {0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22, 0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3, 0x2a, 0xad, 0xbc, 0xe4, 0xbd, 0xa0, 0xb3, 0xf7, 0xe3, 0x6c, 0x9d, 0xa7},
    {0x0f, 0x11, 0xf4, 0xe0, 0x5b, 0xf6, 0x5c, 0xa4, 0x2e, 0x17, 0x36, 0x30, 0x00, 0x5c, 0xcc, 0xe0, 0xc5, 0xa8, 0x41, 0xdc, 0x08, 0x14, 0xb8, 0x44, 0x27, 0xf1, 0x40, 0xff},
    {0xcf, 0x32, 0x02, 0xca, 0xaf, 0x5d, 0x97, 0xd5, 0x68, 0xf2, 0xd4, 0x29, 0x4e, 0x10, 0x1a, 0x5c, 0xd7, 0xf7, 0x01, 0x04, 0x2b, 0xc9, 0x12, 0xef, 0xc3, 0x80, 0x73, 0x51},
};
static  unsigned char SHA256_digest [3][32] = {
    {0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad},
    {0xc7, 0xd6, 0x15, 0x6c, 0xff, 0x61, 0xa5, 0xe4, 0x94, 0x9d, 0x74, 0xe8, 0x8d, 0x7d, 0x18, 0x2e, 0x6a, 0xbb, 0x7e, 0xfd, 0x35, 0xb7, 0xb6, 0x37, 0x51, 0x54, 0x2b, 0x31, 0x31, 0x94, 0x82, 0xa7},
    {0x87, 0x27, 0x0d, 0x9c, 0xe5, 0x48, 0x60, 0xf0, 0xf0, 0x7f, 0x78, 0xf2, 0x73, 0x1f, 0x18, 0xc6, 0x53, 0x44, 0xc6, 0x39, 0x4c, 0x8d, 0x3f, 0x5f, 0x67, 0x47, 0x55, 0xef, 0x3b, 0xdd, 0x45, 0xab},
};

static  unsigned char SHA1_mac [3][20] = {
    {0x4f, 0xd0, 0xb2, 0x15, 0x27, 0x6e, 0xf1, 0x2f, 0x2b, 0x3e, 0x4c, 0x8e, 0xca, 0xc2, 0x81, 0x14, 0x98, 0xb6, 0x56, 0xfc},
    {0x86, 0xc9, 0xcc, 0x6f, 0x38, 0x95, 0xe3, 0x27, 0x66, 0xc9, 0x8c, 0x77, 0x22, 0x8a, 0x63, 0xc8, 0x28, 0xd4, 0xa3, 0xf7},
    {0x61, 0xd3, 0xa0, 0xdb, 0x8e, 0x2c, 0x5a, 0xfa, 0xac, 0xfc, 0xa5, 0x6b, 0x0e, 0xd1, 0xb7, 0x17, 0x18, 0x44, 0x64, 0x94},
};
static  unsigned char SHA224_mac [3][28] = {
    {0xf5, 0x24, 0x67, 0x0b, 0x7e, 0x34, 0xf3, 0x14, 0x67, 0xde, 0x0a, 0xa9, 0x65, 0x93, 0x86, 0x1c, 0xf6, 0x51, 0x17, 0xd4, 0x14, 0xfb, 0x2d, 0x86, 0x15, 0x8d, 0x76, 0x0e},
    {0x10, 0x06, 0x88, 0x55, 0x15, 0xd8, 0x22, 0x32, 0xa9, 0x95, 0xa0, 0xdf, 0x9e, 0x78, 0x21, 0xe4, 0xbc, 0x73, 0xa6, 0x40, 0x89, 0xb9, 0xab, 0x0f, 0x97, 0x85, 0xff, 0x5d},
    {0x4d, 0x2e, 0x06, 0x9d, 0xc4, 0x65, 0xe3, 0x42, 0xf7, 0x37, 0xaa, 0xb9, 0x83, 0x7e, 0x01, 0x41, 0xa0, 0x86, 0xee, 0xd8, 0x03, 0xaa, 0x74, 0xde, 0x09, 0xeb, 0x7d, 0xc2},
};
static  unsigned char SHA256_mac [3][32] = {
    {0x9c, 0x19, 0x6e, 0x32, 0xdc, 0x01, 0x75, 0xf8, 0x6f, 0x4b, 0x1c, 0xb8, 0x92, 0x89, 0xd6, 0x61, 0x9d, 0xe6, 0xbe, 0xe6, 0x99, 0xe4, 0xc3, 0x78, 0xe6, 0x83, 0x09, 0xed, 0x97, 0xa1, 0xa6, 0xab},
    {0x7c, 0xf7, 0xd6, 0xc3, 0x7d, 0x5b, 0x98, 0xf2, 0x80, 0x3c, 0x8b, 0x24, 0x36, 0xae, 0xf9, 0x3b, 0xa7, 0x1a, 0x4d, 0xae, 0xa5, 0x6d, 0xf4, 0xa5, 0xab, 0xb2, 0x8c, 0x5e, 0x50, 0xe2, 0x96, 0x54},
    {0x52, 0x4a, 0x96, 0x85, 0xde, 0x37, 0xd7, 0x08, 0x2b, 0x8f, 0x52, 0x8e, 0x48, 0x77, 0x7c, 0x91, 0x44, 0x22, 0x37, 0x28, 0x36, 0x3e, 0x33, 0x1f, 0xb8, 0x81, 0x6f, 0x0b, 0x7f, 0x34, 0xb0, 0x01},
};

#define VECTOR_HASH(alg, msg, digest, key, msgLen, keyLen) \
    {                                              \
        AX_CIPHER_ALGO_##alg,                    \
        msg, digest,key,                               \
        msgLen, sizeof(digest),keyLen                     \
    }

static  HASH_Vector hashTestVectors [] = {
    VECTOR_HASH(HASH_SHA1,      msg_3bytes,     SHA1_digest[0],   NULL, 3, 0),
    VECTOR_HASH(HASH_SHA1,      msg_33bytes,    SHA1_digest[1],   NULL, 33, 0),
    VECTOR_HASH(HASH_SHA1,      msg_160bytes,   SHA1_digest[2],   NULL, 160, 0),
    VECTOR_HASH(HASH_SHA224,    msg_3bytes,     SHA224_digest[0], NULL, 3, 0),
    VECTOR_HASH(HASH_SHA224,    msg_33bytes,    SHA224_digest[1], NULL, 33, 0),
    VECTOR_HASH(HASH_SHA224,    msg_160bytes,   SHA224_digest[2], NULL, 160, 0),
    VECTOR_HASH(HASH_SHA256,    msg_3bytes,     SHA256_digest[0], NULL, 3, 0),
    VECTOR_HASH(HASH_SHA256,    msg_33bytes,    SHA256_digest[1], NULL, 33, 0),
    VECTOR_HASH(HASH_SHA256,    msg_160bytes,   SHA256_digest[2], NULL, 160, 0),
    VECTOR_HASH(MAC_HMAC_SHA1,  msg_3bytes,     SHA1_mac[0],   hmac_key, 3, 3),
    VECTOR_HASH(MAC_HMAC_SHA1,  msg_33bytes,    SHA1_mac[1],   hmac_key, 33, 3),
    VECTOR_HASH(MAC_HMAC_SHA1,  msg_160bytes,   SHA1_mac[2],   hmac_key, 160, 3),
    VECTOR_HASH(MAC_HMAC_SHA224, msg_3bytes,     SHA224_mac[0], hmac_key, 3, 3),
    VECTOR_HASH(MAC_HMAC_SHA224, msg_33bytes,    SHA224_mac[1], hmac_key, 33, 3),
    VECTOR_HASH(MAC_HMAC_SHA224, msg_160bytes,   SHA224_mac[2], hmac_key, 160, 3),
    VECTOR_HASH(MAC_HMAC_SHA256, msg_3bytes,     SHA256_mac[0], hmac_key, 3, 3),
    VECTOR_HASH(MAC_HMAC_SHA256, msg_33bytes,    SHA256_mac[1], hmac_key, 33, 3),
    VECTOR_HASH(MAC_HMAC_SHA256, msg_160bytes,   SHA256_mac[2], hmac_key, 160, 3),
};

static int SAMPLE_CIPHER_HashSingle()
{
    int i;
    AX_CIPHER_HASH_CTL_T hashCtl;
    HASH_Vector *tv_p;
    AX_CIPHER_HANDLE handle;
    AX_U8 digest[512 / 8];
    AX_S32 ret;
    for (i = 0; i < sizeof(hashTestVectors) / sizeof(hashTestVectors[0]); i++) {
        tv_p = &hashTestVectors[i];
        hashCtl.hashType = tv_p->algorithm;
        hashCtl.hmacKey = tv_p->key;
        hashCtl.hmackeyLen = tv_p->keyLen;
        ret = AX_CIPHER_HashInit(&hashCtl, &handle);
        if (ret != AX_SUCCESS) {
            printf("%s, Hash init failed\n", __func__);
            return -1;
        }
        ret = AX_CIPHER_HashFinal(handle, tv_p->msg, tv_p->msgLen, digest);
        if (ret != AX_SUCCESS) {
            printf("%s, Hash final failed\n", __func__);
            return -1;
        }
        if (memcmp(digest, tv_p->digest, tv_p->digestLen) != 0) {
            printf("%s, hash compare fail algo %d, len = %d, len = %d\n", __func__, tv_p->algorithm, tv_p->msgLen, tv_p->digestLen);
            return -1;
        }
    }
    return 0;
}
static int SAMPLE_CIPHER_Hash_Multipart()
{
    int i, j;
    int partSize = 0;
    AX_CIPHER_HASH_CTL_T hashCtl;
    AX_S32 ret;
    HASH_Vector *tv_p;
    AX_CIPHER_HANDLE handle;
    AX_U8 digest[512 / 8];
    int partNum;
    partSize = 0x80;
    for (i = 0; i < sizeof(hashTestVectors) / sizeof(hashTestVectors[0]); i++) {
        tv_p = &hashTestVectors[i];
        if (tv_p->msgLen < partSize) {
            continue;
        }
        hashCtl.hashType = tv_p->algorithm;
        hashCtl.hmacKey = tv_p->key;
        hashCtl.hmackeyLen = tv_p->keyLen;
        ret = AX_CIPHER_HashInit(&hashCtl, &handle);
        if (ret != AX_SUCCESS) {
            printf("%s, Hash init failed, ret = %x\n", __func__, ret);
            return -1;
        }
        if (tv_p->msgLen % partSize) {
            partNum = tv_p->msgLen / partSize;
        } else {
            partNum = tv_p->msgLen / partSize - 1;
        }
        for (j = 0; j < partNum; j++) {
            ret = AX_CIPHER_HashUpdate(handle, tv_p->msg + j * partSize, partSize);
            if (ret != AX_SUCCESS) {
                printf("%s, update failed, j = %x, ret=%x\n", __func__, j, ret);
            }
        }

        ret = AX_CIPHER_HashFinal(handle, tv_p->msg + partNum * partSize, tv_p->msgLen - partNum * partSize, digest);
        if (ret != AX_SUCCESS) {
            printf("%s,Hash final failed,ret=%x\n", __func__, ret);
            return -1;
        }
        if (memcmp(digest, tv_p->digest, tv_p->digestLen) != 0) {
            printf("%s, hash compare fail algo %d, len = %d, len=%d\n", __func__, tv_p->algorithm, tv_p->msgLen, tv_p->digestLen);
            return -1;
        }
    }
    return 0;
}
int SAMPLE_CIPHER_Hash()
{
    int ret = 0;
    ret = SAMPLE_CIPHER_HashSingle();
    if (ret < 0) {
        printf("SAMPLE_CIPHER_Hash single test fail\n");
        return -1;
    }
    ret = SAMPLE_CIPHER_Hash_Multipart();
    if (ret < 0) {
        printf("SAMPLE_CIPHER_Hash_Multipart test fail\n");
        return -1;
    }
    printf("test_hash PASS\n");
    return 0;
}
