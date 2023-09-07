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

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

namespace utilities
{
    bool file_exist(const std::string& path)
    {
        auto flag = false;

        std::fstream fs(path, std::ios::in | std::ios::binary);
        flag = fs.is_open();
        fs.close();

        return flag;
    }

    bool read_file(const std::string& path, std::vector<char>& data)
    {
        std::fstream fs(path, std::ios::in | std::ios::binary);

        if (!fs.is_open())
        {
            return false;
        }

        fs.seekg(std::ios::end);
        auto fs_end = fs.tellg();
        fs.seekg(std::ios::beg);
        auto fs_beg = fs.tellg();

        auto file_size = static_cast<size_t>(fs_end - fs_beg);
        auto vector_size = data.size();

        data.reserve(vector_size + file_size);
        data.insert(data.end(), std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());

        fs.close();

        return true;
    }

    bool dump_file(const std::string& path, std::vector<uint8_t> data)
    {
        std::fstream fs(path, std::ios::out | std::ios::binary);

        if (!fs.is_open() || fs.fail())
        {
            fprintf(stderr, "[ERR] cannot open file %s \n", path.c_str());
        }

        fs.write((char*)data.data(), data.size());

        return true;
    }

    bool dump_file(const std::string& path, char* data, int size)
    {
        std::fstream fs(path, std::ios::out | std::ios::binary);

        if (!fs.is_open() || fs.fail())
        {
            fprintf(stderr, "[ERR] cannot open file %s \n", path.c_str());
        }

        fs.write(data, size);

        return true;
    }

    bool read_file(const char* fn, std::vector<uchar>& data)
    {
        FILE* fp = fopen(fn, "r");
        if (fp != nullptr)
        {
            fseek(fp, 0L, SEEK_END);
            auto len = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            data.clear();
            size_t read_size = 0;
            if (len > 0)
            {
                data.resize(len);
                read_size = fread(data.data(), 1, len, fp);
            }
            fclose(fp);
            return read_size == (size_t)len;
        }
        return false;
    }

} // namespace utilities
