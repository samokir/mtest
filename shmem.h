#pragma once

#include "transport.h"

namespace mtest
{
    class shmem final : public transport
    {
    public:
        shmem(const std::string& name, size_t maxSize);
        ~shmem() override;

    public:// transport impl
        std::string get_name() const override { return "shmem"; }

        void setup_reader() override {}
        void setup_writer() override {}
        void shutdown_reader() override {}
        void shutdown_writer() override {}
        int write(const void* data, size_t size) override;
        int read(void* data, size_t size) override;

    private:
        void spin_wait_backoff(size_t currentCycle);
    
    private:
        int fd = -1;
        struct header;
        header* headerPtr = nullptr;
    };
}// namespace mtest
