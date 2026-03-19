#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_LOG_MESSAGE 1024
#define LOG_FILE "ping_log_snippet.txt"

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_PING,
    LOG_LEVEL_TIMEOUT,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_STATS
} LogLevel;

typedef struct {
    FILE *file;
    int console_output;
    int file_output;
    int log_level;
} PingLogger;

// Получение текущего времени в формате строки
static void get_current_time(char *buffer, size_t size) {
    time_t raw_time;
    struct tm *time_info;

    time(&raw_time);
    time_info = localtime(&raw_time);

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", time_info);
}

// Получение времени с миллисекундами
static void get_current_time_ms(char *buffer, size_t size) {
    struct timeval tv;
    struct tm *tm_info;

    gettimeofday(&tv, NULL);
    {
        // На Windows/Msys2 тип tv.tv_sec может не совпадать с time_t, поэтому
        // делаем явное приведение через временную переменную.
        time_t sec = (time_t)tv.tv_sec;
        tm_info = localtime(&sec);
    }

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
    {
        char ms_buffer[64];
        snprintf(ms_buffer, sizeof(ms_buffer), ".%03ld", (long)(tv.tv_usec / 1000));
        strncat(buffer, ms_buffer, size - strlen(buffer) - 1);
    }
}

// Инициализация логгера
static PingLogger* ping_logger_init(int console_output, int file_output, const char *filename) {
    PingLogger *logger = (PingLogger*)malloc(sizeof(PingLogger));
    if (!logger) return NULL;

    logger->console_output = console_output ? 1 : 0;
    logger->file_output = file_output ? 1 : 0;
    logger->log_level = LOG_LEVEL_INFO;

    logger->file = NULL;
    if (logger->file_output) {
        logger->file = fopen(filename ? filename : LOG_FILE, "a");
        if (!logger->file) {
            printf("Ошибка: не удалось открыть файл лога %s\n", filename ? filename : LOG_FILE);
            logger->file_output = 0;
        }
    }

    return logger;
}

// Закрытие логгера
static void ping_logger_close(PingLogger *logger) {
    if (logger) {
        if (logger->file) {
            fclose(logger->file);
        }
        free(logger);
    }
}

// Основная функция логирования
static void ping_log(PingLogger *logger, LogLevel level, const char *format, ...) {
    if (!logger) return;

    char time_buffer[64];
    char message[MAX_LOG_MESSAGE];
    char log_entry[MAX_LOG_MESSAGE + 128];
    const char *level_str;

    // Получаем текущее время
    get_current_time_ms(time_buffer, sizeof(time_buffer));

    // Определяем уровень логирования
    switch(level) {
        case LOG_LEVEL_INFO: level_str = "INFO"; break;
        case LOG_LEVEL_PING: level_str = "PING"; break;
        case LOG_LEVEL_TIMEOUT: level_str = "TIMEOUT"; break;
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        case LOG_LEVEL_STATS: level_str = "STATS"; break;
        default: level_str = "UNKNOWN";
    }

    // Форматируем сообщение
    {
        va_list args;
        va_start(args, format);
        vsnprintf(message, sizeof(message), format, args);
        va_end(args);
    }

    // Формируем запись лога
    snprintf(log_entry, sizeof(log_entry), "[%s] [%s] %s",
             time_buffer, level_str, message);

    // Вывод в консоль
    if (logger->console_output) {
        printf("%s\n", log_entry);
    }

    // Запись в файл
    if (logger->file_output && logger->file) {
        fprintf(logger->file, "%s\n", log_entry);
        fflush(logger->file); // Сбрасываем буфер для немедленной записи
    }
}

int main(void) {
    PingLogger *logger = ping_logger_init(1, 1, LOG_FILE);
    if (!logger) {
        fprintf(stderr, "Не удалось создать логгер\n");
        return 1;
    }

    ping_log(logger, LOG_LEVEL_INFO, "Старт теста логгера");
    ping_log(logger, LOG_LEVEL_PING, "PING host=%s seq=%d", "example.com", 1);
    ping_log(logger, LOG_LEVEL_TIMEOUT, "TIMEOUT host=%s seq=%d", "example.com", 2);
    ping_log(logger, LOG_LEVEL_ERROR, "ERROR код=%d", 500);
    ping_log(logger, LOG_LEVEL_STATS, "STATS sent=%d received=%d lost=%d", 3, 2, 1);

    ping_logger_close(logger);
    return 0;
}

