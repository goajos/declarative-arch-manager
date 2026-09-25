#include <kdl/kdl.h> 
#include <string.h>
#include "state.h"
#include "state_utils.h"

int parse_config_kdl(FILE* fid, struct config* config)  {
    kdl_parser* parser = kdl_create_stream_parser(&read_func, (void* )fid, KDL_DEFAULTS);
    
    size_t depth = 0;
    char* node_d1 = nullptr;
    bool eof = false;

    while(true) {
        kdl_event_data* next_event = kdl_parser_next_event(parser);
        kdl_event event = next_event->event;
        const char* name_data = next_event->name.data;
        kdl_value value = next_event->value;
        switch(event) {
            case KDL_EVENT_START_NODE:
                switch(depth) {
                    case 0: // config(_state) level
                        // reading new state
                        if (strlen(name_data) == 6 && memcmp(name_data, "config", 6) != 0) {
                            goto invalid_config;
                        }
                        // reading old state
                        if (strlen(name_data) == 12 && memcmp(name_data, "config_state", 12) != 0) {
                            goto invalid_config_state;
                        }
                        break;
                    case 1: // child level
                        node_d1 = string_copy((char* )name_data);
                        break;
                }
                depth += 1;
                break;
            case KDL_EVENT_END_NODE:
                depth -= 1;
                break;
            case KDL_EVENT_ARGUMENT:
                if (memcmp(node_d1, "aur_helper", 10) == 0) {
                    config->aur_helper = string_copy((char* )value.string.data);
                } else if (memcmp(node_d1, "active_host", 11) == 0){
                    struct host host = { .name=string_copy((char* )value.string.data) };       
                    config->active_host = host; 
                }
                break;
            case KDL_EVENT_PROPERTY:
                break;
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
    // no need to keep, since valuable data is stored in the args
    if (node_d1 != nullptr) {
        free_sized(node_d1, strlen(node_d1));
    }
    kdl_destroy_parser(parser); 
    return EXIT_SUCCESS;
    
    invalid_config:
        puts("Can't parse state from the config.kdl\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;

    invalid_config_state:
        puts("Can't parse state from the config_state.kdl\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;
}

int write_config_kdl(FILE* fid, struct config* config) {
    kdl_emitter_options e_opts = KDL_DEFAULT_EMITTER_OPTIONS;
    kdl_emitter* emitter = kdl_create_stream_emitter(&write_func, (void* )fid, &e_opts);

    kdl_emit_node(emitter, kdl_str_from_cstr("config_state"));
    kdl_start_emitting_children(emitter);
    if (config->aur_helper != nullptr) {
        kdl_emit_node(emitter, kdl_str_from_cstr("aur_helper"));
        kdl_value value = { .type=KDL_TYPE_STRING, .string=kdl_str_from_cstr(config->aur_helper) };
        kdl_emit_arg(emitter, &value);
    } else goto invalid_state;
    if (config->active_host.name != nullptr) {
        kdl_emit_node(emitter, kdl_str_from_cstr("active_host"));
        kdl_value value = { 
                    .type=KDL_TYPE_STRING,
                    .string=kdl_str_from_cstr(config->active_host.name) };
        kdl_emit_arg(emitter, &value);
    } else goto invalid_state;
    kdl_finish_emitting_children(emitter);
    kdl_emit_end(emitter);

    kdl_destroy_emitter(emitter);
    return EXIT_SUCCESS;

    invalid_state:
        puts("Not a valid config state for writing config_state.kdl\n");
        kdl_destroy_emitter(emitter);
        return EXIT_FAILURE;
}

int free_config(struct config config) {
    int ret;
    config.aur_helper = nullptr;
    ret = free_host(config.active_host);

    return ret; 
}
