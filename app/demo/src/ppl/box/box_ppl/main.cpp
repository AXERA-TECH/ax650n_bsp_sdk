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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <memory>
#include "AppLogApi.h"
#include "BoxBuilder.hpp"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "make_unique.hpp"
#ifdef __SLT__
#include "AXExport.h"
#include "ax_test_api.h"
#endif

static AX_VOID DisableDynamicMallocShrink(AX_S32 nKB) {
    printf("%s to %d KB\n", __func__, nKB);
    mallopt(M_TRIM_THRESHOLD, nKB*1024);
}

static bool gIsRunning = false;

#ifdef __SLT__
CBoxBuilder *gBoxApp = NULL;
EXPORT_API int BoxDemo_SLT_Stop(void) {
    if (gBoxApp) {
        gBoxApp->StopAllStreams();
    }

    return 0;
}

EXPORT_API int BoxDemo_SLT_Entry(int argc, const cmd_args *argv) {
#else
static int gExitCount = 0;
static void exit_handler(int s) {
    printf("\n====================== Caught signal: %d ======================\n", s);
    printf("please waiting to quit ...\n\n");
    gIsRunning = false;
    gExitCount++;
    if (gExitCount >= 3) {
        printf("\n======================      Force to exit    ====================== \n");
        _exit(1);
    }
}

static void ignore_sig_pipe(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1) {
        perror("failed to ignore SIGPIPE, sigaction");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[]) {
    signal(SIGINT, exit_handler);
    signal(SIGQUIT, exit_handler);
    ignore_sig_pipe();
#endif
    AX_MTRACE_ENTER(BoxDemo);

    DisableDynamicMallocShrink(128);

    try {
        std::unique_ptr<CBoxBuilder> pApp = std::make_unique<CBoxBuilder>();
        if (!pApp) {
            perror("create box build fail");
            return 1;
        }

#ifdef __SLT__
        gBoxApp = pApp.get();
#endif

        if (!pApp->Start()) {
            return 1;
        }

        gIsRunning = true;
        int nRunCount = 0;
        while (gIsRunning) {
            CElapsedTimer::GetInstance()->mSleep(1000);
            if (++nRunCount > 30) {
                // Release memory back to the system every 30 seconds.
                // https://linux.die.net/man/3/malloc_trim
                malloc_trim(0);

                nRunCount = 0;
            }

            // if (pApp->QueryStreamsAllEof()) {
            //     break;
            // }
        }

    #ifndef __SLT__
        pApp->StopAllStreams();
    #endif

        pApp->WaitDone();

    } catch (CAXException &e) {
        printf("%s\n", e.what());
        AX_MTRACE_LEAVE;
        return 1;
    }

    AX_MTRACE_LEAVE;
    return 0;
}
