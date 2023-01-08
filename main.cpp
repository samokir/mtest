#include "transport.h"
#include "stat.h"

#include <iostream>
#include <iomanip>
#include <ratio>
#include <ranges>

#include <unistd.h> // fork
#include <sys/wait.h> // wait

#include <time.h> // clock_gettime

#include <sched.h> // sched_setaffinity

#include <immintrin.h> // _rdtsc

namespace mtest
{
    bool run_test(transport_creator tcreator,
        int parent_cpu, int child_cpu,
        size_t nloops, size_t nwarmups);
}// namespace mtest

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "usage: mtest <parent_cpu> <child_cpu> [<nloops> <nwarmups> <transports>]"  << std::endl;
        return 1;
    }

    auto parent_cpu = atoi(argv[1]);
    auto child_cpu = atoi(argv[2]);

    auto ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (parent_cpu < 0 || parent_cpu >= ncpus || ncpus < 0 || child_cpu >= ncpus)
    {
        std::cerr << "parent_cpu and child_cpu must be in range [0;" << (ncpus-1) << ']' << std::endl;
        return 1;
    }

    auto nloops = 10000;
    if (argc > 3 && (nloops = atoi(argv[3])) < 100)
    {
        std::cerr << "number of loops must be >= 100" << std::endl;
        return 1;
    }

    auto nwarmups = nloops / 5; // 20%
    if (argc > 4 && (nwarmups = atoi(argv[4])) < 0)
    {
        std::cerr << "number of warmup loops must be >= 0" << std::endl;
        return 1;
    }

    std::vector<mtest::transport_creator> tcreators;
    if (argc > 5)
    {
        std::stringstream ss(argv[5]);
        std::string name;
        while (std::getline(ss, name, '|'))
        {
            if (name == "all")
            {
                tcreators = mtest::get_transport_creators(); 
                break;
            }
            auto t = mtest::get_transport_creator(name);
            if (t == nullptr)
            {
                std::cerr << "transport " << name << " is not registered" << std::endl;
                continue;
            }
            tcreators.emplace_back(t);
        }
    }
    else
    {
        tcreators = mtest::get_transport_creators();        
    }
    if (tcreators.empty())
    {
        std::cerr << "no transports to start mtest" << std::endl;
        return 1;
    }

    std::cout << "mtest: test cycles: " << nloops 
        << ", warmup cycles: " << nwarmups
        << ", CPUs: " << parent_cpu << "<->" << child_cpu << std::endl;

    for (auto c : tcreators)
    {
        mtest::run_test(c, parent_cpu, child_cpu, nloops, nwarmups);
    }

    return 0;
}

namespace mtest
{
    void exec_child(transport_ptr tRead, transport_ptr tWrite, size_t nloops, size_t nwarmups);
    void exec_parent(transport_ptr tRead, transport_ptr tWrite, size_t nloops, size_t nwarmups);

    bool run_test(transport_creator tcreator,
        int parent_cpu, int child_cpu,
        size_t nloops, size_t nwarmups)
    {
        mtest::transport_ptr t1, t2;
        try
        {
            t1 = tcreator();
            t2 = tcreator();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return false;
        }
        
        std::cout << std::endl << "running mtest for transport '" 
            << t1->get_name() << "'..." << std::endl << std::flush;

        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);

        auto pid = fork();
        if (pid == -1)
        {
            std::cerr << "fork error" << std::endl;
            return 1;
        }
        else if (pid == 0)
        {
            CPU_SET(child_cpu, &cpu_set);
            if (sched_setaffinity(getpid(), sizeof(cpu_set), &cpu_set) == -1)
            {
                std::cerr << "Child: cannot set CPU affinity" << std::endl;
                return 1;
            }
            exec_child(t1, t2, nloops, nwarmups);
            exit(0);
        }
        else
        {
            CPU_SET(parent_cpu, &cpu_set);
            if (sched_setaffinity(getpid(), sizeof(cpu_set), &cpu_set) == -1)
            {
                std::cerr << "Parent: cannot set CPU affinity" << std::endl;
                return 1;
            }
            exec_parent(t2, t1, nloops, nwarmups);
            ::wait(nullptr);
        }

        return true;
    }

    constexpr bool _log_debug = false;
