#include "shmem.h"

#include <string>
#include <cstring>
#include <stdexcept>
#include <atomic>
#include <thread>

#include <errno.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <immintrin.h> // For _mm_pause

namespace mtest
{
    static bool registered = register_transport("shmem", []{
        static int count = 0;
        auto name = "/mtest.shmem." + std::to_string(++count);
        return std::make_shared<shmem>(name, 1024*1024);
    });

    struct shmem::header
    {
        enum state { free, writing, ready };
        static_assert(std::atomic<state>::is_always_lock_free);

        std::atomic<state> dataState = ATOMIC_VAR_INIT(free);
        size_t dataSize = 0;

        void* data() { return reinterpret_cast<void*>(this + 1); }
        const void* data() const { return reinterpret_cast<const void*>(this + 1); }
    };

    shmem::shmem(const std::string& name, size_t maxSize)
    {
        if (maxSize == 0)
        {
            throw std::runtime_error("cannot create empty shmem");
        }

        fd = shm_open(name.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if (fd <= 0)
        {
            throw std::runtime_error(std::string("cannot create shmem: ") + std::strerror(errno));
        }

        auto mmapSize = maxSize + sizeof(header);

        if (ftruncate(fd, mmapSize) != 0)
        {
            throw std::runtime_error(std::string("cannot resize shmem: ") + std::strerror(errno));
        }

        auto data = mmap(NULL, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (data == MAP_FAILED)
        {
            throw std::runtime_error(std::string("cannot mmap shmem: ") + std::strerror(errno));
        }

        shm_unlink(name.c_str());

        headerPtr = new (data) header;
        headerPtr->dataSize = maxSize;
    }

    shmem::~shmem()
    {
        if (headerPtr != nullptr)
        {
            munmap(headerPtr, headerPtr->dataSize + sizeof(header));
        }
        if (fd > 0)
        {
            ::close(fd);
        }
    }

    void shmem::spin_wait_backoff(size_t currentCycle)
    {
        // hint for CPU it's a "spin-wait" loop
        _mm_pause();

        // switch to reader/writer thread if they are executed concurrently
        if (currentCycle % 10 == 0) std::this_thread::yield();
    }

    int shmem::write(const void* inData, size_t size)
    {
        if (size <= headerPtr->dataSize)[[likely]]
        {
            // enter in "writing" state
            header::state expected = header::free;
            for (size_t cycles = 1;
                !headerPtr->dataState.compare_exchange_strong(
                    expected, header::writing, std::memory_order_acq_rel);
                ++cycles)
            {
                expected = header::free;
                spin_wait_backoff(cycles);
            }

            std::memcpy(headerPtr->data(), inData, size);

            // mark shared memory as ready to read data
            headerPtr->dataState.store(header::ready, std::memory_order_release);
            return size;
        }
        return -1;
    }
    
    int shmem::read(void* outData, size_t size)
    {
        auto outSize = std::min(size, headerPtr->dataSize);

        // wait for data is ready to read (only single reader thread is allowed)
        for (size_t cycles = 1;
            headerPtr->dataState.load(std::memory_order_acquire) != header::ready;
            ++cycles)
        {
            spin_wait_backoff(cycles);
        }

        std::memcpy(outData, headerPtr->data(), outSize);

        // mark shared memory as ready for writing new data
        headerPtr->dataState.store(header::free, std::memory_order_release);
        return outSize;
    }
}// namespace mtest
