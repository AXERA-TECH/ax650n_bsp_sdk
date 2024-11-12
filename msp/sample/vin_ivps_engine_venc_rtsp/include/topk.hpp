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

#include <algorithm>
#include <cstdio>
#include <vector>

#include "score.hpp"


namespace classification
{
    void sort_score(std::vector<score>& array, bool reverse = false)
    {
        auto compare_func = [](const score& a, const score& b) -> bool
        {
            return a.score > b.score;
        };

        std::sort(array.begin(), array.end(), compare_func);

        if (reverse) std::reverse(array.begin(), array.end());
    }


    void print_score(const std::vector<score>& array, const size_t& n)
    {
        for (size_t i = 0; i < n; i++)
        {
            fprintf(stdout, "%.4f, %d\n", array[i].score, array[i].id);
        }
    }
}