#define LOG_DEBUG (!_log_debug) ? std::cout : std::cout

    std::ostream& operator<<(std::ostream& s, const std::vector<int>& v)
    {
        s << '{';
        size_t i = 0;
        for (; i <= 4 && i < v.size()-1; ++i)
            s << v[i] << ',';
        if (i < v.size() - 1)
            s << "...,";
        s << v.back();
        s << '}';
        return s;
    }

    uint64_t gettime_ns()
    {
        return __rdtsc();
    }

    constexpr std::array DATA_SIZE_TO_TEST = {4, 16, 64, 256, 1024, 4096, 16384, 65536};

    void exec_childImpl(size_t dataSize, transport_ptr tRead, transport_ptr tWrite, size_t nloops, size_t nwarmups)
    {
        std::vector<int> data(dataSize / sizeof(int));
        dataSize = data.size() * sizeof(int);

        for (size_t i = 0; i < (nloops + nwarmups); ++i)
        {
            int rres = tRead->read(data.data(), dataSize);
            LOG_DEBUG << "Child[" << i << "]: read " << data << " res=" << rres << std::endl;

            // echo data to the parent process
            auto wres = tWrite->write(data.data(), dataSize);
            LOG_DEBUG << "Child[" << i << "]: write " << wres << std::endl;

            if (wres != rres)
            {
                std::cerr << "Child[" << i << "]: write size != read size" << std::endl;
                std::cerr << "Child[" << i << "]: " << wres << " != " << rres << std::endl;
            }
        }
    }

    void exec_child(transport_ptr tRead, transport_ptr tWrite, size_t nloops, size_t nwarmups)
    {
        tRead->setup_reader();
        tWrite->setup_writer();

        for (auto dataSize : DATA_SIZE_TO_TEST)
        {
            LOG_DEBUG << "Child: testing dataSize=" << dataSize << std::endl;
            exec_childImpl(dataSize, tRead, tWrite, nloops, nwarmups);
        }

        tWrite->shutdown_writer();
        tRead->shutdown_reader();
    }

    double timeDiffK = 0.0;

    void exec_parentImpl(size_t dataSize, transport_ptr tRead, transport_ptr tWrite, size_t nloops, size_t nwarmups)
    {
        std::vector<int> outData(dataSize / sizeof(int));
        dataSize = outData.size() * sizeof(int);
        std::vector<int> inData;

        std::vector<uint64_t> results(nloops);
        for (size_t i = 0; i < (nloops + nwarmups); ++i)
        {
            inData.clear();
            inData.resize(outData.size());
            for (auto& e : outData)
            {
                e = std::rand();
            }

            auto start = gettime_ns();
            int wres = tWrite->write(outData.data(), dataSize);
            LOG_DEBUG << "Parent[" << i << "]: write " << outData << " res=" << wres << std::endl;

            auto rres = tRead->read(inData.data(), dataSize);
            auto finish = gettime_ns();
            LOG_DEBUG << "Parent[" << i << "]: read " << inData << " res=" << rres 
                << " tm=" << (finish-start) << "ns" << std::endl;

            if (wres != rres)
            {
                std::cerr << "Parent[" << i << "]: write size != read size" << std::endl;
                std::cerr << "Parent[" << i << "]: " << wres << " != " << rres << std::endl;
            }
            if (inData != outData)
            {
                std::cerr << "Parent[" << i << "]: write data != read data" << std::endl;
                std::cerr << "Parent[" << i << "]: " << outData << " != " << inData << std::endl;
            }

            if (i >= nwarmups)
            {
                results[i-nwarmups] = uint64_t(timeDiffK*(finish-start)/2);// half of the round-trip
            }
        }
        
        stat st;
        get_stat(results, st);
        
        std::cout.fill(' ');
        std::cout << std::fixed;
        std::cout.precision(1);

        std::cout << "| " << std::setw(6) << dataSize << '|';
        std::cout << std::setw(11) << st.min << '|';
        std::cout << std::setw(11) << st.p50 << '|';
        std::cout << std::setw(11) << st.p95 << '|';
        std::cout << std::setw(11) << st.p99 << '|';
        std::cout << std::setw(11) << st.p999 << '|';
        std::cout << std::setw(11) << st.max << '|';
        std::cout << std::endl;
    }

    void exec_parent(transport_ptr tRead, transport_ptr tWrite, size_t nloops, size_t nwarmups)
    {
        tWrite->setup_writer();
        tRead->setup_reader();

        uint64_t t1, t2;
        timespec ts = {1, 0};
        do
        {
            t1 = gettime_ns();
        }
        while (nanosleep(&ts, nullptr) != 0);
        t2 = gettime_ns();
        auto tdiff = (t2 > t1) ? (t2 - t1) : (t1 - t2);
        std::cout << "CPU frequency " << (tdiff/1000000) << "MHz" << std::endl;
        timeDiffK = 1000000000.0/tdiff;

        std::cout << "------------------------------------------------------------------------------_--" << std::endl;
        std::cout << "| bytes |   min(us) |   50%(us) |   95%(us) |   99%(us) | 99.9%(us) |   max(us) |" << std::endl;
        std::cout << "---------------------------------------------------------------------------------" << std::endl;

        for (auto dataSize : DATA_SIZE_TO_TEST)
        {
            LOG_DEBUG << "Parent: testing dataSize=" << dataSize << std::endl;
            exec_parentImpl(dataSize, tRead, tWrite, nloops, nwarmups);
        }

        std::cout << "---------------------------------------------------------------------------------" << std::endl;

        tWrite->shutdown_writer();
        tRead->shutdown_reader();
    }
}// namespace mtest
