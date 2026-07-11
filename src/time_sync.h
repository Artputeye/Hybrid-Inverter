//time_sync.h
#ifndef TIME_SYNC_H
#define TIME_SYNC_H
#include "config.h"

struct DateTimeCache
{
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int second;
};
extern DateTimeCache rtc;

// ===== constants =====
#define RESYNC_INTERVAL_MS (24UL * 60 * 60 * 1000)

// ===== extern variables =====
extern volatile bool ntpSynced;
extern volatile bool needNtpSync;

extern TaskHandle_t ntpTaskHandle;
extern TimerHandle_t ntpResyncTimer;

// ===== functions =====
void TaskNTP(void *pvParameters);
void NTPbegin();
void ntpResyncCallback(TimerHandle_t xTimer);
void timeUpdade();
void updateDateCache(const struct tm &t);
void resyncTime();

unsigned long getUptimeSeconds();
String getUptimeString();

#endif
