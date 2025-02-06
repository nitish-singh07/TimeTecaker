#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include "app_tracker.h"

void send_desktop_notification(const char *title, const char *message);
void send_discord_notification(const char *webhook_url, const char *report);
void generate_report(ApplicationTracker *tracker, char *report, size_t report_size);
void send_daily_report(ApplicationTracker *tracker);

#endif // NOTIFICATIONS_H