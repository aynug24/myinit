cmake_minimum_required(VERSION 3.22)
project(untitled C)

set(CMAKE_C_STANDARD 17)

add_executable(myinit main.c list.c list.h daemon.c)

add_executable(fork_example fork_waitpid.c log/log.c log/log.h)
add_executable(deamon daemon.c log/log.c log/log.h)