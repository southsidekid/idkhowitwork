#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <locale.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#define MAX_LOG_MESSAGE 1024
#define LOG_FILE_DEFAULT "ping_log.txt"

static void get_current_time_ms(char *buffer, size_t size) {
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, size, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
             (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay,
             (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond,
             (unsigned)st.wMilliseconds);
#else
    struct timeval tv;
    struct tm *tm_info;
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    if (!tm_info) {
        snprintf(buffer, size, "1970-01-01 00:00:00.000");
        return;
    }
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
    char ms_buffer[64];
    snprintf(ms_buffer, sizeof(ms_buffer), ".%03ld", (long)(tv.tv_usec / 1000));
    strncat(buffer, ms_buffer, size - strlen(buffer) - 1);
#endif
}

static const char* level_to_str(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_PING: return "PING";
        case LOG_LEVEL_TIMEOUT: return "TIMEOUT";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_STATS: return "STATS";
        default: return "UNKNOWN";
    }
}

PingLogger* ping_logger_init(int console_output, int file_output, const char *filename) {
    PingLogger *logger = (PingLogger*)malloc(sizeof(PingLogger));
    if (!logger) return NULL;

    logger->console_output = console_output ? 1 : 0;
    logger->file_output = file_output ? 1 : 0;
    logger->log_level = LOG_LEVEL_INFO;
    {
        const char *fn = (filename && filename[0]) ? filename : LOG_FILE_DEFAULT;
        strncpy(logger->filename, fn, sizeof(logger->filename) - 1);
        logger->filename[sizeof(logger->filename) - 1] = '\0';
    }

#ifdef _WIN32
    if (logger->console_output) {
        // На Windows PowerShell кириллица часто “ломается”, если кодировка консоли
        // не совпадает с тем, что печатает printf (UTF-8 байты).
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        setlocale(LC_ALL, "C.UTF-8");
    }
#endif

    return logger;
}

void ping_logger_close(PingLogger *logger) {
    if (!logger) return;
    free(logger);
}

static int my_log(PingLogger *logger, LogLevel level, const char *log_entry) {
    if (!logger || !log_entry) return -1;

    // Уровень логирования: логируем только если level >= logger->log_level.
    if (level < (LogLevel)logger->log_level) {
        return 0; // фильтр: “ничего не делаем”
    }

    // Open(log); rv; (ветвление по rv); print("log"...) как на блок-схеме.
    // rv: 0 = success, rv != 0 = failure
    int rv = 0;

    if (logger->file_output) {
        FILE *f = fopen(logger->filename, "a");
        if (!f) {
            rv = -1;
        } else {
            rv = 0;
            fprintf(f, "%s\n", log_entry);
            fflush(f);
            fclose(f);
        }
    }

    // print("log"...): делаем как fallback при Open(log) неуспешном (rv != 0).
    // Если файл вообще не включён — печатаем при console_output.
    if (logger->console_output) {
        if (!logger->file_output || rv != 0) {
            printf("%s\n", log_entry);
        }
    }

    return rv;
}

void ping_log(PingLogger *logger, LogLevel level, const char *format, ...) {
    if (!logger) return;

    char time_buffer[64];
    char message[MAX_LOG_MESSAGE];
    char log_entry[MAX_LOG_MESSAGE + 128];

    get_current_time_ms(time_buffer, sizeof(time_buffer));

    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    snprintf(log_entry, sizeof(log_entry), "[%s] [%s] %s",
             time_buffer, level_to_str(level), message);

    (void)my_log(logger, level, log_entry);
}

void ping_log_start(PingLogger *logger, const char *host, int count, int interval_ms) {
    ping_log(logger, LOG_LEVEL_INFO,
             "Начало ping теста для %s (количество: %d, интервал: %d мс)",
             host, count, interval_ms);
}

void ping_log_end(PingLogger *logger, const char *host) {
    ping_log(logger, LOG_LEVEL_INFO, "Завершение ping теста для %s", host);
}

void ping_log_result(PingLogger *logger, const char *host, int seq,
                      int ttl, double time_ms, int success) {
    if (success) {
        ping_log(logger, LOG_LEVEL_PING,
                 "Ответ от %s: seq=%d ttl=%d время=%.2f мс",
                 host, seq, ttl, time_ms);
    } else {
        ping_log(logger, LOG_LEVEL_TIMEOUT,
                 "Превышен интервал ожидания для %s seq=%d",
                 host, seq);
    }
}

void ping_log_statistics(PingLogger *logger, const char *host,
                           int sent, int received, int lost,
                           double min_time, double max_time, double avg_time) {
    double loss_percent = sent > 0 ? (double)lost / (double)sent * 100.0 : 0.0;

    ping_log(logger, LOG_LEVEL_STATS,
             "\n--- Статистика ping для %s ---\n"
             "Отправлено: %d\n"
             "Получено: %d\n"
             "Потеряно: %d (%.1f%% потерь)\n"
             "Минимальное время: %.2f мс\n"
             "Максимальное время: %.2f мс\n"
             "Среднее время: %.2f мс",
             host, sent, received, lost, loss_percent,
             min_time, max_time, avg_time);
}

void ping_log_error(PingLogger *logger, const char *host, const char *error_msg) {
    ping_log(logger, LOG_LEVEL_ERROR, "Ошибка при ping %s: %s", host, error_msg ? error_msg : "(none)");
}

void ping_log_with_rotation(PingLogger *logger, const char *message) {
    (void)logger; // В этом варианте функция работает независимо от PingLogger.
    time_t t = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    struct tm *tm_info = localtime(&t);
    if (!tm_info) return;
    tmv = *tm_info;
#endif

    char filename[256];
    snprintf(filename, sizeof(filename), "ping_log_%d-%02d-%02d.log",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);

    FILE *daily_log = fopen(filename, "a");
    if (daily_log) {
        fprintf(daily_log, "%s\n", message ? message : "");
        fclose(daily_log);
    }
}

void ping_log_json(PingLogger *logger, const char *host, int seq,
                    double time_ms, int success) {
    char time_buf[64];
    get_current_time_ms(time_buf, sizeof(time_buf));

    char json[512];
    snprintf(json, sizeof(json),
             "{\"timestamp\":\"%s\",\"host\":\"%s\",\"seq\":%d,\"time_ms\":%.2f,\"success\":%d}",
             time_buf, host ? host : "", seq, time_ms, success);

    ping_log(logger, LOG_LEVEL_INFO, "%s", json);
}

