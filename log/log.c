//
// Created by sergei on 10.06.22.
//

#include <stdio.h>
#include <string.h>
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

int log_msg_e(enum LogSeverity ls, const char* msg, int err) {
    const char* severity_msg = get_severity_msg(ls);
    return fprintf(log_file, "%s %s: %s\n", severity_msg, msg, strerror(err));
}

int log_crit(const char* msg) {
    return log_msg(LS_CRIT, msg);
}

int log_crit_e(const char* msg, int err) {
    return log_msg_e(LS_CRIT, msg, err);
}

int log_info(const char* msg) {
    return log_msg(LS_INFO, msg);
}

//int log_err(char* msg) {
//    return log_msg(LS_ERR, msg);
//}
//
//int log_warning(char* msg) {
//    return log_msg(LS_WARNING, msg);
//}
//
//int log_notice(char* msg) {
//    return log_msg(LS_NOTICE, msg);
//}

//int log_debug(char* msg) {
//    return log_msg(LS_DEBUG, msg);
//}
