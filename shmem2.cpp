#include "shmem2.h"

#include <string>
#include <cstring>
#include <stdexcept>
#include <atomic>
#include <thread>

#include <errno.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/ipc.h>

#include <semaphore.h>

namespace mtest
{
    static bool registered = register_transport("shmem2", []{
        return std::make_shared<shmem2>(1024*1024);
    });

    struct shmem2::header
    {
        sem_t sem = {};
        size_t dataSize = 0;

        void* data() { return reinterpret_cast<void*>(this + 1); }
        const void* data() const { return reinterpret_cast<const void*>(this + 1); }
    };

    shmem2::shmem2(size_t maxSize)
    {
        if (maxSize == 0)
        {
            throw std::runtime_error("cannot create empty shmem2");
        }

        auto mmapSize = maxSize + sizeof(header);

        auto shmid = shmget(IPC_PRIVATE, mmapSize, IPC_CREAT|0600);
        if (shmid == -1)
        {
            throw std::runtime_error(std::string("cannot shmget: ") + std::strerror(errno));
        }

        auto data = shmat(shmid, NULL, 0);
        if (data == (void*)-1)
        {
            throw std::runtime_error(std::string("cannot shmat: ") + std::strerror(errno));
        }

        headerPtr = new (data) header;
        headerPtr->dataSize = maxSize;
        if (sem_init(&headerPtr->sem, 1, 0) != 0)
        {
            throw std::runtime_error(std::string("cannot init semaphor: ") + std::strerror(errno));
        }
    }

    shmem2::~shmem2()
    {
        if (headerPtr != nullptr)
        {
            sem_destroy(&headerPtr->sem);
            shmdt(headerPtr);
        }
    }

    int shmem2::write(const void* inData, size_t size)
    {
        if (size <= headerPtr->dataSize)[[likely]]
        {
            std::memcpy(headerPtr->data(), inData, size);

            // mark shared memory as ready to read data
            sem_post(&headerPtr->sem);
            return size;
        }
        return -1;
    }
    
    int shmem2::read(void* outData, size_t size)
    {
        auto outSize = std::min(size, headerPtr->dataSize);

        // wait for data is ready to read (only single reader thread is allowed)
        sem_wait(&headerPtr->sem);

        std::memcpy(outData, headerPtr->data(), outSize);
        return outSize;
    }
}// namespace mtest
