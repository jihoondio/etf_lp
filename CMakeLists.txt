cmake_minimum_required(VERSION 3.28)
project(etf_lp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O3 -pthread)

add_executable(shm_producer protocol.h shm_producer.cpp)
add_executable(shm_consumer protocol.h shm_consumer.cpp)

add_executable(lf_producer protocol_lf.h lf_producer.cpp)
target_link_libraries(lf_producer PRIVATE rt pthread)

add_executable(lf_consumer protocol_lf.h lf_consumer.cpp)
target_link_libraries(lf_consumer PRIVATE rt pthread)
