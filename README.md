# mtest

Test of latency for inter-process communication (pipes, shared memory, unix domain sockets).
The program measures half of the round-trip latency between a parent and its child process.
On each iteration a parent process creates a randomly generated set of data with a given size and sends it to its child process.
The child process receives the data and echo the data with the same size to its parent process.

**How to build**
```
cd <repository>
./build.sh
```

**How to run**
```
usage: mtest <parent_cpu> <child_cpu> [<nloops> <nwarmups> <transports>]
```

- _Run tests for all default type transports, pipes (*pipe*), shared memory (*shmem*), unix domain sockets (*udsocket*):_
```
./build/mtest 0 3
```

- _Run tests for a subset of transports_
```
./build/mtest 2 3 100000 1000 'pipe|shmem'
```

- _Run tests for custom transport(s) from external library:_
```
LD_PRELOAD=./build/libmtest_ext.so ./build/mtest 1 2 10000 1000 'shmem2|udsocket2'
```

For IRQs isolation via [tuna](https://manpages.ubuntu.com/manpages/focal/en/man8/tuna.8.html) out of working CPUs use the *run.sh* script:
```
./run.sh <parent_cpu> <child_cpu> [<nloops> <nwarmups> <transports>]
```

**How to install tuna**
```
sudo apt-get update
sudo apt-get -y install tuna
```
