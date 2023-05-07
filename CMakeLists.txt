cmake_minimum_required(VERSION 3.16)

project (Ticket-System)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(./src DIR_SRCS)

add_executable(code ${DIR_SRCS})