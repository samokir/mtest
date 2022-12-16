#include "stat.h"

#include <algorithm>
#include <limits>
#include <cmath>
#include <iostream>

namespace mtest
{
    constexpr double get_percentile(const std::vector<uint64_t>& results, double p)
    {
        if (p < 0 || p > 100 || results.empty())
            return std::numeric_limits<double>::quiet_NaN();

        auto n = p * (results.size() + 1) / 100;
        auto n1 = std::floor(n);
        auto i = size_t(n1 - 1);
        if (i >= results.size() - 1)
            return results.back();

        auto k = n - n1;
        auto rI = results[i];
        auto rI1 = results[i + 1];

        return rI + k * (rI1 - rI);
    }
/*
    bool self_test()
    {
        auto res = get_percentile({3, 4, 4, 6, 7, 9, 12, 13, 14, 16, 17,
            19, 22, 23, 23, 25, 28, 29, 34, 37}, 25);
        std::cerr << "25% => " << res << std::endl;
        res = get_percentile({3, 4, 4, 6, 7, 9, 12, 13, 14, 16, 17,
            19, 22, 23, 23, 25, 28, 29, 34, 37}, 50);
        std::cerr << "50% => " << res << std::endl;
        res = get_percentile({3, 4, 4, 6, 7, 9, 12, 13, 14, 16, 17,
            19, 22, 23, 23, 25, 28, 29, 34, 37}, 75);
        std::cerr << "75% => " << res << std::endl;
        res = get_percentile({3, 4, 4, 6, 7, 9, 12, 13, 14, 16, 17,
            19, 22, 23, 23, 25, 28, 29, 34, 37}, 95);
        std::cerr << "95% => " << res << std::endl;
        res = get_percentile({3, 4, 4, 6, 7, 9, 12, 13, 14, 16, 17,
            19, 22, 23, 23, 25, 28, 29, 34, 37}, 0);
        std::cerr << "0% => " << res << std::endl;
        res = get_percentile({3, 4, 4, 6, 7, 9, 12, 13, 14, 16, 17,
            19, 22, 23, 23, 25, 28, 29, 34, 37}, 100);
        std::cerr << "100% => " << res << std::endl;
        return true;
    }

    static auto testres = self_test();
*/
    void get_stat(std::vector<uint64_t>& results, stat& out)
    {
        out = stat{};

        if (results.empty())
            return;

        std::sort(results.begin(), results.end());

        out.min = results.front();
        out.max = results.back();

        out.p50   = get_percentile(results, 50);
        out.p99   = get_percentile(results, 99);
        out.p999  = get_percentile(results, 99.9);
    }
}// namespace mtest