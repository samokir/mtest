#pragma once

#include "transport.h"

namespace mtest
{
    class udsocket : public transport
    {
    public:
        udsocket();
        ~udsocket() override;

    public:// transport impl
        std::string get_name() const override { return "udsocket"; }
        void setup_reader() override;
        void setup_writer() override;
        void shutdown_reader() override;
        void shutdown_writer() override;
        int write(const void* data, size_t size) override;
        int read(void* data, size_t size) override;
    
    private:
        int fds[2] = {-1, -1};
    };
}// namespace mtest
