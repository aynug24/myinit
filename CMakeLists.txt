cmake_minimum_required(VERSION 3.22)
project(untitled C)

set(CMAKE_C_STANDARD 17)


add_executable(sigtest sigtest.c)

add_executable(myinit main.c
        log/log.c log/log.h
        parse/parse_myinit_args.c parse/parse_myinit_args.h
        parse/parse_config.c parse/parse_config.h
        task/task.c task/task.h lists/add.c lists/add.h run_daemon/run_daemon.c run_daemon/run_daemon.h)

file(
    COPY
    config
    DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ
)