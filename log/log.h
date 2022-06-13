#ifndef MYINIT_LOG_H
#define MYINIT_LOG_H

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
int log_crit_f(const char* msg_f, ...);

int log_err(char* msg);
int log_err_e(char* msg, int err);

int log_warning(char* msg);
int log_warning_e(char* msg, int err);
int log_warning_f1e(char* msg_f, char* s, int err); // f => format, 1 => one format string arg, e => append errno msg

int log_info(const char* msg);
int log_info_f(const char* msg_f, ...);

#endif //MYINIT_LOG_H
