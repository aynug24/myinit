//
// Created by sergei on 10.06.22.
//

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "log.h"

const char* LOG_PATH = "/tmp/myinit/myinit.log";

const char* CRIT_MSG = "CRIT";
const char* ERR_MSG = "ERR";
const char* WARNING_MSG = "WARNING";
const char* NOTICE_MSG = "NOTICE";
const char* INFO_MSG = "INFO";
const char* DEBUG_MSG = "DEBUG";

const char* UNK_MSG = "UNK";

FILE* log_file = NULL;


int _mkdirs(const char* path) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", path);

    size_t len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for (char* p = tmp + 1; *p != '\0'; p++) {
        if (*p != '/') {
            continue;
        }

        *p = 0;
        if (mkdir(tmp, S_IRWXU) < 0 && errno != EEXIST) {
            return -1;
        }
        *p = '/';
    }
    return 0;
}

FILE* open_log() {
    if (_mkdirs(LOG_PATH) < 0) {
        return NULL;
    }
    log_file = fopen(LOG_PATH, "w"); // w is better for debug
    return log_file;
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
    return fprintf(log_file, "%ld %s %s\n", time(NULL), severity_msg, msg);
}

int log_msg_e(enum LogSeverity ls, const char* msg, int err) {
    const char* severity_msg = get_severity_msg(ls);
    return fprintf(log_file, "%ld %s %s: %s\n", time(NULL), severity_msg, msg, strerror(err));
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

    int print_res = fprintf(log_file, "%ld %s %s: %s\n", time(NULL), severity_msg, fmted, strerror(err));
    free(fmted);
    return print_res;
}

int log_msg_f(enum LogSeverity ls, const char* msg_f, ...) {
    int res = 0;
    const char* severity_msg = get_severity_msg(ls);

    int print_res = fprintf(log_file, "%ld %s ", time(NULL), severity_msg);
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

// can't forward variadic(((
int log_crit_f(const char* msg_f, ...) {
    int res = 0;
    const char* severity_msg = get_severity_msg(LS_CRIT);

    int print_res = fprintf(log_file, "%ld %s ", time(NULL), severity_msg);
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

    int print_res = fprintf(log_file, "%ld %s ", time(NULL), severity_msg);
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
