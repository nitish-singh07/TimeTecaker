#include "notifications.h"
#include <curl/curl.h>
#include <json-c/json.h>
#include <libnotify/notify.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void send_desktop_notification(const char *title, const char *message) {
    notify_init("App Tracker");
    NotifyNotification *notification = notify_notification_new(
        title, message, "dialog-information"
    );
    notify_notification_show(notification, NULL);
    g_object_unref(G_OBJECT(notification));
    notify_uninit();
}

void send_discord_notification(const char *webhook_url, const char *report) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    
    curl = curl_easy_init();
    if (curl) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        json_object *root = json_object_new_object();
        json_object *embeds = json_object_new_array();
        json_object *embed = json_object_new_object();

        json_object_object_add(embed, "title",
            json_object_new_string("📊 Daily Application Usage Report"));
        json_object_object_add(embed, "description",
            json_object_new_string(report));
        json_object_object_add(embed, "color",
            json_object_new_int(3447003));

        json_object_array_add(embeds, embed);
        json_object_object_add(root, "embeds", embeds);

        const char *json_str = json_object_to_json_string(root);

        curl_easy_setopt(curl, CURLOPT_URL, webhook_url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Discord notification failed: %s\n",
                    curl_easy_strerror(res));
        }

        json_object_put(root);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

void generate_report(ApplicationTracker *tracker, char *report, size_t report_size) {
    pthread_mutex_lock(&tracker->mutex);
    
    int indices[MAX_APPS];
    for (int i = 0; i < tracker->app_count; i++) {
        indices[i] = i;
    }
    
    for (int i = 0; i < tracker->app_count - 1; i++) {
        for (int j = 0; j < tracker->app_count - i - 1; j++) {
            if (tracker->apps[indices[j]].duration < 
                tracker->apps[indices[j + 1]].duration) {
                int temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    int offset = snprintf(report, report_size,
        "Report for %04d-%02d-%02d\n\n",
        tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    
    double total_time = 0;
    for (int i = 0; i < tracker->app_count; i++) {
        total_time += tracker->apps[i].duration;
    }
    
    for (int i = 0; i < tracker->app_count; i++) {
        AppUsage *app = &tracker->apps[indices[i]];
        int hours = (int)app->duration / 3600;
        int minutes = ((int)app->duration % 3600) / 60;
        float percentage = (app->duration / total_time) * 100;
        
        offset += snprintf(report + offset, report_size - offset,
            "• %s: %dh %dm (%.1f%%)\n",
            app->app_name, hours, minutes, percentage);
    }
    
    offset += snprintf(report + offset, report_size - offset,
        "\nTotal tracked time: %.1f hours", total_time / 3600);
    
    pthread_mutex_unlock(&tracker->mutex);
}

void send_daily_report(ApplicationTracker *tracker) {
    char report[4096];
    generate_report(tracker, report, sizeof(report));
    
    send_desktop_notification("Daily Report Ready", 
        "Your application usage report has been generated.");
    
    if (strlen(tracker->config.discord_webhook_url) > 0) {
        send_discord_notification(tracker->config.discord_webhook_url, report);
    }
    
    pthread_mutex_lock(&tracker->mutex);
    for (int i = 0; i < tracker->app_count; i++) {
        tracker->apps[i].duration = 0;
        tracker->apps[i].switches = 0;
    }
    pthread_mutex_unlock(&tracker->mutex);
}