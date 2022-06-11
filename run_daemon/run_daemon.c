//
// Created by sergei on 11.06.22.
//

#include <stddef.h>
#include <unistd.h>
#include "run_daemon.h"
#include "../parse/parse_config.h"
#include "../parse/parse_myinit_args.h"
#include "../log/log.h"

InitTasks tasks;

int run_daemon(int argc, char* argv[]) {
    log_info("Running myinit");

    char* config_path = parse_config_path(argc, argv);
    if (config_path == NULL) {
        log_crit("Couldn't get config path");
        return -1;
    }
    log_info("Successfully got config path");

    if (parse_config(config_path, &tasks) < 0) {
        log_crit("Error parsing config");
        return -1;
    }
    log_info("Successfully parsed config file");

    sleep(30);
    log_info("Successfully slept!");
    log_info("Shutting down...");
    return 0;
}