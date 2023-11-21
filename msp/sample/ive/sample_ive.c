/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <semaphore.h>

#include "sample_ive.h"
#include "ax_engine_api.h"

AX_BOOL g_bAlignNeed = AX_FALSE;
static AX_U32 g_u32TestIndex = 0;
static AX_BOOL g_bInitEngine = AX_FALSE;
sem_t g_sem_lock;

typedef struct axTEST_MULTI_CALC_T {
    AX_IVE_SRC_IMAGE_T stSrc1;
    AX_IVE_SRC_IMAGE_T stSrc2;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_DST_MEM_INFO_T  stHist;
    AX_IVE_SUB_CTRL_T stSubCtrl;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_MULTI_CALC_T;

static TEST_MULTI_CALC_T s_stTestMultiCalc;
/******************************************************************************
* function : test multi calc uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestMultiCalc_Uninit(TEST_MULTI_CALC_T* pstTestMultiCalc)
{
    IVE_CMM_FREE(pstTestMultiCalc->stSrc1.au64PhyAddr[0], pstTestMultiCalc->stSrc1.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestMultiCalc->stSrc2.au64PhyAddr[0], pstTestMultiCalc->stSrc2.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestMultiCalc->stDst.au64PhyAddr[0], pstTestMultiCalc->stDst.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestMultiCalc->stHist.u64PhyAddr, pstTestMultiCalc->stHist.u64VirAddr);

    if (NULL != pstTestMultiCalc->pFpSrc) {
        fclose(pstTestMultiCalc->pFpSrc);
        pstTestMultiCalc->pFpSrc = NULL;
    }

    if (NULL != pstTestMultiCalc->pFpDst) {
        fclose(pstTestMultiCalc->pFpDst);
        pstTestMultiCalc->pFpDst = NULL;
    }
}
/******************************************************************************
* function : test multi calc init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestMultiCalc_Init(TEST_MULTI_CALC_T* pstTestMultiCalc, AX_CHAR* pchSrcFileName,
        AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    AX_U32 u32Size;
    memset(pstTestMultiCalc, 0, sizeof(TEST_MULTI_CALC_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestMultiCalc->stSrc1), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src1 image failed!\n", s32Ret);
        SAMPLE_IVE_TestMultiCalc_Uninit(pstTestMultiCalc);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestMultiCalc->stSrc2), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src2 image failed!\n", s32Ret);
        SAMPLE_IVE_TestMultiCalc_Uninit(pstTestMultiCalc);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestMultiCalc->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestMultiCalc_Uninit(pstTestMultiCalc);
        return s32Ret;
    }

    u32Size = AX_IVE_HIST_NUM * sizeof(AX_U32);
    s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&(pstTestMultiCalc->stHist), u32Size);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create hist mem info failed!\n", s32Ret);
        SAMPLE_IVE_TestMultiCalc_Uninit(pstTestMultiCalc);
    }
    pstTestMultiCalc->stSubCtrl.enMode = AX_IVE_SUB_MODE_ABS;

    s32Ret = AX_FAILURE;
    pstTestMultiCalc->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestMultiCalc->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestMultiCalc_Uninit(pstTestMultiCalc);
        return s32Ret;
    }
    pstTestMultiCalc->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestMultiCalc->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestMultiCalc_Uninit(pstTestMultiCalc);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test multi calc
**************************************************************************/
static AX_S32 SAMPLE_IVE_TestMultiCalcProc(TEST_MULTI_CALC_T* pstTestMultiCalc)
{
    AX_S32 s32Ret;
    AX_U32* pu32Hist;
    AX_U32 i;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;
    AX_IVE_SRC_DATA_T stSrcData;
    AX_IVE_DST_DATA_T stDstData;
    AX_IVE_DMA_CTRL_T stDmaCtrl;
    memset(&stDmaCtrl, 0, sizeof(AX_IVE_DMA_CTRL_T));
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestMultiCalc->stSrc1), pstTestMultiCalc->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    stDmaCtrl.enMode = AX_IVE_DMA_MODE_DIRECT_COPY;
    stSrcData.u64VirAddr = pstTestMultiCalc->stSrc1.au64VirAddr[0];
    stSrcData.u64PhyAddr = pstTestMultiCalc->stSrc1.au64PhyAddr[0];
    stSrcData.u32Width = pstTestMultiCalc->stSrc1.u32Width;
    stSrcData.u32Height = pstTestMultiCalc->stSrc1.u32Height;
    stSrcData.u32Stride = pstTestMultiCalc->stSrc1.au32Stride[0];

    stDstData.u64VirAddr = pstTestMultiCalc->stSrc2.au64VirAddr[0];
    stDstData.u64PhyAddr = pstTestMultiCalc->stSrc2.au64PhyAddr[0];
    stDstData.u32Width = pstTestMultiCalc->stSrc2.u32Width;
    stDstData.u32Height = pstTestMultiCalc->stSrc2.u32Height;
    stDstData.u32Stride = pstTestMultiCalc->stSrc2.au32Stride[0];
    s32Ret = AX_IVE_DMA(&IveHandle, &stSrcData, &stDstData, &stDmaCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_DMA failed!\n",s32Ret);
        return s32Ret;
    }

    s32Ret = AX_IVE_Sub(&IveHandle, &pstTestMultiCalc->stSrc1, &pstTestMultiCalc->stSrc2, &pstTestMultiCalc->stDst, &pstTestMultiCalc->stSubCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Sub failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    s32Ret = AX_IVE_Hist(&IveHandle, &pstTestMultiCalc->stDst, &pstTestMultiCalc->stHist, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Hist failed!\n",s32Ret);
        return s32Ret;
    }

    s32Ret = AX_IVE_Query(IveHandle, &bFinish, bBlock);
    while (AX_ERR_IVE_QUERY_TIMEOUT == s32Ret) {
        usleep(100 * 1000);
        s32Ret = AX_IVE_Query(IveHandle, &bFinish, bBlock);
    }
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Query failed!\n",s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestMultiCalc->stDst, pstTestMultiCalc->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    pu32Hist = (AX_U32*)pstTestMultiCalc->stHist.u64VirAddr;
    if (pu32Hist[0] != pstTestMultiCalc->stSrc1.u32Width * pstTestMultiCalc->stSrc1.u32Height) {
        s32Ret = AX_FAILURE;
        SAMPLE_IVE_PRT("Test multi calc error,pu32Hist[0] = %d\n", pu32Hist[0]);
        for (i = 1; i < AX_IVE_HIST_NUM; i++) {
            if (pu32Hist[i] != 0) {
                SAMPLE_IVE_PRT("Test multi calc error, pu32Hist[%d] = %d\n", i, pu32Hist[i]);
            }
        }
    } else {
        SAMPLE_IVE_PRT("pu32Hist[0]:%d\n", pu32Hist[0]);
        SAMPLE_IVE_PRT("Test multi calc success!\n");
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test multi calc sample
******************************************************************************/
AX_VOID SAMPLE_IVE_TestMultiCalc(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc)
{
    AX_S32 s32Ret;
    AX_U32 u32Width = u32WidthSrc;
    AX_U32 u32Height = u32HeightSrc;
    AX_CHAR* pchSrcFile = pchSrcPath;
    AX_CHAR* pchDstFile = pchDstPath;
    if (!pchSrcFile || !pchDstFile) {
        SAMPLE_IVE_PRT("Error: null pointer(src or dst path not specified)!\n");
        return;
    }

    memset(&s_stTestMultiCalc, 0, sizeof(s_stTestMultiCalc));
    s32Ret = SAMPLE_IVE_TestMultiCalc_Init(&s_stTestMultiCalc, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestMultiCalc_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestMultiCalcProc(&s_stTestMultiCalc);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestMultiCalc_Uninit(&s_stTestMultiCalc);
    memset(&s_stTestMultiCalc, 0, sizeof(s_stTestMultiCalc));
}

/******************************************************************************
* function : Test multi calc sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_TestMultiCalc_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestMultiCalc_Uninit(&s_stTestMultiCalc);
    memset(&s_stTestMultiCalc, 0, sizeof(s_stTestMultiCalc));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}


/******************************************************************************
* function : to process abnormal case
******************************************************************************/
AX_VOID SAMPLE_IVE_HandleSig(AX_S32 s32Signo)
{
    if (SIGINT == s32Signo || SIGTERM == s32Signo) {
        AX_U32 u32Index = g_u32TestIndex;
        if (u32Index < 0)
            return;
        if(sem_trywait(&g_sem_lock)==0) {
            switch (u32Index) {
            case 0:
                SAMPLE_IVE_DMA_TEST_HandleSig();
                break;
            case 1:
                SAMPLE_IVE_DualPicCalc_TEST_HandleSig();
                break;
            case 2:
                SAMPLE_IVE_EdgeDetection_TEST_HandleSig();
                break;
            case 3:
                SAMPLE_IVE_CCL_TEST_HandleSig();
                break;
            case 4:
                SAMPLE_IVE_ED_TEST_HandleSig();
                break;
            case 5:
                SAMPLE_IVE_Filter_TEST_HandleSig();
                break;
            case 6:
                SAMPLE_IVE_Hist_TEST_HandleSig();
                break;
            case 7:
                SAMPLE_IVE_Integ_TEST_HandleSig();
                break;
            case 8:
                SAMPLE_IVE_MagAng_TEST_HandleSig();
                break;
            case 9:
                SAMPLE_IVE_Sobel_TEST_HandleSig();
                break;
            case 10:
                SAMPLE_IVE_GMM_TEST_HandleSig();
                break;
            case 11:
                SAMPLE_IVE_Thresh_TEST_HandleSig();
                break;
            case 12:
                SAMPLE_IVE_16To8Bit_TEST_HandleSig();
                break;
            case 13:
                SAMPLE_IVE_TestMultiCalc_HandleSig();
                break;
            case 14:
                SAMPLE_IVE_CropResize_TEST_HandleSig();
                break;
            case 15:
                SAMPLE_IVE_CSC_TEST_HandleSig();
                break;
            case 16:
                SAMPLE_IVE_CropResize2_TEST_HandleSig();
                break;
            case 17:
                SAMPLE_IVE_MatMul_TEST_HandleSig(g_bInitEngine);
                break;
            default :
                break;
            }
            sem_post(&g_sem_lock);
        }
        SAMPLE_IVE_PRT("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}
/******************************************************************************
* function : show usage
******************************************************************************/

static AX_CHAR optstr[] = "?::c:e:m:t:i:o:w:h:p:a:";
static const struct option long_options[] = {
    {"calc_choice", required_argument, NULL, 'c'},
    {"engine_choice", required_argument, NULL, 'e'},
    {"mode_ctl", required_argument, NULL, 'm'},
    {"type_image", required_argument, NULL, 't'},
    {"input_path", required_argument, NULL, 'i'},
    {"out_path", required_argument, NULL, 'o'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"param_list", required_argument, NULL, 'p'},
    {"align_need", required_argument, NULL, 'a'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

AX_VOID SAMPLE_IVE_Usage(AX_CHAR* pchPrgName)
{
    printf("Usage : %s -c case_index [options]\n", pchPrgName);

    printf("\t-c | --case_index:Calc case index, default:0\n");
    printf("\t\t0-DMA.\n");
    printf("\t\t1-DualPicCalc.\n");
    printf("\t\t2-HysEdge and CannyEdge.\n");
    printf("\t\t3-CCL.\n");
    printf("\t\t4-Erode and Dilate.\n");
    printf("\t\t5-Filter.\n");
    printf("\t\t6-Hist and EqualizeHist.\n");
    printf("\t\t7-Integ.\n");
    printf("\t\t8-MagAng.\n");
    printf("\t\t9-Sobel.\n");
    printf("\t\t10-GMM and GMM2.\n");
    printf("\t\t11-Thresh.\n");
    printf("\t\t12-16bit to 8bit.\n");
    printf("\t\t13-Multi Calc.\n");
    printf("\t\t14-Crop and Resize.\n");
    printf("\t\t15-CSC.\n");
    printf("\t\t16-CropResize2.\n");
    printf("\t\t17-MatMul.\n");

    printf("\t-e | --engine_choice:Choose engine id, default:0\n");
    printf("\t\t0-IVE; 1-TDP; 2-VGP; 3-VPP; 4-GDC; 5-DSP; 6-NPU; 7-CPU; 8-MAU.\n");
    printf("\t\tFor Crop and Resize case, cropimage support IVE/VGP/VPP engine, cropresize and cropresize_split_yuv support VGP/VPP engine.\n");
    printf("\t\tFor CSC case, support TDP/VGP/VPP engine.\n");
    printf("\t\tFor CropResize2 case, support VGP/VPP engine.\n");
    printf("\t\tFor MatMul case, support NPU/MAU engine.\n");

    printf("\t-m | --mode_choice:Choose test mode, default:0\n");
    printf("\t\tFor DualPicCalc case, indicate dual pictures calculation task:\n"
           "\t\t  0-add; 1-sub; 2-and; 3-or; 4-xor; 5-mse.\n");
    printf("\t\tFor HysEdge and CannyEdge case, indicate hys edge or canny edge calculation task:\n"
           "\t\t  0-hys edge; 1-canny edge.\n");
    printf("\t\tFor Erode and Dilate case, indicate erode or dilate calculation task:\n"
           "\t\t  0-erode; 1-dilate.\n");
    printf("\t\tFor Hist and EqualizeHist case, indicate hist or equalize hist calculation task:\n"
           "\t\t  0-hist; 1-equalize hist.\n");
    printf("\t\tFor GMM and GMM2 case, indicate gmm or gmm2 calculation task:\n"
           "\t\t  0-gmm; 1-gmm2.\n");
    printf("\t\tFor Crop and Resize case, indicate cropimage, cropresize, cropresize_split_yuv calculation task:\n"
           "\t\t  0-crop image; 1-crop_resize; 2-cropresize_split_yuv.\n");
    printf("\t\tFor CropResize2 case, indicate crop_resize2 or cropresize2_split_yuv calculation task:\n"
           "\t\t  0-crop_resize2; 1-cropresize2_split_yuv.\n");

    printf("\t-t | --type_image:Image type index refer to IVE_IMAGE_TYPE_E(IVE engine) or AX_IMG_FORMAT_E(other engine)\n");
    printf("\t\tNote:\n"
        "\t\t  1. For all case, both input and output image types need to be specified in the same order as the specified input and output file order.\n"
        "\t\t  2. If no type is specified, i.e. a type value of -1 is passed in, then a legal type is specified, as qualified by the API documentation.\n"
        "\t\t  3. Multiple input and output image types, separated by spaces.\n"
        "\t\t  4. For One-dimensional data (such as AX_IVE_MEM_INFO_T type data), do not require a type to be specified.\n");

    printf("\t-i | --input_files:Input image files, if there are multiple inputs, separated by spaces.\n");
    printf("\t-o | --output_files:Output image files or dir, if there are multiple outputs, separated by spaces\n"
        "\t\tNote:for DMA, Crop Resize, blob of CCL case and CropResize2 case must be specified as directory.\n");
    printf("\t-w | --width:Image width of inputs, default:1280.\n");
    printf("\t-h | --height:Image height of inputs, default:720.\n");
    printf("\t-p | --param_list:Control parameters list or file(in json data format)\n");
    printf("\t\tNote:\n"
        "\t\t  1. Please refer to the json file in the '/opt/data/ive/' corresponding directory of each test case.\n"
        "\t\t  2. For MagAng, Multi Calc and CSC case, no need control parameters.\n");
    printf("\t-a | --align_need:Does the width/height/stride need to be aligned automatically, default:0.\n"
        "\t\t  0-no; 1-yes.\n");
    printf("\t-? | --help:Show usage help.\n");

}

/******************************************************************************
* function : ive sample
******************************************************************************/
int main(int argc, char *argv[])
{
    AX_S32 s32Ret;
    AX_U32 u32Index = 0;
    AX_U32 u32EngineId = 0;
    AX_U32 u32Mode = 0;
    AX_S32 as32Type[5] = {-1, -1, -1, -1, -1};
    AX_CHAR *pchPathSrc[2] = {NULL, NULL};
    AX_CHAR *pchPathDst[3] = {NULL, NULL, NULL};
    AX_U32 u32WidthSrc = 1280;
    AX_U32 u32HeightSrc = 720;
    AX_CHAR *pchParamsList = NULL;

    if (argc < 2) {
        SAMPLE_IVE_Usage(argv[0]);
        return AX_FAILURE;
    }

    s32Ret = sem_init(&g_sem_lock, 0, 1);
    if(s32Ret != 0){
        SAMPLE_IVE_PRT("sem_init failed!\n");
        return AX_FAILURE;
    }
    signal(SIGINT, SAMPLE_IVE_HandleSig);
    signal(SIGTERM, SAMPLE_IVE_HandleSig);

    while ((s32Ret = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
        if (s32Ret < 0)
            break;

        switch(s32Ret) {
        case 'c':
            u32Index = (AX_U32)atoi(optarg);
            break;
        case 'e':
            u32EngineId = (AX_U32)atoi(optarg);
            if (u32EngineId >= AX_IVE_ENGINE_BUTT) {
                SAMPLE_IVE_PRT("Engine[%d] illegal!\n", u32EngineId);
                return AX_FAILURE;
            }
        break;
        case 'm':
            u32Mode = (AX_U32)atoi(optarg);
            break;
        case 't':
            as32Type[0] = atoi(argv[optind - 1]);
            if (u32Index != 6) {
                if (optind <= (argc - 1))
                    as32Type[1] = atoi(argv[optind]);
                if (u32Index == 1 || (u32Index == 2 && u32Mode == 0)
                    || u32Index == 8 || u32Index == 10 || u32Index == 13) {
                    if ((optind + 1) <= (argc -1))
                        as32Type[2] = atoi(argv[optind + 1]);
                }
                if (u32Index == 8) {
                    if ((optind + 2) <= (argc -1))
                        as32Type[3] = atoi(argv[optind + 2]);
                }
            }
            break;
        case 'i':
            pchPathSrc[0] = argv[optind - 1];
            if (u32Index == 1 || (u32Index == 2 && u32Mode == 0)
                || u32Index == 8 || u32Index == 10 || u32Index == 16) {
                if (optind <= (argc - 1))
                    pchPathSrc[1] = argv[optind];
            }
            break;
        case 'o':
            pchPathDst[0] = argv[optind - 1];
            if (u32Index == 3 || u32Index == 8 || u32Index == 10) {
                if (optind <= (argc - 1))
                    pchPathDst[1] = argv[optind];
                if (u32Index == 10) {
                    if ((optind + 1) <= (argc -1))
                        pchPathDst[2] = argv[optind + 1];
                }
            }
            break;
        case 'w':
            u32WidthSrc = (AX_U32)atoi(optarg);
            break;
        case 'h':
            u32HeightSrc = (AX_U32)atoi(optarg);
            break;
        case 'p':
            pchParamsList = optarg;
            break;
        case 'a':
            g_bAlignNeed = (AX_BOOL)atoi(optarg);
            break;
        case '?':
        default:
            SAMPLE_IVE_Usage(argv[0]);
            return 0;
        }
    }
    g_u32TestIndex = u32Index;

    if (AX_SYS_Init() < 0) {
        SAMPLE_IVE_PRT("Sys init failed!\n");
        return AX_FAILURE;
    }

    if ((s32Ret = AX_IVE_Init()) < 0) {
        SAMPLE_IVE_PRT("Ive init failed, s32Ret=0x%x!\n", s32Ret);
        return AX_FAILURE;
    }

    switch(u32Index) {
    case 0://DMA
        SAMPLE_IVE_DMA_TEST(as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 1://Dual picture calculate
        if (u32Mode > 5) {
            SAMPLE_IVE_PRT("Mode[%d] illegal!\n", u32Mode);
            SAMPLE_IVE_Usage(argv[0]);
            goto EXIT;
        }
        SAMPLE_IVE_DualPicCalc_TEST(u32Mode, as32Type, pchPathSrc, pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 2://HysEdge and CannyEdge
        if (u32Mode > 1) {
            SAMPLE_IVE_PRT("Mode[%d] illegal!\n", u32Mode);
            SAMPLE_IVE_Usage(argv[0]);
            goto EXIT;
        }
        SAMPLE_IVE_EdgeDetection_TEST(u32Mode, as32Type, pchPathSrc, pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 3://CCL
        SAMPLE_IVE_CCL_TEST(as32Type, pchPathSrc[0], pchPathDst, u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 4://Erode and Dilate
        if (u32Mode > 1) {
            SAMPLE_IVE_PRT("Mode[%d] illegal!\n", u32Mode);
            SAMPLE_IVE_Usage(argv[0]);
            goto EXIT;
        }
        SAMPLE_IVE_ED_TEST(u32Mode, as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 5://Filter
        SAMPLE_IVE_Filter_TEST(as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 6://Hist and EqualHist
        if (u32Mode > 1) {
            SAMPLE_IVE_PRT("Mode[%d] illegal!\n", u32Mode);
            SAMPLE_IVE_Usage(argv[0]);
            goto EXIT;
        }
        SAMPLE_IVE_Hist_TEST(u32Mode, as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 7://Integ
        SAMPLE_IVE_Integ_TEST(as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 8://MagAng
        SAMPLE_IVE_MagAng_TEST(as32Type, pchPathSrc, pchPathDst, u32WidthSrc, u32HeightSrc);
        break;
    case 9://Sobel
        SAMPLE_IVE_Sobel_TEST(as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 10://GMM and GMM2
        if (u32Mode > 1) {
            SAMPLE_IVE_PRT("Mode[%d] illegal!\n", u32Mode);
            SAMPLE_IVE_Usage(argv[0]);
            goto EXIT;
        }
        SAMPLE_IVE_GMM_TEST(u32Mode, as32Type, pchPathSrc, pchPathDst, u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 11://Thresh
        SAMPLE_IVE_Thresh_TEST(as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 12://16bit to 8bit
        SAMPLE_IVE_16To8Bit_TEST(as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 13://MultiCalc
        SAMPLE_IVE_TestMultiCalc(as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc);
        break;
    case 14://CropResize
        if (u32Mode > 2) {
            SAMPLE_IVE_PRT("Mode[%d] illegal!\n", u32Mode);
            SAMPLE_IVE_Usage(argv[0]);
            goto EXIT;
        }
        SAMPLE_IVE_CropResize_TEST(u32EngineId, u32Mode, as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 15://CSC
        SAMPLE_IVE_CSC_TEST(u32EngineId, as32Type, pchPathSrc[0], pchPathDst[0], u32WidthSrc, u32HeightSrc);
        break;
    case 16://CropResize2
        if (u32Mode > 1) {
            SAMPLE_IVE_PRT("Mode[%d] illegal!\n", u32Mode);
            SAMPLE_IVE_Usage(argv[0]);
            goto EXIT;
        }
        SAMPLE_IVE_CropResize2_TEST(u32EngineId, u32Mode, as32Type, pchPathSrc, pchPathDst[0], u32WidthSrc, u32HeightSrc, pchParamsList);
        break;
    case 17: { //MatMul
            AX_ENGINE_NPU_ATTR_T stNpuAttr;
            memset(&stNpuAttr, 0, sizeof(stNpuAttr));
            for (int i = 0; i < AX_ENGINE_VIRTUAL_NPU_BUTT; i++) {
                stNpuAttr.eHardMode = (AX_ENGINE_NPU_MODE_T)i;
                AX_S32 s32Ret = AX_ENGINE_Init(&stNpuAttr);
                if (s32Ret == 0) {
                    SAMPLE_IVE_PRT("Found npu mode:%d\n", stNpuAttr.eHardMode);
                    g_bInitEngine = AX_TRUE;
                    break;
                }
            }
            SAMPLE_IVE_MatMul_TEST(u32EngineId, pchParamsList);
        }
        break;
    default :
        SAMPLE_IVE_PRT("No support the testing case!\n");
        SAMPLE_IVE_Usage(argv[0]);
        goto EXIT;
        break;
    }

EXIT:
    sem_wait(&g_sem_lock);
    if (g_bInitEngine) {
        AX_ENGINE_Deinit();
    }
    AX_IVE_Exit();
    AX_SYS_Deinit();
    sem_post(&g_sem_lock);

    return 0;

}



