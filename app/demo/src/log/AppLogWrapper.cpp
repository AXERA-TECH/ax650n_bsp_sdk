/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AppLogWrapper.hpp"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <exception>
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

/*
03-25 22:29:30:263  56396 E: This is a test
03-25 22:29:30:263  56396 N tag: This is a test
*/
#define VK_SPACE 0x20
#define LINE_MAXCHAR 16
#define MAX_LINES 16
#define TIME_PREFIX 18
#define TID_PREFIX 6
#define MAX_PREFIX 48
#define MAX_LOG_STR 1024
static constexpr AX_U32 MAX_LOG_BUFF = MAX_LOG_STR + MAX_PREFIX;

static const AX_CHAR* DIRECT_PREFIX[APP_LOG_FLAG_BUTT] = {"-->", "<--", "->>", "<<-"};

static const AX_CHAR* HEX_TABLE[256] = {
    "00 ", "01 ", "02 ", "03 ", "04 ", "05 ", "06 ", "07 ", "08 ", "09 ", "0A ", "0B ", "0C ", "0D ", "0E ", "0F ",
    "10 ", "11 ", "12 ", "13 ", "14 ", "15 ", "16 ", "17 ", "18 ", "19 ", "1A ", "1B ", "1C ", "1D ", "1E ", "1F ",
    "20 ", "21 ", "22 ", "23 ", "24 ", "25 ", "26 ", "27 ", "28 ", "29 ", "2A ", "2B ", "2C ", "2D ", "2E ", "2F ",
    "30 ", "31 ", "32 ", "33 ", "34 ", "35 ", "36 ", "37 ", "38 ", "39 ", "3A ", "3B ", "3C ", "3D ", "3E ", "3F ",
    "40 ", "41 ", "42 ", "43 ", "44 ", "45 ", "46 ", "47 ", "48 ", "49 ", "4A ", "4B ", "4C ", "4D ", "4E ", "4F ",
    "50 ", "51 ", "52 ", "53 ", "54 ", "55 ", "56 ", "57 ", "58 ", "59 ", "5A ", "5B ", "5C ", "5D ", "5E ", "5F ",
    "60 ", "61 ", "62 ", "63 ", "64 ", "65 ", "66 ", "67 ", "68 ", "69 ", "6A ", "6B ", "6C ", "6D ", "6E ", "6F ",
    "70 ", "71 ", "72 ", "73 ", "74 ", "75 ", "76 ", "77 ", "78 ", "79 ", "7A ", "7B ", "7C ", "7D ", "7E ", "7F ",
    "80 ", "81 ", "82 ", "83 ", "84 ", "85 ", "86 ", "87 ", "88 ", "89 ", "8A ", "8B ", "8C ", "8D ", "8E ", "8F ",
    "90 ", "91 ", "92 ", "93 ", "94 ", "95 ", "96 ", "97 ", "98 ", "99 ", "9A ", "9B ", "9C ", "9D ", "9E ", "9F ",
    "A0 ", "A1 ", "A2 ", "A3 ", "A4 ", "A5 ", "A6 ", "A7 ", "A8 ", "A9 ", "AA ", "AB ", "AC ", "AD ", "AE ", "AF ",
    "B0 ", "B1 ", "B2 ", "B3 ", "B4 ", "B5 ", "B6 ", "B7 ", "B8 ", "B9 ", "BA ", "BB ", "BC ", "BD ", "BE ", "BF ",
    "C0 ", "C1 ", "C2 ", "C3 ", "C4 ", "C5 ", "C6 ", "C7 ", "C8 ", "C9 ", "CA ", "CB ", "CC ", "CD ", "CE ", "CF ",
    "D0 ", "D1 ", "D2 ", "D3 ", "D4 ", "D5 ", "D6 ", "D7 ", "D8 ", "D9 ", "DA ", "DB ", "DC ", "DD ", "DE ", "DF ",
    "E0 ", "E1 ", "E2 ", "E3 ", "E4 ", "E5 ", "E6 ", "E7 ", "E8 ", "E9 ", "EA ", "EB ", "EC ", "ED ", "EE ", "EF ",
    "F0 ", "F1 ", "F2 ", "F3 ", "F4 ", "F5 ", "F6 ", "F7 ", "F8 ", "F9 ", "FA ", "FB ", "FC ", "FD ", "FE ", "FF ",
};

