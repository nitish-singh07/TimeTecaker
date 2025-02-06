#include "tracker.h"
#include "notifications.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <json-c/json.h>

void save_usage_data(ApplicationTracker *tracker) {
    json_object *root = json_object_new_object();
    json_object *apps_array = json_object_new_array();

    pthread_mutex_lock(&tracker->mutex);
    
    for (int i = 0; i < tracker->app_count; i++) {
        json_object *app_obj = json_object_new_object();
        
        json_object_object_add(app_obj, "name",
            json_object_new_string(tracker->apps[i].app_name));
        json_object_object_add(app_obj, "duration",
            json_object_new_double(tracker->apps[i].duration));
        json_object_object_add(app_obj, "switches",
            json_object_new_int64(tracker->apps[i].switches));
        
        json_object_array_add(apps_array, app_obj);
    }
    
    json_object_object_add(root, "apps", apps_array);
    
    FILE *fp = fopen(DATA_PATH, "w");
    if (fp) {
        fprintf(fp, "%s", json_object_to_json_string_ext(root, 
                JSON_C_TO_STRING_PRETTY));
        fclose(fp);
    }
    
    pthread_mutex_unlock(&tracker->mutex);
    json_object_put(root);
}

void load_usage_data(ApplicationTracker *tracker) {
    json_object *root;
    FILE *fp = fopen(DATA_PATH, "r");
    
    if (!fp) {
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *json_str = malloc(file_size + 1);
    fread(json_str, 1, file_size, fp);
    json_str[file_size] = '\0';
    fclose(fp);

    root = json_tokener_parse(json_str);
    free(json_str);

    if (root) {
        json_object *apps_array;
        if (json_object_object_get_ex(root, "apps", &apps_array)) {
            int array_len = json_object_array_length(apps_array);
            
            pthread_mutex_lock(&tracker->mutex);
            tracker->app_count = 0;
            
            for (int i = 0; i < array_len && i < MAX_APPS; i++) {
                json_object *app_obj = json_object_array_get_idx(apps_array, i);
                json_object *name, *duration, *switches;
                
                if (json_object_object_get_ex(app_obj, "name", &name) &&
                    json_object_object_get_ex(app_obj, "duration", &duration) &&
                    json_object_object_get_ex(app_obj, "switches", &switches)) {
                    
                    strncpy(tracker->apps[i].app_name,
                           json_object_get_string(name),
                           MAX_WINDOW_TITLE - 1);
                    tracker->apps[i].duration = json_object_get_double(duration);
                    tracker->apps[i].switches = json_object_get_int64(switches);
                    gettimeofday(&tracker->apps[i].last_active, NULL);
                    tracker->app_count++;
                }
            }
            pthread_mutex_unlock(&tracker->mutex);
        }
        json_object_put(root);
    }
}

char* get_active_window_title(ApplicationTracker *tracker) {
    static char window_title[MAX_WINDOW_TITLE];
    Window focused;
    int revert_to;
    
    XGetInputFocus(tracker->display, &focused, &revert_to);
    
    if (focused == None || focused == PointerRoot) {
        return "No Focus";
    }
    
    XTextProperty text_prop;
    if (XGetWMName(tracker->display, focused, &text_prop) != 0) {
        strncpy(window_title, (char *)text_prop.value, MAX_WINDOW_TITLE - 1);
        window_title[MAX_WINDOW_TITLE - 1] = '\0';
        XFree(text_prop.value);
    } else {
        strcpy(window_title, "Unknown");
    }
    
    return window_title;
}

void update_app_usage(ApplicationTracker *tracker, const char *window_title) {
    struct timeval now;
    gettimeofday(&now, NULL);
    
    pthread_mutex_lock(&tracker->mutex);
    
    double time_diff = (now.tv_sec - tracker->last_check.tv_sec) + 
                      (now.tv_usec - tracker->last_check.tv_usec) / 1000000.0;
    
    int found = 0;
    for (int i = 0; i < tracker->app_count; i++) {
        if (strcmp(tracker->apps[i].app_name, window_title) == 0) {
            tracker->apps[i].duration += time_diff;
            tracker->apps[i].switches++;
            tracker->apps[i].last_active = now;
            found = 1;
            break;
        }
    }
    
    if (!found && tracker->app_count < MAX_APPS) {
        strncpy(tracker->apps[tracker->app_count].app_name, 
                window_title, MAX_WINDOW_TITLE - 1);
        tracker->apps[tracker->app_count].duration = time_diff;
        tracker->apps[tracker->app_count].switches = 1;
        tracker->apps[tracker->app_count].last_active = now;
        tracker->app_count++;
    }
    
    tracker->last_check = now;
    pthread_mutex_unlock(&tracker->mutex);
}

void init_tracker(ApplicationTracker *tracker) {
    tracker->app_count = 0;
    tracker->root = DefaultRootWindow(tracker->display);
    gettimeofday(&tracker->last_check, NULL);
    load_usage_data(tracker);
}

void start_tracking(ApplicationTracker *tracker) {
    time_t last_save = time(NULL);
    time_t last_report = time(NULL);

    while (1) {
        char *window_title = get_active_window_title(tracker);
        update_app_usage(tracker, window_title);

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        
        if (tm_info->tm_hour == tracker->config.report_hour && 
            tm_info->tm_min == tracker->config.report_minute) {
            
            if (difftime(now, last_report) >= 86400) {
                send_daily_report(tracker);
                last_report = now;
            }
        }

        if (difftime(now, last_save) >= tracker->config.save_interval) {
            save_usage_data(tracker);
            last_save = now;
        }

        usleep(100000); // 100ms interval
    }
}