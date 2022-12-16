#pragma once

#include "transport.h"

#include <string>

namespace mtest
{
    class udsocket2 : public transport
    {
    public:
        udsocket2(const std::string& file_name);
        ~udsocket2() override;

    public:// transport impl
        std::string get_name() const override { return "udsocket2"; }
        void setup_reader() override;
        void setup_writer() override;
        void shutdown_reader() override;
        void shutdown_writer() override;
        int write(const void* data, size_t size) override;
        int read(void* data, size_t size) override;
    
    private:
        const std::string file_name;
        int client_fd = -1;
        int server_fd = -1;
    };
}// namespace mtest