#define SAFE_RELEASE_LOGGER(p) \
    do {                       \
        if (p) {               \
            p->Close();        \
            delete p;          \
            p = nullptr;       \
        }                      \
    } while (0)

AX_S32 CAppLogWrapper::Init(const APP_LOG_ATTR_T* pstAttr) {
    if (!pstAttr) {
        return -1;
    }

    do {
        if (pstAttr->nTarget & APP_LOG_TARGET_APPLOG) {
            m_pAppLog = new (std::nothrow) CAppLog();
            if (!m_pAppLog) {
                break;
            }

            if (!m_pAppLog->Open(*pstAttr)) {
                break;
            }
        }

        if (pstAttr->nTarget & APP_LOG_TARGET_STDOUT) {
            m_pStdLog = new (std::nothrow) CStdLog();
            if (!m_pStdLog) {
                break;
            }

            if (!m_pStdLog->Open(*pstAttr)) {
                break;
            }
        }

        if (pstAttr->nTarget & APP_LOG_TARGET_SYSLOG) {
            m_pSysLog = new (std::nothrow) CSysLog();
            if (!m_pSysLog) {
                break;
            }

            if (!m_pSysLog->Open(*pstAttr)) {
                break;
            }
        }

        m_stAttr = *pstAttr;
        return 0;

    } while (0);

    SAFE_RELEASE_LOGGER(m_pAppLog);
    SAFE_RELEASE_LOGGER(m_pStdLog);
    SAFE_RELEASE_LOGGER(m_pSysLog);

    return -1;
}

AX_VOID CAppLogWrapper::DeInit(AX_VOID) {
    SAFE_RELEASE_LOGGER(m_pAppLog);
    SAFE_RELEASE_LOGGER(m_pStdLog);
    SAFE_RELEASE_LOGGER(m_pSysLog);
}

AX_U64 CAppLogWrapper::GetTickCount(AX_VOID) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

AX_VOID CAppLogWrapper::LogArgStr(AX_S32 nLv, const AX_CHAR* pFmt, va_list vlist) {
    if (nLv > m_stAttr.nLv) {
        return;
    }

    AX_CHAR szBuf[MAX_LOG_STR] = {0};
    AX_S32 nLen = vsnprintf(szBuf, sizeof(szBuf), pFmt, vlist);
    if (nLen > 0) {
        AX_CHAR szLog[MAX_LOG_BUFF] = {0};
        timeval tv;
        struct tm t;
        gettimeofday(&tv, NULL);
        time_t now = time(NULL);
        localtime_r(&now, &t);
#if 1
        snprintf(szLog, sizeof(szLog), "%02u-%02u %02u:%02u:%02u:%03u %lld %6lu %s", t.tm_mon + 1, t.tm_mday, t.tm_hour,
                 t.tm_min, t.tm_sec, (AX_U32)(tv.tv_usec / 1000), GetTickCount(), (long unsigned int)gettid(), szBuf);
#else
        snprintf(szLog, sizeof(szLog), "[%lu.%06lu] %6lu %s", tv.tv_sec, tv.tv_usec, (long unsigned int)gettid(), szBuf);
#endif
        Logging(nLv, szLog);
    }
}

AX_VOID CAppLogWrapper::LogFmtStr(AX_S32 nLv, const AX_CHAR* pFmt, ...) {
    if (nLv > m_stAttr.nLv) {
        return;
    }

    va_list args;
    va_start(args, pFmt);
    LogArgStr(nLv, pFmt, args);
    va_end(args);
}

