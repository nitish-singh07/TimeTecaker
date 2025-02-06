#include "config.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_config(Config *config) {
    json_object *root;
    FILE *fp = fopen(CONFIG_PATH, "r");
    
    // Set defaults
    config->report_hour = 8;
    config->report_minute = 0;
    config->save_interval = 300;
    config->ignored_apps_count = 0;
    config->ignored_apps = NULL;
    strcpy(config->discord_webhook_url, "");
    strcpy(config->whatsapp_number, "");

    if (!fp) {
        fprintf(stderr, "Warning: No config file found, using defaults\n");
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
        json_object *webhook_url, *whatsapp_num, *hour, *minute, 
                    *save_interval, *ignored_apps;
        
        if (json_object_object_get_ex(root, "discord_webhook_url", &webhook_url))
            strncpy(config->discord_webhook_url, 
                   json_object_get_string(webhook_url), 255);
        
        if (json_object_object_get_ex(root, "whatsapp_number", &whatsapp_num))
            strncpy(config->whatsapp_number, 
                   json_object_get_string(whatsapp_num), 31);
        
        if (json_object_object_get_ex(root, "report_hour", &hour))
            config->report_hour = json_object_get_int(hour);
        
        if (json_object_object_get_ex(root, "report_minute", &minute))
            config->report_minute = json_object_get_int(minute);

        if (json_object_object_get_ex(root, "save_interval", &save_interval))
            config->save_interval = json_object_get_int(save_interval);

        if (json_object_object_get_ex(root, "ignored_apps", &ignored_apps)) {
            int array_len = json_object_array_length(ignored_apps);
            config->ignored_apps = malloc(array_len * sizeof(char*));
            config->ignored_apps_count = array_len;

            for (int i = 0; i < array_len; i++) {
                json_object *app = json_object_array_get_idx(ignored_apps, i);
                const char *app_name = json_object_get_string(app);
                config->ignored_apps[i] = strdup(app_name);
            }
        }

        json_object_put(root);
    }
}

void save_config(Config *config) {
    json_object *root = json_object_new_object();
    
    json_object_object_add(root, "discord_webhook_url",
        json_object_new_string(config->discord_webhook_url));
    json_object_object_add(root, "whatsapp_number",
        json_object_new_string(config->whatsapp_number));
    json_object_object_add(root, "report_hour",
        json_object_new_int(config->report_hour));
    json_object_object_add(root, "report_minute",
        json_object_new_int(config->report_minute));
    json_object_object_add(root, "save_interval",
        json_object_new_int(config->save_interval));

    json_object *ignored_apps = json_object_new_array();
    for (int i = 0; i < config->ignored_apps_count; i++) {
        json_object_array_add(ignored_apps,
            json_object_new_string(config->ignored_apps[i]));
    }
    json_object_object_add(root, "ignored_apps", ignored_apps);

    FILE *fp = fopen(CONFIG_PATH, "w");
    if (fp) {
        fprintf(fp, "%s", json_object_to_json_string_ext(root, 
                JSON_C_TO_STRING_PRETTY));
        fclose(fp);
    }

    json_object_put(root);
}