#include "ping.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(int ms) {
    Sleep(ms);
}
#else
#include <unistd.h>
static void sleep_ms(int ms) {
    usleep((useconds_t)ms * 1000u);
}
#endif

int ping_host_sim(PingLogger *logger, const char *host, int count, int interval_ms) {
    if (!logger) return -1;
    if (!host || host[0] == '\0') {
        ping_log_error(logger, "(null)", "пустой host");
        return -1;
    }
    if (count <= 0) return -1;
    if (interval_ms < 0) interval_ms = 0;

    ping_log_start(logger, host, count, interval_ms);

    int sent = 0;
    int received = 0;
    int lost = 0;

    double min_time = 0.0;
    double max_time = 0.0;
    double avg_time = 0.0; // накопление для avg

    // seed: только для симуляции, чтобы было “чуть похоже” на ping.
    srand((unsigned)time(NULL));

    for (int i = 0; i < count; i++) {
        sent++;

        // 80% успеха.
        int r = rand() % 100;
        int success = (r < 80) ? 1 : 0;
        int ttl = 64;
        double response_time = 0.0;

        int seq = i + 1;
        if (success) {
            received++;
            response_time = 10.0 + (rand() % 9000) / 100.0; // 10..100 мс

            ping_log_result(logger, host, seq, ttl, response_time, 1);

            if (received == 1) {
                min_time = response_time;
                max_time = response_time;
            } else {
                if (response_time < min_time) min_time = response_time;
                if (response_time > max_time) max_time = response_time;
            }

            avg_time += response_time;
        } else {
            lost++;
            ping_log_result(logger, host, seq, 0, 0.0, 0);
        }

        if (i + 1 < count) {
            sleep_ms(interval_ms);
        }
    }

    if (received > 0) avg_time /= (double)received;

    ping_log_statistics(logger, host, sent, received, lost, min_time, max_time, avg_time);
    ping_log_end(logger, host);

    return 0;
}

