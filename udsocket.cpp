#include "udsocket.h"

#include <string>
#include <cstring>
#include <stdexcept>

#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

namespace mtest
{
    static bool registered = register_transport("udsocket", 
        []{return std::make_shared<udsocket>();});

    udsocket::udsocket()
    {
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
        {
            throw std::runtime_error(std::string("cannot create udsocket: ") + std::strerror(errno));
        }
    }

    udsocket::~udsocket()
    {                
    }

    void udsocket::setup_reader()
    {
        ::close(fds[1]);
    }

    void udsocket::setup_writer()
    {
        ::close(fds[0]);
    }

    void udsocket::shutdown_reader()
    {
        ::close(fds[0]);
    }

    void udsocket::shutdown_writer()
    {
        ::close(fds[1]);
    }

    int udsocket::write(const void* data, size_t size)
    {
        return ::write(fds[1], data, size);
    }
    
    int udsocket::read(void* data, size_t size)
    {
        size_t rsize = 0;
        while (rsize < size)
        {
            auto res = ::read(fds[0], (char*)data + rsize, size - rsize);
            if (res <= 0)
                break;
            rsize += res;
        }
        return rsize;
    }
}// namespace mtest
