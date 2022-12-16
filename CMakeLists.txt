cmake_minimum_required(VERSION 3.25.1)
project (mtest)

add_compile_options(-O3 -ggdb -std=c++20 -Wextra)

add_library(mtest_transport SHARED
    transport.cpp
    pipe.cpp
    udsocket.cpp
    shmem.cpp
)

add_executable(mtest 
    main.cpp
    stat.cpp
)
target_link_libraries(mtest mtest_transport)

add_library(mtest_ext SHARED
    udsocket2.cpp
    shmem2.cpp
)
target_link_libraries(mtest_ext mtest_transport)
