#include "app_tracker.h"
#include "tracker.h"
#include "config.h"
#include "notifications.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

ApplicationTracker *global_tracker = NULL;

void cleanup(int signum) {
    printf("\nReceived signal %d, cleaning up...\n", signum);
    if (global_tracker) {
        save_usage_data(global_tracker);
        if (global_tracker->display) {
            XCloseDisplay(global_tracker->display);
        }
        pthread_mutex_destroy(&global_tracker->mutex);
        free(global_tracker);
    }
    exit(0);
}

void setup_signal_handlers() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
}

void send_startup_notification(ApplicationTracker *tracker) {
    send_desktop_notification(
        "Application Monitoring Started",
        "⚠️ Your machine time is now being monitored. Stay focused!"
    );

    const char *startup_message = "🚀 **Application Monitoring Started**\n\n"
                                "⚠️ Your machine time is now being monitored.\n"
                                "💡 Remember to stay focused and productive!\n"
                                "📊 Daily reports will be sent at scheduled time.";

    if (strlen(tracker->config.discord_webhook_url) > 0) {
        send_discord_notification(tracker->config.discord_webhook_url, startup_message);
    }
}

int main() {
    global_tracker = malloc(sizeof(ApplicationTracker));
    if (!global_tracker) {
        fprintf(stderr, "Failed to allocate memory for tracker\n");
        return 1;
    }

    if (pthread_mutex_init(&global_tracker->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex\n");
        free(global_tracker);
        return 1;
    }

    global_tracker->display = XOpenDisplay(NULL);
    if (!global_tracker->display) {
        fprintf(stderr, "Cannot open display\n");
        pthread_mutex_destroy(&global_tracker->mutex);
        free(global_tracker);
        return 1;
    }

    load_config(&global_tracker->config);
    setup_signal_handlers();
    init_tracker(global_tracker);
    send_startup_notification(global_tracker);

    printf("Application tracker started. Press Ctrl+C to exit.\n");
    start_tracking(global_tracker);

    return 0;
}