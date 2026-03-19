#ifndef PING_LOGGER_H
#define PING_LOGGER_H

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_PING,
    LOG_LEVEL_TIMEOUT,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_STATS
} LogLevel;

typedef struct PingLogger {
    int console_output;
    int file_output;
    int log_level;
    char filename[260];
} PingLogger;

PingLogger* ping_logger_init(int console_output, int file_output, const char *filename);
void ping_logger_close(PingLogger *logger);

void ping_log(PingLogger *logger, LogLevel level, const char *format, ...);

void ping_log_start(PingLogger *logger, const char *host, int count, int interval_ms);
void ping_log_end(PingLogger *logger, const char *host);

void ping_log_result(PingLogger *logger, const char *host, int seq,
                      int ttl, double time_ms, int success);

void ping_log_statistics(PingLogger *logger, const char *host,
                           int sent, int received, int lost,
                           double min_time, double max_time, double avg_time);

void ping_log_error(PingLogger *logger, const char *host, const char *error_msg);

// Optional helpers (not required by the basic ping simulation)
void ping_log_with_rotation(PingLogger *logger, const char *message);
void ping_log_json(PingLogger *logger, const char *host, int seq,
                    double time_ms, int success);

#ifdef __cplusplus
}
#endif

#endif

