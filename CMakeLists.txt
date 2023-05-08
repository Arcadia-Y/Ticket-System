cmake_minimum_required(VERSION 3.16)

project (Ticket-System)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE "release")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

aux_source_directory(./src DIR_SRCS)

add_executable(code ${DIR_SRCS})