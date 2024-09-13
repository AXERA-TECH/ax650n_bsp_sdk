#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>

#include "cmdline.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QCoreApplication>
#include <signal.h>

#include "nvrmainwindow.h"
#include "AXNVRFramework.h"
#include "AXNVRFrameworkDefine.h"
#include "ElapsedTimer.hpp"
#include "RemoteDeviceParser.h"

#define MAX_STREAM_COUNT (64)

static AX_S32 gLoopExit = 0;

#ifdef USE_COLORBAR_FB0_FB1

AX_VOID NVR_SigInt(AX_S32 s32SigNo) {
    gLoopExit = 1;
}

AX_VOID NVR_SigStop(AX_S32 s32SigNo) {
    gLoopExit = 1;
}

AX_S32 NVR_CheckSig(AX_VOID) {.
    return gLoopExit;
}
#endif

void ctrlC_handler(int signo) {
    qDebug() << "Ctrl+C pressed!";
    gLoopExit = 1;
    QApplication::quit();
}

int main(int argc, char *argv[]) {
    printf("============== START ==============\n");

    mallopt(M_TRIM_THRESHOLD, 1024 * 128);

#ifdef USE_COLORBAR_FB0_FB1
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, NVR_SigInt);
    signal(SIGTSTP, NVR_SigStop);
    signal(SIGQUIT, NVR_SigStop);
#endif

    cmdline::parser cmd_parser;
    cmd_parser.add<AX_S32>("detectsrc", 't', "0:none 1:ivps-chn1 2:vdec-chn1", false, 2);
    cmd_parser.add<AX_S32>("screentype", 's', "0:primary 1:sencondary", false, 0);
    cmd_parser.add<AX_S32>("log", 'v', "log level", false, APP_LOG_WARN);
    cmd_parser.add<AX_S32>("testsuite", 'u', "0:disable 1:stability 2:ut", false, 0);
    /* coredump is preprocessed within run.sh */
    cmd_parser.add<AX_S32>("coredump", 'q', "0:disable coredump path config others:enable...", false, 1);
    cmd_parser.add("gdb", 'd', "gdb debug");
    cmd_parser.parse_check(argc, argv);
	AX_S32 screen_type = cmd_parser.get<AX_S32>("screentype");
    do {
        if (screen_type == 0) {
            AX_NVR_TS_RUN_MODE eMode = (AX_NVR_TS_RUN_MODE)cmd_parser.get<AX_S32>("testsuite");
            CNVRConfigParser::GetInstance()->SetTestSuiteRunMode(eMode);

            // nvr framework initialize
            if (!CAXNVRFramework::GetInstance()->Init(cmd_parser.get<AX_S32>("detectsrc"),
                                                    cmd_parser.get<AX_S32>("log"))) {
                break;
            }
            else {
                printf("============== [SUCCESS] NVR Framework initialize. ==============\n");
            }
        }
        // else if (screen_type == 1) {
        //     qputenv("QT_QPA_PLATFORM", "linuxfb:fb=/dev/fb1:size=1920x1080");
        // }

        // qDebug() << "QT_QPA_PLATFORM is set to" << qgetenv("QT_QPA_PLATFORM");

#ifndef USE_COLORBAR_FB0_FB1
        // ui initialize and show and exec
        QApplication nvr(argc, argv);
        NVRMainWindow window(screen_type, 1920, 1080);

        window.show();
        if (screen_type == 0) {
            signal(SIGINT, ctrlC_handler);
        }

        std::thread thread([]() mutable {
            int nRunCount = 0;
            while (!gLoopExit) {
                CElapsedTimer::GetInstance()->mSleep(100);
                if (++nRunCount > 300) {
                    // Release memory back to the system every 30 seconds.
                    // https://linux.die.net/man/3/malloc_trim
                    malloc_trim(0);

                    nRunCount = 0;
                }
            }
        });
        thread.detach();

        printf("============== [SUCCESS] NVR UI[%d] initialize and show. ==============\n", screen_type);
        nvr.exec();
        gLoopExit = 1;

        if (screen_type == 0) {
            window.close();
        }

#endif

    } while(0);

	if (screen_type == 0) {
#ifdef USE_COLORBAR_FB0_FB1
        while (!NVR_CheckSig()) {
            sleep(1);
        }
#endif
        CAXNVRFramework::GetInstance()->DeInit();
    }

    printf("============== END [%d] ==============\n", screen_type);
    return 0;
}
