/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <malloc.h>
#include <signal.h>
#include <sys/prctl.h>

#include "CmdLineParser.h"
#include "CommonUtils.hpp"
#include "ElapsedTimer.hpp"
#include "FramerateCtrlHelper.h"
#include "GlobalDef.h"
#include "IPCBuilder.h"
#include "ITSBuilder.h"
#include "PanoBuilder.h"
#include "PrintHelper.h"

#ifdef SLT
#include "AXExport.h"
#endif

#define MAIN "MAIN"

string g_SDKVersion = "Unreconigzed";

static AX_BOOL gRunning = AX_FALSE;
static AX_S32 gExit_count = 0;

typedef struct _PIPE_FRAMERATE_CONTROL_T {
    AX_U8 nDevID;
    AX_U8 nPipeID;
    AX_U8 nPipeFramerate;
} PIPE_FRAMERATE_CONTROL_T;

/* common functions */
#ifndef SLT
extern AX_VOID exit_handler(int s);
extern AX_VOID ignore_sig_pipe();
#endif
extern AX_VOID get_sdk_version();

IPPLBuilder* g_pPPLBuilder = nullptr;
std::map<AX_U8, std::unique_ptr<CFramerateCtrlHelper>> g_mapPipe2FrmCtrl;

#ifdef SLT
EXPORT_API int FRTDemoITS_SLT_CheckResult() {
    return CPrintHelper::GetInstance()->GetSLTResult();
}

EXPORT_API int FRTDemoITS_SLTEntry(int argc, char const* argv[]) {
#else
int main(int argc, const char* argv[]) {
#endif
    AX_S32 nRet = 0;
    nRet = CCmdLineParser::GetInstance()->Parse(argc, argv);
    if (0 != nRet) {
        _exit(1);
    }

    AX_S32 nPPLIndex = 0;
    AX_S32 nLoadType = (AX_S32)E_LOAD_TYPE_MAX;

    if (!CCmdLineParser::GetInstance()->GetPPLIndex(nPPLIndex)) {
        _exit(1);
    }

    if (!CCmdLineParser::GetInstance()->GetLoadType(nLoadType)) {
        _exit(1);
    }

    get_sdk_version();

    switch (nPPLIndex) {
        case 0: {
            g_pPPLBuilder = new AX_ITS::CITSBuilder();
            break;
        }
        case 1: {
            g_pPPLBuilder = new AX_IPC::CIPCBuilder();
            break;
        }
        case 2: {
            g_pPPLBuilder = new AX_PANO::CPanoBuilder();
            break;
        }
        default:
            break;
    }

    if (g_pPPLBuilder) {
        printf("============= APP(APP Ver: %s, SDK Ver: %s) Started %s %s =============\n", APP_BUILD_VERSION, g_SDKVersion.c_str(),
               __DATE__, __TIME__);

        if (!g_pPPLBuilder->Construct()) {
            printf("PPL build failed.\n");
            g_pPPLBuilder->Destroy();
            _exit(1);
        }

        if (!g_pPPLBuilder->Start()) {
            printf("PPL start failed.\n");
            g_pPPLBuilder->Stop();
            g_pPPLBuilder->Destroy();
            _exit(1);
        }
    }

    AX_MTRACE_ENTER(app_main);
    prctl(PR_SET_NAME, "APP_MAIN");

#ifndef SLT
    signal(SIGINT, exit_handler);
    signal(SIGQUIT, exit_handler);
    ignore_sig_pipe();
#endif

    CElapsedTimer::GetInstance()->mSleep(100);

#ifndef SLT
    AX_CHAR szIP[64] = {0};
    if (CCommonUtils::GetIP(&szIP[0])) {
        printf("Preview the video using URL: <<<<< http://%s:8080 >>>>>\n", szIP);
    } else {
        printf("Can not get host ip address.\n");
    }
#else
    printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< FRTDemo SLT Started >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
#endif

    AX_S32 nScenario = 0;
    if (!CCmdLineParser::GetInstance()->GetScenario(nScenario)) {
        LOG_M_W(MAIN, "Load default scenario %d", nScenario);
    }

    AX_S32 nLevel = 0;
    if (!CCmdLineParser::GetInstance()->GetLogLevel(nLevel)) {
        LOG_M_W(MAIN, "Load default scenario %d", nLevel);
    }

    LOG_M_C(MAIN, "CMD: ./frtdemo -p %d -s %d -n %d -l %d", nPPLIndex, nLoadType, nScenario, nLevel);
    gRunning = AX_TRUE;
    AX_U32 nRunTimes = 0;

    while (gRunning) {
        CElapsedTimer::GetInstance()->mSleep(100);
        if (++nRunTimes > 10 * 30) {
            // Release memory back to the system every 30 seconds.
            // https://linux.die.net/man/3/malloc_trim
            malloc_trim(0);

            nRunTimes = 0;
        }

#ifdef SLT
        AX_S32 nSLTRet = CPrintHelper::GetInstance()->GetSLTResult();
        if (nSLTRet == 1 || nSLTRet == 0) {
            /* FPS checks failed or reaches max run time */
            printf("\n<<<<<<<<<<<<<<<<<<<<<< FRTDemo-ITS stopped for result code: %d. >>>>>>>>>>>>>>>>>>>>>>\n", nSLTRet);
            break;
        }
#endif
    }

    if (g_pPPLBuilder) {
        if (!g_pPPLBuilder->Stop()) {
            printf("PPL stop failed.\n");
            _exit(1);
        }

        if (!g_pPPLBuilder->Destroy()) {
            printf("PPL destroy failed.\n");
            _exit(1);
        }

        delete (g_pPPLBuilder);
        g_pPPLBuilder = nullptr;
    }

    printf("============= APP(APP Ver: %s, SDK Ver: %s) Exited %s %s =============\n", APP_BUILD_VERSION, g_SDKVersion.c_str(), __DATE__,
           __TIME__);

    AX_MTRACE_LEAVE;
    return 0;
}

void exit_handler(int s) {
    printf("\n====================== Caught signal: SIGINT ======================\n");
    gRunning = AX_FALSE;
    gExit_count++;
    if (gExit_count >= 3) {
        printf("====================== Force to exit ======================\n");
        _exit(1);
    }
}

void ignore_sig_pipe() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1) {
        perror("failed to ignore SIGPIPE, sigaction");
        exit(EXIT_FAILURE);
    }
}

void get_sdk_version() {
    if (FILE* pFile = fopen("/proc/ax_proc/version", "r")) {
        char szSDKVer[64] = {0};
        fread(&szSDKVer[0], 64, 1, pFile);
        fclose(pFile);
        szSDKVer[strlen(szSDKVer) - 1] = 0;
        g_SDKVersion = szSDKVer + strlen(SDK_VERSION_PREFIX) + 1;  // 1:blank
    } else {
        printf("Failed to get sdk version information from ax_proc.");
    }
}
