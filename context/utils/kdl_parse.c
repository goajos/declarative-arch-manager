#include <kdl/kdl.h> 
#include <stdio.h>
#include <string.h>
#include "../../utils/array.h"
#include "../../utils/memory.h"

static size_t read_func(void* user_data, char* buf, size_t bufsize) {
    FILE* fid = (FILE* )user_data;
    return fread(buf, 1, bufsize, fid);
}

// active=#true || active=#false
// active=1     || active=0
// #true        || #false
// 1            || 0
static bool parse_value(kdl_value value) {
    kdl_type type = value.type;
    bool boolean;
    if (type == KDL_TYPE_BOOLEAN) {
        boolean = value.boolean;
    } else if (type == KDL_TYPE_NUMBER) {
        long long integer = value.number.integer;
        if (integer == 1) boolean = true;
        else if (integer == 0) boolean = false;
    }
    return boolean;
}

int parse_config_kdl(FILE* fid, struct active_dynamic_array* hosts, char** aur_helper) {
    kdl_parser* parser = kdl_create_stream_parser(&read_func, (void* )fid, KDL_DEFAULTS);

    size_t depth = 0;
    char* node_d1 = nullptr;
    char* node_d2 = nullptr;
    bool eof = false;

    while(1) {
        kdl_event_data* next_event = kdl_parser_next_event(parser);
        kdl_event event = next_event->event;
        const char* name_data = next_event->name.data;
        kdl_value value = next_event->value;
        bool boolean;
        switch(event) {
            case KDL_EVENT_START_NODE:
                switch(depth) {
                    case 0:
                        if (memcmp(name_data, "config", 6) != 0) goto invalid_config;
                        break;
                    case 1:
                        node_d1 = string_copy((char* )name_data);
                        break;
                    case 2:
                        node_d2 = string_copy((char* )name_data);
                        break;
                }
                depth += 1;
                break;
            case KDL_EVENT_END_NODE:
                depth -= 1;
                break;
            case KDL_EVENT_ARGUMENT:
                parse_event:
                boolean = parse_value(value);
                if (memcmp(node_d1, "aur_helper", 10) == 0) {
                    if (boolean) *aur_helper = node_d2;
                } else {
                    struct active_element host = { node_d1, boolean };       
                    if (hosts) DYNAMIC_ARRAY_APPEND((*hosts), host);
                }
                break;
            case KDL_EVENT_PROPERTY:
                goto parse_event;
            case KDL_EVENT_PARSE_ERROR:
                break;
            case KDL_EVENT_COMMENT:
                break;
            case KDL_EVENT_EOF:
                eof = true;
                break;
        }
        if (eof) break; // while break
    }

    if (node_d1 != nullptr) {
        free_sized(node_d1, strlen(node_d1));
        node_d1 = nullptr;
    }
    kdl_destroy_parser(parser); 
    return EXIT_SUCCESS;
    
    invalid_config:
        puts("Not a valid config kdl file\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;
}

int parse_host_kdl(FILE* fid, struct active_dynamic_array* modules, struct active_dynamic_array* services,  struct active_dynamic_array* hooks) {
    kdl_parser* parser = kdl_create_stream_parser(&read_func, (void* )fid, KDL_DEFAULTS);

    size_t depth = 0;
    char* node_d1 = nullptr;
    char* node_d2 = nullptr;
    bool eof = false;

    while(1) {
        kdl_event_data* next_event = kdl_parser_next_event(parser);
        kdl_event event = next_event->event;
        const char* name_data = next_event->name.data;
        kdl_value value = next_event->value;
        bool boolean;
        switch(event) {
            case KDL_EVENT_START_NODE:
                switch(depth) {
                    case 0:
                        if (memcmp(name_data, "host", 4) != 0) goto invalid_config;
                        break;
                    case 1:
                        node_d1 = string_copy((char* )name_data);
                        break;
                    case 2:
                        node_d2 = string_copy((char* )name_data);
                        break;
                }
                depth += 1;
                break;
            case KDL_EVENT_END_NODE:
                depth -= 1;
                break;
            case KDL_EVENT_ARGUMENT:
                parse_event:
                boolean = parse_value(value);
                if (memcmp(node_d1, "modules", 7) == 0) {
                    struct active_element module = { node_d2, boolean };
                    DYNAMIC_ARRAY_APPEND((*modules), module);
                } else if (memcmp(node_d1, "services", 8) == 0) {
                    struct active_element service = { node_d1, boolean };       
                    DYNAMIC_ARRAY_APPEND((*services), service);
                } else if (memcmp(node_d1, "hooks", 6) == 0) {
                    struct active_element hook = { node_d1, boolean };       
                    DYNAMIC_ARRAY_APPEND((*hooks), hook);
                }
                break;
            case KDL_EVENT_PROPERTY:
                goto parse_event;
            case KDL_EVENT_PARSE_ERROR:
                break;
            case KDL_EVENT_COMMENT:
                break;
            case KDL_EVENT_EOF:
                eof = true;
                break;
        }
        if (eof) break; // while break
    }

    if (node_d1 != nullptr) {
        free_sized(node_d1, strlen(node_d1));
        node_d1 = nullptr;
    }
    kdl_destroy_parser(parser); 
    return EXIT_SUCCESS;
    
    invalid_config:
        puts("Not a valid host kdl file\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;
}

int parse_module_kdl(FILE* fid, struct active_dynamic_array* packages, struct active_dynamic_array* aur_packages, struct active_dynamic_array* services,  struct active_dynamic_array* hooks) {
    kdl_parser* parser = kdl_create_stream_parser(&read_func, (void* )fid, KDL_DEFAULTS);

    size_t depth = 0;
    char* node_d1 = nullptr;
    char* node_d2 = nullptr;
    bool eof = false;

    while(1) {
        kdl_event_data* next_event = kdl_parser_next_event(parser);
        kdl_event event = next_event->event;
        const char* name_data = next_event->name.data;
        kdl_value value = next_event->value;
        bool boolean;
        switch(event) {
            case KDL_EVENT_START_NODE:
                switch(depth) {
                    case 0:
                        if (memcmp(name_data, "module", 6) != 0) goto invalid_config;
                        break;
                    case 1:
                        node_d1 = string_copy((char* )name_data);
                        break;
                    case 2:
                        node_d2 = string_copy((char* )name_data);
                        break;
                }
                depth += 1;
                break;
            case KDL_EVENT_END_NODE:
                depth -= 1;
                break;
            case KDL_EVENT_ARGUMENT:
                parse_event:
                boolean = parse_value(value);
                if (memcmp(node_d1, "packages", 8) == 0) {
                    struct active_element package = { node_d2, boolean };
                    DYNAMIC_ARRAY_APPEND((*packages), package);
                } else if (memcmp(node_d1, "aur_packages", 12) == 0) {
                    struct active_element aur_package = { node_d2, boolean };
                    DYNAMIC_ARRAY_APPEND((*aur_packages), aur_package);
                } else if (memcmp(node_d1, "services", 8) == 0) {
                    struct active_element service = { node_d1, boolean };       
                    DYNAMIC_ARRAY_APPEND((*services), service);
                } else if (memcmp(node_d1, "hooks", 6) == 0) {
                    struct active_element hook = { node_d1, boolean };       
                    DYNAMIC_ARRAY_APPEND((*hooks), hook);
                }
                break;
            case KDL_EVENT_PROPERTY:
                goto parse_event;
            case KDL_EVENT_PARSE_ERROR:
                break;
            case KDL_EVENT_COMMENT:
                break;
            case KDL_EVENT_EOF:
                eof = true;
                break;
        }
        if (eof) break; // while break
    }

    if (node_d1 != nullptr) {
        free_sized(node_d1, strlen(node_d1));
        node_d1 = nullptr;
    }
    kdl_destroy_parser(parser); 
    return EXIT_SUCCESS;
    
    invalid_config:
        puts("Not a valid module kdl file\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;
}
