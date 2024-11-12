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
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <thread>
#include "ElapsedTimer.hpp"
#include "AppLog.hpp"

#define TIME "TIME"

using namespace std;

AX_U64 CElapsedTimer::GetTickCount(AX_VOID) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

AX_VOID CElapsedTimer::mSleep(AX_U32 ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

AX_VOID CElapsedTimer::uSleep(AX_U32 us) {
    this_thread::sleep_for(chrono::microseconds(us));
}

AX_VOID CElapsedTimer::Sleep(AX_U32 sec) {
    this_thread::sleep_for(chrono::seconds(sec));
}

AX_VOID CElapsedTimer::Yield(AX_VOID) {
    /* fixme: does app really need yield? */
#if 1
    // Here nanoseconds can be 0, but for all OS, we set it 1 nanosecond
    // sleep(0) and probably nanosleep with zero provide a mechanism for a thread to surrender the rest of its
    // timeslice. This effectively is a thread yield. So when calling sleep(0) we enter kernel mode and places the
    // thread onto the "runnable" queue. https://stackoverflow.com/questions/58630270/why-does-nanosleep-0-still-execute
    const long nanoseconds = 1;
    struct timespec ts = {0, nanoseconds};
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno))
        ;
#else
    this_thread::yield();
#endif
}

const AX_CHAR *CElapsedTimer::GetLocalDate(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep) {
    struct tm t;
    time_t now = time(NULL);
    localtime_r(&now, &t);

    snprintf(pBuf, nBufSize, "%04d%c%02d%c%02d", t.tm_year + 1900, cSep, t.tm_mon + 1, cSep, t.tm_mday);
    return pBuf;
}

const AX_CHAR *CElapsedTimer::GetLocalTime(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep, AX_BOOL bCarryTick /*= AX_TRUE*/) {
    timeval tv;
    struct tm t;
    gettimeofday(&tv, NULL);
    time_t now = time(NULL);
    localtime_r(&now, &t);

    if (bCarryTick) {
        snprintf(pBuf, nBufSize, "%02d%c%02d%c%02d%c%03d", t.tm_hour, cSep, t.tm_min, cSep, t.tm_sec, cSep, (AX_U32)(tv.tv_usec / 1000));
    } else {
        snprintf(pBuf, nBufSize, "%02d%c%02d%c%02d", t.tm_hour, cSep, t.tm_min, cSep, t.tm_sec);
    }

    return pBuf;
}

const AX_CHAR *CElapsedTimer::GetLocalDateTime(AX_CHAR *pBuf, AX_U32 nBufSize, AX_CHAR cSep) {
    timeval tv;
    struct tm t;
    gettimeofday(&tv, NULL);
    time_t now = time(NULL);
    localtime_r(&now, &t);

    snprintf(pBuf, nBufSize, "%04d%c%02d%c%02d %02d%c%02d%c%02d%c%03d", t.tm_year + 1900, cSep, t.tm_mon + 1, cSep, t.tm_mday, t.tm_hour, cSep,
             t.tm_min, cSep, t.tm_sec, cSep, (AX_U32)(tv.tv_usec / 1000));
    return pBuf;
}

AX_VOID CElapsedTimer::Start(AX_VOID) {
    m_begin = clock::now();
}

AX_VOID CElapsedTimer::Stop(AX_BOOL bRestart, AX_U32 nUnit, AX_CHAR const *pFmt, ...) {
    auto end = clock::now();

    va_list args;
    va_start(args, pFmt);
    Show(end, nUnit, pFmt, args);
    va_end(args);

    if (bRestart) {
        m_begin = clock::now();
    }
}

AX_VOID CElapsedTimer::Show(clock::time_point &end, AX_U32 nUnit, AX_CHAR const *pFmt, va_list vlist) {
    AX_CHAR szBuf[1024] = {0};
    AX_S32 nLen = vsnprintf(szBuf, sizeof(szBuf), pFmt, vlist);
    if (nLen < 0) {
        return;
    }

    switch (nUnit) {
        case CElapsedTimer::seconds:
            printf("%s => elapsed: %d (s)\n", szBuf,
                   (AX_U32)(chrono::duration_cast<chrono::seconds>(end - m_begin).count()));
            break;
        case CElapsedTimer::milliseconds:
            printf("%s => elapsed: %d (ms)\n", szBuf,
                   (AX_U32)(chrono::duration_cast<chrono::milliseconds>(end - m_begin).count()));
            break;
        case CElapsedTimer::microseconds:
            printf("%s => elapsed: %lld (us)\n", szBuf,
                   (AX_U64)(chrono::duration_cast<chrono::microseconds>(end - m_begin).count()));
            break;
        case CElapsedTimer::nanoseconds:
            printf("%s => elapsed: %lld (ns)\n", szBuf,
                   (AX_U64)(chrono::duration_cast<chrono::nanoseconds>(end - m_begin).count()));
            break;
        default:
            printf("%s => elapsed: %d (ms)\n", szBuf,
                   (AX_U32)(chrono::duration_cast<chrono::milliseconds>(end - m_begin).count()));
            break;
    }
}


