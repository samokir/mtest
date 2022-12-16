#pragma once

#include "transport.h"

namespace mtest
{
    class shmem2 final : public transport
    {
    public:
        shmem2(size_t maxSize);
        ~shmem2() override;

    public:// transport impl
        std::string get_name() const override { return "shmem2"; }

        void setup_reader() override {}
        void setup_writer() override {}
        void shutdown_reader() override {}
        void shutdown_writer() override {}
        int write(const void* data, size_t size) override;
        int read(void* data, size_t size) override;
    
    private:
        struct header;
        header* headerPtr = nullptr;
    };
}// namespace mtest