AX_VOID CAppLogWrapper::LogBufData(AX_S32 nLv, const AX_VOID* pBuf, AX_U32 nBufSize, AX_U32 nFlag) {
    if (nLv > m_stAttr.nLv) {
        return;
    }

    /*
    03-26 10:15:16:373   9605 --> 1024(0x00000400) Bytes
                          00000000h: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ................
                          00000010h: 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F  ................
                          00000020h: 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F   !"#$%&'()*+,-./
                          00000030h: 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F  0123456789:;<=>?
                          00000040h: 40 41 42 43 44 45 46 47 48 49 4A 4B 4C 4D 4E 4F  @ABCDEFGHIJKLMNO
                          00000050h: 50 51 52 53 54 55 56 57 58 59 5A 5B 5C 5D 5E 5F  PQRSTUVWXYZ[\]^_
                          00000060h: 60 61 62 63 64 65 66 67 68 69 6A 6B 6C 6D 6E 6F  `abcdefghijklmno
                          00000070h: 70 71 72 73 74 75 76 77 78 79 7A 7B 7C 7D 7E 7F  pqrstuvwxyz{|}~.
                          00000080h: 80 81 82 83 84 85 86 87 88 89 8A 8B 8C 8D 8E 8F  ................
                          00000090h: 90 91 92 93 94 95 96 97 98 99 9A 9B 9C 9D 9E 9F  ................
                          000000a0h: A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF  ................
                          000000b0h: B0 B1 B2 B3 B4 B5 B6 B7 B8 B9 BA BB BC BD BE BF  ................
                          000000c0h: C0 C1 C2 C3 C4 C5 C6 C7 C8 C9 CA CB CC CD CE CF  ................
                          000000d0h: D0 D1 D2 D3 D4 D5 D6 D7 D8 D9 DA DB DC DD DE DF  ................
                          000000e0h: E0 E1 E2 E3 E4 E5 E6 E7 E8 E9 EA EB EC ED EE EF  ................
                          000000f0h: F0 F1 F2 F3 F4 F5 F6 F7 F8 F9 FA FB FC FD FE 00  ................
                          00000100h: 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10  ................
                          000003f0h: ... omitted 48 lines ...

    */

    LogFmtStr(nLv, "%s %d(0x%08X) Bytes\n", DIRECT_PREFIX[(nFlag >= (AX_U32)APP_LOG_FLAG_BUTT) ? (AX_U32)APP_LOG_SYNC_SEND : nFlag],
              nBufSize, nBufSize);

    constexpr AX_U32 HEAD_PREFIX = TIME_PREFIX + 1 + TID_PREFIX + 1; /* 03-26 10:15:16:373   9605 */
    constexpr AX_U32 SIZE_PREFIX = 11;                               /* 00000010h:  */
    constexpr AX_U32 ASCII_OFFSET = HEAD_PREFIX + SIZE_PREFIX + LINE_MAXCHAR * 3 + 1 /* ; */;
    constexpr AX_U32 LINE_CHARS = ASCII_OFFSET + LINE_MAXCHAR;
    AX_CHAR szLine[LINE_CHARS + 3];

    /* set end chars */
    szLine[LINE_CHARS] = '\n';
    szLine[LINE_CHARS + 1] = '\0';

    const AX_U32 LINES = (nBufSize / LINE_MAXCHAR) + ((nBufSize % LINE_MAXCHAR) ? 1 : 0);
    AX_U32 nCharNum = 0;
    AX_U8* pChar = (AX_U8*)pBuf;
    for (AX_U32 i = 0; i < LINES; ++i) {
        AX_CHAR* p = &szLine[0];
        memset(p, VK_SPACE, LINE_CHARS);
        p += HEAD_PREFIX;

        if (i > MAX_LINES) {
            sprintf(p, "%08xh: ... omitted %d lines ...\n", (LINES - 1) * LINE_MAXCHAR, LINES - MAX_LINES);
            Logging(nLv, szLine);
            break;
        }

        sprintf(p, "%08xh: ", i * LINE_MAXCHAR);
        p += SIZE_PREFIX;
        for (AX_U32 j = 0; j < LINE_MAXCHAR; ++j) {
            if (nCharNum < nBufSize) {
                memcpy(p, HEX_TABLE[*pChar], 3);
                szLine[ASCII_OFFSET + j] = isprint(*pChar) ? *pChar : '.';
                ++pChar;
                ++nCharNum;
            } else {
                szLine[ASCII_OFFSET + j] = '.';
            }
            p += 3;
        }

        Logging(nLv, szLine);
    }
}
