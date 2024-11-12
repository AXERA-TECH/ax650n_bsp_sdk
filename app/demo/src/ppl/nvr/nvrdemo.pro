QT += core gui widgets network

CONFIG += c++14

TARGET = nvrdemo
TEMPLATE = app

HOME_PATH        = ../../../../..
MSP_OUT_PATH     = $$HOME_PATH/msp/out
THIRD_PARTY_PATH = $$HOME_PATH/third-party
DRM_PATH         = $$THIRD_PARTY_PATH/drm/lib
APP_SRC_DIR      = $$HOME_PATH/app/demo/src

GUI_DIR     = gui
GUI_SRC_DIR = $$GUI_DIR/ui
GUI_RES_DIR = $$GUI_DIR/res
HAL_DIR     = hal
HAL_SRC_DIR = $$HAL_DIR
FRM_DIR     = frm
FRM_SRC_DIR = $$FRM_DIR
CFG_DIR     = cfg
CFG_SRC_DIR = $$CFG_DIR

DEPENDPATH  += $$MSP_OUT_PATH/lib
DEPENDPATH  += $$THIRD_PARTY_PATH/ffmpeg/lib
DEPENDPATH  += $$THIRD_PARTY_PATH/openssl/lib

INCLUDEPATH += $$MSP_OUT_PATH/include
INCLUDEPATH += $$APP_SRC_DIR/3rd/inc/live/BasicUsageEnvironment
INCLUDEPATH += $$APP_SRC_DIR/3rd/inc/live/groupsock
INCLUDEPATH += $$APP_SRC_DIR/3rd/inc/live/liveMedia
INCLUDEPATH += $$APP_SRC_DIR/3rd/inc/live/UsageEnvironment
INCLUDEPATH += $$THIRD_PARTY_PATH/ffmpeg/include
INCLUDEPATH += $$THIRD_PARTY_PATH/openssl/include
INCLUDEPATH += $$THIRD_PARTY_PATH/cmdline
INCLUDEPATH += $$APP_SRC_DIR/config/ini
INCLUDEPATH += $$APP_SRC_DIR/config/json
INCLUDEPATH += $$APP_SRC_DIR/header
INCLUDEPATH += $$APP_SRC_DIR/log
INCLUDEPATH += $$APP_SRC_DIR/utils
INCLUDEPATH += $$APP_SRC_DIR/pool
INCLUDEPATH += $$APP_SRC_DIR/rtsp
INCLUDEPATH += $$APP_SRC_DIR/vdec/stream
INCLUDEPATH += $$GUI_SRC_DIR
INCLUDEPATH += $$HAL_SRC_DIR
INCLUDEPATH += $$FRM_SRC_DIR
INCLUDEPATH += $$CFG_SRC_DIR

QMAKE_CFLAGS = `pkg-config --cflags -Wl,-rpath-link=$$THIRD_PARTY_PATH/ffmpeg/lib`

equals(ASAN, yes) {
    QMAKE_CFLAGS += -fsanitize=leak
    QMAKE_CFLAGS += -fsanitize=address
    QMAKE_CFLAGS += -fno-omit-frame-pointer
}

QMAKE_CXXFLAGS = $$QMAKE_CFLAGS

equals(ASAN, yes) {
    QMAKE_LFLAGS += -fsanitize=leak
    QMAKE_LFLAGS += -fsanitize=address
    QMAKE_LFLAGS += -fno-omit-frame-pointer
}

LIBS += -L$$MSP_OUT_PATH/lib
LIBS += -lax_sys
LIBS += -lax_vdec
LIBS += -lax_venc
LIBS += -lax_ivps
LIBS += -lax_interpreter
LIBS += -lax_engine
LIBS += -lax_skel
LIBS += -lax_vo
LIBS += -L$$THIRD_PARTY_PATH/drm/lib -ldrm
LIBS += -L$$THIRD_PARTY_PATH/ffmpeg/lib -lavcodec -lavutil -lavformat -lswresample
LIBS += -L$$THIRD_PARTY_PATH/openssl/lib -lssl -lcrypto
# LIBS += -L$$APP_SRC_DIR/3rd/lib -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment

