#include "ping.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog) {
    printf("Использование:\n");
    printf("  %s <host> [count] [interval_ms] [--no-file] [--no-console] [--log-file <path>]\n", prog);
    printf("\nПример:\n");
    printf("  %s google.com 4 1000\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *host = argv[1];
    int count = 4;
    int interval_ms = 1000;
    int console_output = 1;
    int file_output = 1;
    const char *log_file = "ping_log.txt";

    // Разбор аргументов: после host ожидаем count/interval, затем опции.
    int idx = 2;
    if (idx < argc && argv[idx][0] != '-') {
        count = atoi(argv[idx]);
        idx++;
    }
    if (idx < argc && argv[idx][0] != '-') {
        interval_ms = atoi(argv[idx]);
        idx++;
    }

    for (; idx < argc; idx++) {
        if (strcmp(argv[idx], "--no-file") == 0) {
            file_output = 0;
        } else if (strcmp(argv[idx], "--no-console") == 0) {
            console_output = 0;
        } else if (strcmp(argv[idx], "--log-file") == 0 && idx + 1 < argc) {
            log_file = argv[++idx];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    PingLogger *logger = ping_logger_init(console_output, file_output, log_file);
    if (!logger) {
        printf("Ошибка инициализации логгера\n");
        return 1;
    }

    int rc = ping_host_sim(logger, host, count, interval_ms);
    ping_logger_close(logger);
    return rc == 0 ? 0 : 1;
}

