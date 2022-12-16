#include "udsocket2.h"

#include <string>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <iostream>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace mtest
{
    static bool registered = register_transport("udsocket2", []{
        static int count = 0;
        auto name = "udsocket2." + std::to_string(++count);
        return std::make_shared<udsocket2>(name);
    });

    udsocket2::udsocket2(const std::string& fname) : file_name(fname)
    {
    }

    udsocket2::~udsocket2()
    {
        unlink(file_name.c_str());
    }

    void udsocket2::setup_reader()
    {
        server_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            throw std::runtime_error(std::string("cannot create udsocket2: ") + std::strerror(errno));
        }

        struct sockaddr_un saddr;
        saddr.sun_family = AF_UNIX;
        std::strncpy(saddr.sun_path, file_name.c_str(), sizeof(saddr.sun_path)-1);
        unlink(file_name.c_str());

        for (bool bound = false;;)
        {
            using namespace std::chrono_literals;

            auto rc = bound ? 0 : bind(server_fd, (struct sockaddr *)&saddr, sizeof(saddr));
            if (rc == -1)
            {
                std::cerr << "cannot bind socket " << file_name
                    << ": " << std::strerror(errno) << ", fd=" << server_fd << std::endl;
                std::this_thread::sleep_for(1s);
                continue;
            }
            bound = true;

            rc = listen(server_fd, 1);
            if (rc == -1)
            {
                std::cerr << "cannot listen to socket " << file_name
                    << ": " << std::strerror(errno) << std::endl;
                std::this_thread::sleep_for(1s);
                continue;
            }

            socklen_t len = sizeof(saddr);
            client_fd = accept(server_fd, (struct sockaddr *)&saddr, &len);
            if (client_fd != -1)
                break;

            std::cerr << "cannot listen socket " << file_name 
                << ": " << std::strerror(errno) << std::endl;
            std::this_thread::sleep_for(1s);
        }
    }

    void udsocket2::setup_writer()
    {
        client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_fd < 0)
        {
            throw std::runtime_error(std::string("cannot create udsocket2: ") + std::strerror(errno));
        }

        struct sockaddr_un saddr;
        saddr.sun_family = AF_UNIX;
        std::strncpy(saddr.sun_path, file_name.c_str(), sizeof(saddr.sun_path)-1);

        for (bool bound = false;;)
        {
            using namespace std::chrono_literals;
            auto rc = connect(client_fd, (struct sockaddr *)&saddr, sizeof(saddr));
            if(rc != -1)
                break;
            std::cerr << "cannot connect to socket " << file_name
                << ": " << std::strerror(errno) << " " << errno << std::endl;
            std::this_thread::sleep_for(1s);
        }        
    }

    void udsocket2::shutdown_reader()
    {
        ::close(client_fd);
        ::close(server_fd);
    }

    void udsocket2::shutdown_writer()
    {
        ::close(client_fd);
    }

    int udsocket2::write(const void* data, size_t size)
    {
        return ::write(client_fd, data, size);
    }
    
    int udsocket2::read(void* data, size_t size)
    {
        size_t rsize = 0;
        while (rsize < size)
        {
            auto res = ::read(client_fd, (char*)data + rsize, size - rsize);
            if (res <= 0)
                break;
            rsize += res;
        }
        return rsize;
    }
}// namespace mtest
