#ifndef TRACKER_H
#define TRACKER_H

#include "app_tracker.h"

void init_tracker(ApplicationTracker *tracker);
void start_tracking(ApplicationTracker *tracker);
char* get_active_window_title(ApplicationTracker *tracker);
void update_app_usage(ApplicationTracker *tracker, const char *window_title);
void save_usage_data(ApplicationTracker *tracker);
void load_usage_data(ApplicationTracker *tracker);

#endif // TRACKER_H