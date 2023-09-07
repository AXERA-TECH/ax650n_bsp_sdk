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
#include <string.h>
#include <exception>
#include <string>

class CAXException : public std::exception {
public:
    template <typename... Args>
    CAXException(const char *fmt, Args... args) noexcept {
        size_t sz = 1 + snprintf(nullptr, 0, fmt, args...);  // Extra space for \0
        if (sz <= 1) {
            m_sWhat = "unknown exception";
        } else {
            m_sWhat.resize(sz);
            snprintf(&m_sWhat[0], sz, fmt, args...);
        }
    }

    const char *what(void) const noexcept {
        return m_sWhat.c_str();
    }

private:
    std::string m_sWhat;
};

#define THROW_AX_EXCEPTION(fmt, ...) throw CAXException("catch an exception from file: %s, func: %s, line: %d\n ==> " fmt, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);