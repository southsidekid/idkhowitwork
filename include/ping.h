#ifndef PING_H
#define PING_H

#include "logger.h"

// Пинг (пока симуляция): генерируем события (PING/TIMEOUT/ERROR) и пишем их в лог.
int ping_host_sim(PingLogger *logger, const char *host, int count, int interval_ms);

#endif

