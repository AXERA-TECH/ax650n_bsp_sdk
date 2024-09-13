#pragma once

enum class WINDOW_TYPE {
    PREVIEW     = 0,
    PLAYBACK    = 1,
    SETTINGS    = 2,
    POLLING     = 3
};

typedef struct _NVR_RECT_T {
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int w = 0;
    unsigned int h = 0;
} NVR_RECT_T;

enum class SPLIT_TYPE {
     ONE        = 1,    // 1x1=1
     FOUR       = 2,    // 2x2=4
     EIGHT      = 7,    // 1+7=8
     SIXTEEN    = 4,    // 4x4=16
     THIRTYSIX  = 6,    // 6x6=32
     SIXTYFOUR  = 8,    // 8x8=64
     PIP        = 98,
     MAX        = 99,
     UNKNOW     = -1
};

enum class MAIN_ACTION_TYPE {
    UNKNOWN            = -1,
    MAIN_SHOW          = 0,
    MAIN_SHOW_PREVIEW  = 1,
    MAIN_SHOW_PLAYBACK = 2,
    MAIN_SHOW_SETTING  = 3,
};

enum class PREVIEW_ACTION_TYPE {
    UNKNOWN            = -1,
    PREVIEW_SHOW       = 0,
    PREVIEW_SPLIT      = 1,
    PREVIEW_PREVIOUS   = 2,
    PREVIEW_NEXT       = 3,
    PREVIEW_MAINSUB    = 4,
    PREVIEW_MIN_MAX    = 5
};

enum class PREV_NEXT_TYPE {
     PREV       = 0,    // previous
     NEXT       = 1,
     UNKNOW     = -1
};

enum class DISPLAY_TYPE {
     SHOW       = 0,
     HIDE       = 1,
     UPDATE     = 2
};

enum class CONFIG_TYPE {
     SYSTEM       = 0,
     DEVICE       = 1,
     RECORD       = 2
};

enum class PLAYBACK_ACTION_TYPE {
     INIT           = 0,

     PLAY           = 1,
     PAUSE          = 2,
     RESUME         = 3,
     STOP           = 4,
     REVERSE        = 5,
     PREV_FRAME     = 6,
     NEXT_FRAME     = 7,
     SLOW_SPEED     = 8,
     FAST_SPEED     = 9,
     PREV           = 10,
     NEXT           = 11,

     UPDATE         = 12,
     MAINSUB1       = 13,
};

enum class PLAYBACK_ACTION_STATUS_TYPE {
     INIT           = 1,
     PLAY           = 1,
     PAUSE          = 2,
     STOP           = 3,
     REWIND         = 4,
};


enum class AX_NVR_PREVIEW_RES_TYPE {
    UNKNOWN         = 0x00,

    PREVIEW_ERR     = 0x10,
    PREVIEW_OK      = 0x11,
    PREVIEW_UPDATE  = 0x12,

    RPATROL_ERR     = 0x20,
    RPATROL_START   = 0x21,
    RPATROL_STOP    = 0x22,
    RPATROL_UPDATE  = 0x23,
};

typedef struct _AX_NVR_ACTION_RES_T {
    AX_NVR_PREVIEW_RES_TYPE enResult = AX_NVR_PREVIEW_RES_TYPE::UNKNOWN;
    SPLIT_TYPE enSplitType = SPLIT_TYPE::UNKNOW;
    MAIN_ACTION_TYPE enMainActionType = MAIN_ACTION_TYPE::UNKNOWN;
    PREVIEW_ACTION_TYPE enPreviewActionType = PREVIEW_ACTION_TYPE::UNKNOWN;
    PLAYBACK_ACTION_TYPE enPlaybackActionType = PLAYBACK_ACTION_TYPE::INIT;
} AX_NVR_ACTION_RES_T;

#define ROUND_SPLIT_START       "ROUND_SPLIT_I\n"
#define ROUND_SPLIT_ONE         "ROUND_SPLIT_O\n"
#define ROUND_SPLIT_FOUR        "ROUND_SPLIT_F\n"
#define ROUND_SPLIT_EIGHT       "ROUND_SPLIT_E\n"
#define ROUND_SPLIT_SIXTEEN     "ROUND_SPLIT_S\n"
#define ROUND_SPLIT_THIRTYSIX   "ROUND_SPLIT_T\n"
#define ROUND_SPLIT_FINISH      "ROUND_SPLIT_H\n"
#define ROUND_SPLIT_CLOSE       "ROUND_SPLIT_C\n"

#define ROUND_SPLIT_IP          "127.0.0.1"
#define ROUND_SPLIT_PORT        (12345)

#define CSS_TABLE "\
color:rgb(230,230,230);  \
gridline-color:rgb(95,95,95);  \
alternate-background-color:rgb(55,55,55);  \
background-color:rgb(44,44,44) \
"

#define CSS_SCROLLBAR "\
QScrollBar:vertical{background-color:transparent;width:20px;height:255px;padding-top:20px;padding-bottom:20px;} \
QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background-color:rgb(55,55,55);} \
QScrollBar::handle:vertical{background:rgba(33,33,33,0.8);border:0px;} \
"

#define CSS_TABLE_HEAD "\
QHeaderView::section {background-color:rgb(55,55,55);} \
"

#define CSS_PUSHBUTTON "\
QPushButton:disabled{background-color: rgb(0, 0, 0);outline: none;} \
QPushButton:enabled{background-color: rgb(44, 44, 44);outline: none;} \
"

#define CSS_WIDGET "\
color: rgb(255, 255, 255);	\
background-color: rgb(44, 44, 44);	\
"

#define CSS_WIDGET_1 "\
color: rgb(255, 255, 255);	\
background-color: rgb(32, 32, 32);	\
"

#define CSS_WIDGET_POPUP "\
color: rgb(255, 255, 255);	\
background-color: rgb(68, 68, 68);	\
"

#define CSS_TABWIDGET "\
QTabBar::tab{	\
    background-color: #0B0E11;	\
}	\
QTabBar::tab:selected{	\
    background-color: rgba(108, 117, 125, 65);	\
}	\
QTabBar::tab:hover:!selected {	\
    background-color: rgba(108, 117, 125, 45);	\
}	\
QTabWidget::pane {	\
    border: 1px solid rgba(108, 117, 125, 65);	\
}	\
"

#define CSS_CALENDAR " \
#qt_calendar_calendarview {	\
    background: rgb(44, 44, 44);	\
    alternate-background-color: rgb(44, 44, 44);	\
}	\
#calendarWidget {	\
    background: rgb(44, 44, 44);	\
}	\
QToolButton#qt_calendar_prevmonth {	\
    background: rgb(44, 44, 44);	\
}	\
QToolButton#qt_calendar_nextmonth {	\
    background: rgb(44, 44, 44);	\
}	\
QAbstractItemView {	\
    color: white;	\
    selection-color: white;	\
    selection-background-color: rgb(180, 180, 180);	\
}	\
"
