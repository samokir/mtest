#include "pipe.h"

#include <string>
#include <cstring>
#include <stdexcept>

#include <errno.h>
#include <unistd.h>

namespace mtest
{
    static bool registered = register_transport("pipe", 
        []{return std::make_shared<pipe>();});

    pipe::pipe()
    {
        if (::pipe(fds) != 0)
        {
            throw std::runtime_error(std::string("cannot create pipe: ") + std::strerror(errno));
        }
    }

    pipe::~pipe()
    {                
    }

    void pipe::setup_reader()
    {
        ::close(fds[1]);
    }

    void pipe::setup_writer()
    {
        ::close(fds[0]);
    }

    void pipe::shutdown_reader()
    {
        ::close(fds[0]);
    }

    void pipe::shutdown_writer()
    {
        ::close(fds[1]);
    }

    int pipe::write(const void* data, size_t size)
    {
        return ::write(fds[1], data, size);
    }
    
    int pipe::read(void* data, size_t size)
    {
        return ::read(fds[0], data, size);
    }
}// namespace mtest
