/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <stdarg.h>
#include <chrono>
#include <string>
#include "AXSingleton.h"
#include "ax_base_type.h"

#ifdef GETTICKCOUNT
#undef GETTICKCOUNT
#endif
#define GETTICKCOUNT CElapsedTimer::GetTickCount

#define START_ELAPSED_TIME CElapsedTimer::GetInstance()->Start()
#define PRINT_ELAPSED_SEC(fmt, ...) \
    CElapsedTimer::GetInstance()->Stop(AX_TRUE, CElapsedTimer::seconds, fmt, ##__VA_ARGS__)
#define PRINT_ELAPSED_MSEC(fmt, ...) \
    CElapsedTimer::GetInstance()->Stop(AX_TRUE, CElapsedTimer::milliseconds, fmt, ##__VA_ARGS__)
#define PRINT_ELAPSED_USEC(fmt, ...) \
    CElapsedTimer::GetInstance()->Stop(AX_TRUE, CElapsedTimer::microseconds, fmt, ##__VA_ARGS__)
#define PRINT_ELAPSED_NSEC(fmt, ...) \
    CElapsedTimer::GetInstance()->Stop(AX_TRUE, CElapsedTimer::nanoseconds, fmt, ##__VA_ARGS__)

/* 时间类型时间显示格式 */
typedef enum
{
	OSD_DATE_FORMAT_YYMMDD1,				/* XXXX-XX-XX (年月日) */
	OSD_DATE_FORMAT_MMDDYY1,				/* XX-XX-XXXX (月日年) */
	OSD_DATE_FORMAT_DDMMYY1,				/* XX-XX-XXXX (日月年) */
	OSD_DATE_FORMAT_YYMMDD2,				/* XXXX年XX月XX日 */
	OSD_DATE_FORMAT_MMDDYY2,				/* XX月XX日XXXX年 */
	OSD_DATE_FORMAT_DDMMYY2,				/* XX日XX月XXXX年 */
	OSD_DATE_FORMAT_YYMMDD3,				/* XXXX/XX/XX (年月日) */
	OSD_DATE_FORMAT_MMDDYY3,				/* XXXX/XX/XX (年月日) */
	OSD_DATE_FORMAT_DDMMYY3,				/* XXXX/XX/XX (年月日) */
	OSD_DATE_FORMAT_YYMMDDWW1,				/* XXXX-XX-XX XXX (年月日 星期几) */
    OSD_DATE_FORMAT_HHmmSS,				    /* XX:XX:XX (时分秒) */
	OSD_DATE_FORMAT_YYMMDDHHmmSS,			/* XXXX-XX-XX XX:XX:XX (年月日 时分秒) */
    OSD_DATE_FORMAT_YYMMDDHHmmSSWW,			/* XXXX-XX-XX XX:XX:XX XXX (年月日 时分秒 星期几) */
	OSD_OSD_DATE_FORMAT_MAX
} OSD_DATE_FORMAT_E;

///
class CElapsedTimer : public CAXSingleton<CElapsedTimer> {
    friend class CAXSingleton<CElapsedTimer>;

public:
    using clock = std::chrono::steady_clock;
    enum { microseconds = 1, milliseconds = 2, seconds = 3, nanoseconds = 4 };

    AX_VOID Start(AX_VOID);
    AX_VOID Stop(AX_BOOL bRestart, AX_U32 nUnit, AX_CHAR const *pFmt, ...);

    /* ticked in milliseconds */
    static AX_U64 GetTickCount(AX_VOID);

    static AX_VOID mSleep(AX_U32 ms);
    static AX_VOID uSleep(AX_U32 us);
    static AX_VOID Sleep(AX_U32 sec);
    static AX_VOID Yield(AX_VOID);

    /* 2022:03:24 */
    static const AX_CHAR *GetLocalDate(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep = ':');
    /* 21:31:43:630 */
    static const AX_CHAR *GetLocalTime(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep = ':', AX_BOOL bCarryTick = AX_TRUE);
    /* 2022:03:24 21:31:43:630*/
    static const AX_CHAR *GetLocalDateTime(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep = ':');
    static wchar_t *GetCurrDateStr(wchar_t *szOut, AX_U16 nDateFmt, AX_S32 &nOutCharLen);
    /* return days of month: [1-31] */
    static AX_U32 GetCurrDay();
    static time_t StringToDatetime(std::string strYYYYMMDDHHMMSS);
    /* nSeconds: 自1970年1月1日至今描述，转换为整形pair(20230418, 101010) */
    static std::pair<AX_U32, AX_U32> GetDateTimeIntVal(time_t nSeconds);
    /* GetDateTimeIntVal的逆运算，将整形日期和时刻（20230418, 101010）转换为time_t */
    static time_t GetTimeTVal(AX_S32 nDate, AX_S32 nTime);

protected:
    AX_VOID Show(clock::time_point &end, AX_U32 nUnit, AX_CHAR const *pFmt, va_list vlist);

private:
    CElapsedTimer(AX_VOID) noexcept = default;
    virtual ~CElapsedTimer(AX_VOID) = default;

private:
    clock::time_point m_begin = clock::now();
};