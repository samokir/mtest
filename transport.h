#pragma once

#include <cstdio>
#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace mtest
{
    class transport
    {
    public:
        virtual ~transport() = default;

        virtual std::string get_name() const = 0;

        virtual void setup_reader() = 0;
        virtual void setup_writer() = 0;
        virtual void shutdown_reader() = 0;
        virtual void shutdown_writer() = 0;

        virtual int write(const void* data, size_t size) = 0;
        virtual int read(void* data, size_t size) = 0;
    };

    using transport_ptr = std::shared_ptr<mtest::transport>;

    using transport_creator = std::function<transport_ptr()>;

    bool register_transport(const std::string& name, transport_creator&& creator);

    transport_creator get_transport_creator(const std::string& name);

    std::vector<transport_creator> get_transport_creators();
}//namespace mtest