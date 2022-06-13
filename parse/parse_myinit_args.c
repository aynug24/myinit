#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <malloc.h>
#include <errno.h>
#include <ctype.h>
#include "../log/log.h"
#include "parse_myinit_args.h"

char* parse_config_path(int argc, char* argv[]) {
    char* config_path;
    int argname;
    opterr = 0;

    while ((argname = getopt(argc, argv, "c:")) != -1) {
        switch (argname) {
            case 'c':
                config_path = malloc(strlen(optarg) + 1);
                if (config_path == NULL) {
                    log_crit_e("Couldn't allocate memory for config path", errno);
                    return NULL;
                }

                strcpy(config_path, optarg);
                return config_path;
            case '?':
                if (optopt == 'c') {
                    log_crit("Config option -c didn't have argument");
                } else if (isprint(optopt)) {
                    log_crit("Unknown option");
                } else {
                    log_crit("Unknown option character");
                }
                return NULL;
            default:
                log_crit( "Unknown getopt return");
                return NULL;
        }
    }

    log_crit("No config option supplied");
    return NULL;
}