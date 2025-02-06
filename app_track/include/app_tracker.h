#ifndef APP_TRACKER_H
#define APP_TRACKER_H

#include <X11/Xlib.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_WINDOW_TITLE 256
#define MAX_APPS 1000
#define CONFIG_PATH "data/config.json"
#define DATA_PATH "data/usage_data.json"

typedef struct {
    char app_name[MAX_WINDOW_TITLE];
    double duration;
    struct timeval last_active;
    unsigned long long switches;
} AppUsage;

typedef struct {
    char discord_webhook_url[256];
    char whatsapp_number[32];
    int report_hour;
    int report_minute;
    int save_interval;
    char **ignored_apps;
    int ignored_apps_count;
} Config;

typedef struct {
    Display *display;
    Window root;
    AppUsage apps[MAX_APPS];
    int app_count;
    pthread_mutex_t mutex;
    struct timeval last_check;
    Config config;
} ApplicationTracker;

#endif // APP_TRACKER_H