CONFIG(debug, debug|release) {
    LIBS += -L$$APP_SRC_DIR/3rd/lib/dbg -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment
    DESTDIR = build/debug
}
CONFIG(release, debug|release) {
    LIBS += -L$$APP_SRC_DIR/3rd/lib -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment
    DESTDIR = build/release
}

RESOURCES  += $$GUI_RES_DIR/src.qrc
OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR     = $$DESTDIR/.moc
RCC_DIR     = $$DESTDIR/.qrc
UI_DIR      = $$DESTDIR/.u
OBJECTS_DIR = $$DESTDIR/.obj

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES  += main.cpp

# GUI
SOURCES  += \
    $$GUI_SRC_DIR/utils/*.cpp \
    $$GUI_SRC_DIR/nvrmainwindow.cpp \
    $$GUI_SRC_DIR/preview/PreviewLeftToolbar.cpp \
    $$GUI_SRC_DIR/preview/PreviewTopToolbar.cpp \
    $$GUI_SRC_DIR/preview/PreviewPip.cpp \
    $$GUI_SRC_DIR/preview/PreviewMain.cpp \
    $$GUI_SRC_DIR/playback/PlaybackLeftToolbar.cpp \
    $$GUI_SRC_DIR/playback/PlaybackTopToolbar.cpp \
    $$GUI_SRC_DIR/playback/PlaybackMain.cpp \
    $$GUI_SRC_DIR/playback/PlaybackTimeLine.cpp \
    $$GUI_SRC_DIR/playback/PlaybackCtrl.cpp \
    $$GUI_SRC_DIR/round/RoundPatrolMain.cpp \
    $$GUI_SRC_DIR/settings/SettingsLeftToolbar.cpp \
    $$GUI_SRC_DIR/settings/SettingsTopToolbar.cpp \
    $$GUI_SRC_DIR/settings/SettingsMain.cpp \
    $$GUI_SRC_DIR/settings/DlgDevInfo.cpp \
    $$GUI_SRC_DIR/settings/SettingPageSystem.cpp \
    $$GUI_SRC_DIR/settings/SettingPageNetwork.cpp \
    $$GUI_SRC_DIR/settings/SettingPageDevList.cpp \
    $$GUI_SRC_DIR/settings/SettingPageStorage.cpp \
    $$GUI_SRC_DIR/settings/SettingPageRoundPatrol.cpp \
    $$GUI_SRC_DIR/test_suite/*.cpp

# HAL
SOURCES  += \
    $$APP_SRC_DIR/log/*.cpp \
    $$APP_SRC_DIR/config/ini/*.cpp \
    $$APP_SRC_DIR/utils/fpsctrl.cpp \
    $$APP_SRC_DIR/utils/DiskHelper.cpp \
    $$APP_SRC_DIR/utils/ElapsedTimer.cpp \
    $$APP_SRC_DIR/utils/ping4.cpp \
    $$APP_SRC_DIR/utils/SpsParser.cpp \
    $$APP_SRC_DIR/rtsp/AXRTSPClient.cpp \
    $$HAL_SRC_DIR/linker.cpp \
    $$HAL_SRC_DIR/vo.cpp \
    $$HAL_SRC_DIR/vdec.cpp \
    $$HAL_SRC_DIR/venc.cpp \
    $$HAL_SRC_DIR/VideoEncoderEx.cpp \
    $$HAL_SRC_DIR/ivps.cpp \
    $$HAL_SRC_DIR/istream.cpp \
    $$HAL_SRC_DIR/ffmpegstream.cpp \
    $$HAL_SRC_DIR/rtspstream.cpp \
    $$HAL_SRC_DIR/rtspdamon.cpp \
    $$HAL_SRC_DIR/region.cpp \
    $$HAL_SRC_DIR/detector.cpp \
    $$HAL_SRC_DIR/datastream_record.cpp \
    $$HAL_SRC_DIR/datastream_play.cpp \
    $$HAL_SRC_DIR/datastreamfile.cpp \
    $$HAL_SRC_DIR/datastreamIndFile.cpp \
    $$HAL_SRC_DIR/framebufferPaint.cpp \
    $$HAL_SRC_DIR/streamContainer.cpp \
    $$HAL_SRC_DIR/streamTransfer.cpp

# CFG
SOURCES  += \
    $$CFG_SRC_DIR/*.cpp \

# FRM
SOURCES  += \
    $$FRM_SRC_DIR/*.cpp \

HEADERS  += \
    $$GUI_SRC_DIR/global/UiGlobalDef.h \
    $$GUI_SRC_DIR/utils/*.h \
    $$GUI_SRC_DIR/nvrmainwindow.h \
    $$GUI_SRC_DIR/preview/PreviewLeftToolbar.h \
    $$GUI_SRC_DIR/preview/PreviewTopToolbar.h \
    $$GUI_SRC_DIR/preview/PreviewPip.h \
    $$GUI_SRC_DIR/preview/PreviewMain.h \
    $$GUI_SRC_DIR/playback/PlaybackLeftToolbar.h \
    $$GUI_SRC_DIR/playback/PlaybackTopToolbar.h \
    $$GUI_SRC_DIR/playback/PlaybackMain.h \
    $$GUI_SRC_DIR/playback/PlaybackTimeLine.h \
    $$GUI_SRC_DIR/playback/PlaybackCtrl.h \
    $$GUI_SRC_DIR/round/RoundPatrolMain.h \
    $$GUI_SRC_DIR/settings/SettingsLeftToolbar.h \
    $$GUI_SRC_DIR/settings/SettingsTopToolbar.h \
    $$GUI_SRC_DIR/settings/SettingsMain.h \
    $$GUI_SRC_DIR/settings/DlgDevInfo.h \
    $$GUI_SRC_DIR/settings/SettingPageSystem.h \
    $$GUI_SRC_DIR/settings/SettingPageNetwork.h \
    $$GUI_SRC_DIR/settings/SettingPageDevList.h \
    $$GUI_SRC_DIR/settings/SettingPageStorage.h \
    $$GUI_SRC_DIR/settings/SettingPageRoundPatrol.h \
    $$GUI_SRC_DIR/test_suite/*.hpp

FORMS  += \
    $$GUI_SRC_DIR/nvrmainwindow.ui \
    $$GUI_SRC_DIR/preview/PreviewLeftToolbar.ui \
    $$GUI_SRC_DIR/preview/PreviewTopToolbar.ui \
    $$GUI_SRC_DIR/preview/PreviewPip.ui \
    $$GUI_SRC_DIR/preview/PreviewMain.ui \
    $$GUI_SRC_DIR/playback/PlaybackLeftToolbar.ui \
    $$GUI_SRC_DIR/playback/PlaybackTopToolbar.ui \
    $$GUI_SRC_DIR/playback/PlaybackMain.ui \
    $$GUI_SRC_DIR/playback/PlaybackTimeLine.ui \
    $$GUI_SRC_DIR/playback/PlaybackCtrl.ui \
    $$GUI_SRC_DIR/round/RoundPatrolMain.ui \
    $$GUI_SRC_DIR/settings/SettingsLeftToolbar.ui \
    $$GUI_SRC_DIR/settings/SettingsTopToolbar.ui \
    $$GUI_SRC_DIR/settings/SettingsMain.ui \
    $$GUI_SRC_DIR/settings/DlgDevInfo.ui \
    $$GUI_SRC_DIR/settings/SettingPageSystem.ui \
    $$GUI_SRC_DIR/settings/SettingPageNetwork.ui \
    $$GUI_SRC_DIR/settings/SettingPageDevList.ui \
    $$GUI_SRC_DIR/settings/SettingPageStorage.ui \
    $$GUI_SRC_DIR/settings/SettingPageRoundPatrol.ui