AX_U32 CElapsedTimer::GetCurrDay() {
    struct tm t;
    time_t now = time(NULL);
    localtime_r(&now, &t);
    return t.tm_mday;
}

wchar_t *CElapsedTimer::GetCurrDateStr(wchar_t *szOut, AX_U16 nDateFmt, AX_S32 &nOutCharLen) {
    time_t t;
    struct tm tm;

    t = time(NULL);
    localtime_r(&t, &tm);

    AX_U32 nDateLen = 64;
    OSD_DATE_FORMAT_E eDateFmt = (OSD_DATE_FORMAT_E)nDateFmt;
    switch (eDateFmt) {
        case OSD_DATE_FORMAT_YYMMDD1: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case OSD_DATE_FORMAT_MMDDYY1: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%02d-%02d-%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case OSD_DATE_FORMAT_DDMMYY1: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case OSD_DATE_FORMAT_YYMMDD2: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%04d年%02d月%02d日", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case OSD_DATE_FORMAT_MMDDYY2: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%02d月%02d日%04d年", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case OSD_DATE_FORMAT_DDMMYY2: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%02d日%02d月%04d年", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case OSD_DATE_FORMAT_YYMMDD3: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%04d/%02d/%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case OSD_DATE_FORMAT_MMDDYY3: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%02d/%02d/%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case OSD_DATE_FORMAT_DDMMYY3: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case OSD_DATE_FORMAT_YYMMDDWW1: {
            const wchar_t *wday[] = {L"星期天", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"};
            nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %ls", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, wday[tm.tm_wday]);
            break;
        }
        case OSD_DATE_FORMAT_HHmmSS: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
            break;
        }
        case OSD_DATE_FORMAT_YYMMDDHHmmSS: {
            nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d  %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            break;
        }
        case OSD_DATE_FORMAT_YYMMDDHHmmSSWW: {
            const wchar_t *wday[] = {L"星期天", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"};
            nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d  %02d:%02d:%02d  %ls", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, wday[tm.tm_wday]);
            break;
        }
        default: {
            nOutCharLen = 0;
            LOG_M_E(TIME, "Not supported date format: %d.", eDateFmt);
            return nullptr;
        }
    }

    return szOut;
}

time_t CElapsedTimer::StringToDatetime(std::string strYYYYMMDDHHMMSS) {
    tm tm_;
    int year, month, day, hour, minute, second;
    sscanf(strYYYYMMDDHHMMSS.c_str(), "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);
    tm_.tm_year = year - 1900;
    tm_.tm_mon = month - 1;
    tm_.tm_mday = day;
    tm_.tm_hour = hour;
    tm_.tm_min = minute;
    tm_.tm_sec = second;
    tm_.tm_isdst = 0;
    time_t t_ = mktime(&tm_);
    return t_;
}

std::pair<AX_U32, AX_U32> CElapsedTimer::GetDateTimeIntVal(time_t nSeconds) {
    AX_CHAR szDate[32] = {0};
    AX_CHAR szTime[32] = {0};
    struct tm* p = localtime(&nSeconds);
    sprintf(szDate, "%04d%02d%02d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday);
    sprintf(szTime, "%02d%02d%02d", p->tm_hour, p->tm_min, p->tm_sec);
    return make_pair<AX_U32, AX_U32>(atoi(szDate), atoi(szTime));
}

time_t CElapsedTimer::GetTimeTVal(AX_S32 nDate, AX_S32 nTime) {
    AX_CHAR szDateTime[64] = {0};
    sprintf(szDateTime, "%04d-%02d-%02d %02d:%02d:%02d", nDate / 10000, (nDate % 10000) / 100, nDate % 100, nTime / 10000, (nTime % 10000) / 100, nTime % 100);
    return StringToDatetime(szDateTime);
}