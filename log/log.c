//
// Created by sergei on 10.06.22.
//

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include "log.h"

const char* LOG_PATH = "/tmp/myinit.log";

const char* CRIT_MSG = "CRIT";
const char* ERR_MSG = "ERR";
const char* WARNING_MSG = "WARNING";
const char* NOTICE_MSG = "NOTICE";
const char* INFO_MSG = "INFO";
const char* DEBUG_MSG = "DEBUG";

const char* UNK_MSG = "UNK";

FILE* log_file;

FILE* open_log() {
    log_file = fopen(LOG_PATH, "w"); // w is better for debug
    return log_file; // should add recursive directory creation
}

int flush_log() {
    return fflush(log_file);
}

int close_log() {
    return fclose(log_file);
}

const char* get_severity_msg(enum LogSeverity ls) {
    switch (ls) {
        case LS_CRIT:
            return CRIT_MSG;
        case LS_ERR:
            return ERR_MSG;
        case LS_WARNING:
            return WARNING_MSG;
        case LS_NOTICE:
            return NOTICE_MSG;
        case LS_INFO:
            return INFO_MSG;
        case LS_DEBUG:
            return DEBUG_MSG;
        default:
            return UNK_MSG;
    }
}

int log_msg(enum LogSeverity ls, const char* msg) {
    const char* severity_msg = get_severity_msg(ls);
    return fprintf(log_file, "%s %s\n", severity_msg, msg);
}

int log_sig(enum LogSeverity ls, int sig) {
    const char* severity_msg = get_severity_msg(ls);
    const char* signame = strsignal(sig);

    return fprintf(log_file, "%s Received signal %s (%d)\n", severity_msg, signame, sig);
}

int log_msg_e(enum LogSeverity ls, const char* msg, int err) {
    const char* severity_msg = get_severity_msg(ls);
    return fprintf(log_file, "%s %s: %s\n", severity_msg, msg, strerror(err));
}

int log_msg_f1e(enum LogSeverity ls, const char* msg_f, char* s, int err) {
    const char* severity_msg = get_severity_msg(ls);

    size_t msg_len = strlen(msg_f) - 1 + strlen(s);
    char* fmted = malloc(msg_len);
    if (fmted == NULL) {
        log_crit_e("Couldn't allocate fmted message", errno);
        return -1;
    }

    sprintf(fmted, msg_f, s);

    int print_res = fprintf(log_file, "%s %s: %s\n", severity_msg, fmted, strerror(err));
    free(fmted);
    return print_res;
}

int log_msg_f(enum LogSeverity ls, const char* msg_f, ...) {
    int res = 0;
    const char* severity_msg = get_severity_msg(ls);

    int print_res = fprintf(log_file, "%s ", severity_msg);
    if (print_res < 0) {
        return print_res;
    }
    res += print_res;

    va_list args;
    va_start(args, msg_f);
    print_res = vfprintf(log_file, msg_f,args);
    va_end(args);

    if (print_res < 0) {
        return print_res;
    }
    res += print_res;

    print_res = fprintf(log_file, "%c", '\n');
    if (print_res < 0) {
        return print_res;
    }
    res += print_res;

    return res;
}

int log_crit(const char* msg) {
    return log_msg(LS_CRIT, msg);
}

int log_crit_sig(int sig) {
    return log_sig(LS_CRIT, sig);
}

int log_crit_e(const char* msg, int err) {
    return log_msg_e(LS_CRIT, msg, err);
}

int log_err(char* msg) {
    return log_msg(LS_ERR, msg);
}
int log_err_e(char* msg, int err) {
    return log_msg_e(LS_ERR, msg, err);
}

int log_warning(char* msg) {
    return log_msg(LS_WARNING, msg);
}

int log_warning_e(char* msg, int err) {
    return log_msg_e(LS_WARNING, msg, err);
}

int log_warning_f1e(char* msg_f, char* s, int err) {
    return log_msg_f1e(LS_WARNING, msg_f, s, err);
}

int log_info(const char* msg) {
    return log_msg(LS_INFO, msg);
}

// can't forward variadic(((
int log_info_f(const char* msg_f, ...) {
    int res = 0;
    const char* severity_msg = get_severity_msg(LS_INFO);

    int print_res = fprintf(log_file, "%s ", severity_msg);
    if (print_res < 0) {
        return print_res;
    }
    res += print_res;

    va_list args;
    va_start(args, msg_f);
    print_res = vfprintf(log_file, msg_f,args);
    va_end(args);

    if (print_res < 0) {
        return print_res;
    }
    res += print_res;

    print_res = fprintf(log_file, "%c", '\n');
    if (print_res < 0) {
        return print_res;
    }
    res += print_res;

    return res;
}
//

//
//int log_notice(char* msg) {
//    return log_msg(LS_NOTICE, msg);
//}

//int log_debug(char* msg) {
//    return log_msg(LS_DEBUG, msg);
//}
