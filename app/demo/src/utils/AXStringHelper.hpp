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
#include <string.h>
#include <exception>
#include <string>
#include <vector>
#include "ax_base_type.h"

class CAXStringHelper {
public:
    template <typename... Args>
    static std::string Format(const char* fmt, Args... args) {
        auto sz = 1 + snprintf(nullptr, 0, fmt, args...);  // Extra space for \0
        if (sz <= 1) {
            return "";
        }

        char* buf = new (std::nothrow) char[sz];
        if (!buf) {
            return "";
        }

        snprintf(buf, sz, fmt, args...);
        std::string str(buf);
        delete[] buf;
        return str;
    }

    static AX_VOID Split(std::vector<std::string>& tokens, const std::string& text, const std::string delimiters = ",") {
        std::string::size_type lastPos = text.find_first_not_of(delimiters, 0);
        std::string::size_type pos = text.find_first_of(delimiters, lastPos);
        while (std::string::npos != pos || std::string::npos != lastPos) {
            tokens.emplace_back(text.substr(lastPos, pos - lastPos));
            lastPos = text.find_first_not_of(delimiters, pos);
            pos = text.find_first_of(delimiters, lastPos);
        }
    }

private:
    CAXStringHelper(AX_VOID) = delete;
};
