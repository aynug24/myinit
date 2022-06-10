//
// Created by sergei on 10.06.22.
//

#ifndef UNTITLED_LOG_H
#define UNTITLED_LOG_H

#include <bits/types/FILE.h>

enum LogSeverity {
    LS_CRIT,
    LS_ERR,
    LS_WARNING,
    LS_NOTICE,
    LS_INFO,
    LS_DEBUG
};

extern const char* LOG_PATH;

FILE* open_log();
int flush_log();
int close_log();

int log_crit(const char* msg);
int log_crit_e(const char* msg, int err);
int log_info(const char* msg);

#endif //UNTITLED_LOG_H
