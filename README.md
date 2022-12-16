# mtest

Test of latency for inter-process communication (pipes, shared memory, unix domain sockets).
The program measures half of the round-trip latency between a parent and its child process.

**How to build**
cd <repository>
./build.sh

**How to run**
*usage: mtest <parent_cpu> <child_cpu> \[<nloops> <nwarmups> <transports>\]*

_Run tests for all default type transports, pipes (*pipe*), shared memory (*shmem*), unix domain sockets (*udsocket*):_
./build/mtest 0 3

_Run tests for a subset of transports_
./build/mtest 2 3 100000 1000 'pipe|shmem'

_Run tests for custom transport(s) from external library:_
LD_PRELOAD=./build/libmtest_ext.so ./build/mtest 1 2 10000 1000 'shmem2|udsocket2'
