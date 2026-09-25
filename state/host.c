#include <kdl/kdl.h> 
#include <string.h>
#include "state.h"
#include "state_utils.h"

int parse_host_kdl(FILE* fid, struct host* host)  {
    kdl_parser* parser = kdl_create_stream_parser(&read_func, (void* )fid, KDL_DEFAULTS);
    
    size_t depth = 0;
    char* node_d2 = nullptr;
    bool eof = false;

    while(true) {
        kdl_event_data* next_event = kdl_parser_next_event(parser);
        kdl_event event = next_event->event;
        const char* name_data = next_event->name.data;
        switch(event) {
            case KDL_EVENT_START_NODE:
                switch(depth) {
                    case 0: // host(_state) level
                        // reading new state
                        if (strlen(name_data) == 4 && memcmp(name_data, "host", 4) != 0) {
                            goto invalid_host;
                        }
                        // reading old state
                        if (strlen(name_data) == 10 && memcmp(name_data, "host_state", 10) != 0) {
                            goto invalid_host_state;
                        }
                        break;
                    case 1: // example_host level
                        break;
                    case 2: // modules/services level
                        node_d2 = string_copy((char* )name_data);
                        break;
                    case 3: // child level
                        if (memcmp(node_d2, "modules", 7) == 0) {
                            struct module module = { .name=string_copy((char* )name_data) };
                            DYNAMIC_ARRAY_APPEND(host->modules, module);
                        } else if (memcmp(node_d2, "services", 8) == 0){
                            char* service = string_copy((char* )name_data); // implicit root=#true
                            DYNAMIC_ARRAY_APPEND(host->root_services, service);
                        }
                        break;
                }
                depth += 1;
                break;
            case KDL_EVENT_END_NODE:
                depth -= 1;
                break;
            case KDL_EVENT_ARGUMENT:
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

    kdl_destroy_parser(parser); 
    return EXIT_SUCCESS;
    
    invalid_host:
        puts("Can't parse state from the host.kdl\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;

    invalid_host_state:
        puts("Can't parse state from the host_state.kdl\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;
}

int write_host_kdl(FILE* fid, struct host* host) {
    kdl_emitter_options e_opts = KDL_DEFAULT_EMITTER_OPTIONS;
    kdl_emitter* emitter = kdl_create_stream_emitter(&write_func, (void* )fid, &e_opts);

    kdl_emit_node(emitter, kdl_str_from_cstr("host_state"));
    kdl_start_emitting_children(emitter); // open host_state level
    if (host->name != nullptr) {
        kdl_emit_node(emitter, kdl_str_from_cstr(host->name));
        kdl_start_emitting_children(emitter); // open example_host level
        kdl_emit_node(emitter, kdl_str_from_cstr("modules"));
        kdl_start_emitting_children(emitter); // open modules level
        for (size_t i = 0; i < host->modules.count; ++i) {
            kdl_emit_node(emitter, kdl_str_from_cstr(host->modules.items[i].name));
        }
        kdl_finish_emitting_children(emitter); // close modules level
        kdl_emit_node(emitter, kdl_str_from_cstr("services"));
        kdl_start_emitting_children(emitter); // open services level
        for (size_t i = 0; i < host->root_services.count; ++i) {
            kdl_emit_node(emitter, kdl_str_from_cstr(host->root_services.items[i]));
        }
        kdl_finish_emitting_children(emitter); // close services level
        kdl_finish_emitting_children(emitter); // close example_host level
    } else goto invalid_state;
    kdl_finish_emitting_children(emitter); // clsoe host_state level
    kdl_emit_end(emitter);

    kdl_destroy_emitter(emitter);
    return EXIT_SUCCESS;

    invalid_state:
        puts("Not a valid host state for writing host_state.kdl\n");
        kdl_destroy_emitter(emitter);
        return EXIT_FAILURE;
}

int free_host(struct host host) {
    int ret;
    for (size_t i = 0; i < host.modules.count; ++i) {
        ret = free_module(host.modules.items[i]);
    }
    MODULES_FREE(host.modules); // also free the modules itself
    DYNAMIC_ARRAY_FREE(host.root_services);

    return ret;
}
