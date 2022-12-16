#pragma once

#include <vector>

#include <stdint.h>

namespace mtest
{
    struct stat
    {
        uint64_t min = 0;
        double p50 = 0;
        double p99 = 0;
        double p999= 0;
        uint64_t max = 0;
    };

    void get_stat(std::vector<uint64_t>& results, stat& out);
}// namespace mtest