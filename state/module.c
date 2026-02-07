#include <kdl/kdl.h> 
#include <string.h>
#include "state.h"
#include "state_utils.h"

int parse_module_kdl(FILE* fid, struct module* module)  {
    kdl_parser* parser = kdl_create_stream_parser(&read_func, (void* )fid, KDL_DEFAULTS);
    
    size_t depth = 0;
    char* node_d2 = nullptr;
    char* node_d3 = nullptr;
    bool eof = false;

    while(true) {
        kdl_event_data* next_event = kdl_parser_next_event(parser);
        kdl_event event = next_event->event;
        const char* name_data = next_event->name.data;
        kdl_value value = next_event->value;
        switch(event) {
            case KDL_EVENT_START_NODE:
                switch(depth) {
                    case 0: // module(_state) level
                        // reading new state
                        if (strlen(name_data) == 6 && memcmp(name_data, "module", 6) != 0) {
                            goto invalid_module;
                        }
                        // reading old state
                        if (strlen(name_data) == 12 && memcmp(name_data, "module_state", 12) != 0) {
                            goto invalid_module_state;
                        }
                        break;
                    case 1: // example_module level
                        break;
                    case 2: // dotfiles/packages/services/hooks level
                        node_d2 = string_copy((char* )name_data);
                        break;
                    case 3: // child level
                        if (memcmp(node_d2, "packages", 8) == 0) {
                            struct dynamic_array* packages = &module->packages;
                            char* package = string_copy((char* )name_data);
                            DYNAMIC_ARRAY_APPEND((*packages), package);
                        } else if (memcmp(node_d2, "aur_packages", 12) == 0) {
                            struct dynamic_array* aur_packages = &module->aur_packages;
                            char* aur_package = string_copy((char* )name_data);
                            DYNAMIC_ARRAY_APPEND((*aur_packages), aur_package);
                        } else if (memcmp(node_d2, "services", 8) == 0){
                            struct permissions* services = &module->services;
                            struct permission service = { 
                                                    .name=string_copy((char* )name_data),
                                                    .root=false }; // implicit root=#false
                            DYNAMIC_ARRAY_APPEND((*services), service);
                        } else if (memcmp(node_d2, "hooks", 4) == 0){
                            node_d3 = string_copy((char* )name_data);
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
                if (memcmp(node_d2, "dotfiles", 8) == 0) {
                    if (value.boolean) module->sync = true; // dotfiles sync=#true
                } else if (memcmp(node_d2, "hooks", 5) == 0) {
                    struct permissions* hooks = &module->hooks;
                    struct permission hook = { .name=string_copy(node_d3), .root=value.boolean };
                    DYNAMIC_ARRAY_APPEND((*hooks), hook);
                }
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
    
    invalid_module:
        puts("Can't parse state from the module.kdl\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;

    invalid_module_state:
        puts("Can't parse state from the module_state.kdl\n");
        kdl_destroy_parser(parser); 
        return EXIT_FAILURE;
}

int write_module_kdl(FILE* fid, struct module* module) {
    kdl_emitter_options e_opts = KDL_DEFAULT_EMITTER_OPTIONS;
    kdl_emitter* emitter = kdl_create_stream_emitter(&write_func, (void* )fid, &e_opts);

    kdl_emit_node(emitter, kdl_str_from_cstr("module_state"));
    kdl_start_emitting_children(emitter); // open module level
    if (module->name != nullptr) {
        kdl_emit_node(emitter, kdl_str_from_cstr(module->name));
        kdl_start_emitting_children(emitter); // open example_module level
        if (module->sync) {
            kdl_emit_node(emitter, kdl_str_from_cstr("dotfiles"));
            kdl_value value = { .type=KDL_TYPE_BOOLEAN, .boolean=true };
            kdl_emit_property(emitter, kdl_str_from_cstr("sync"), &value);
        }
        kdl_emit_node(emitter, kdl_str_from_cstr("packages"));
        kdl_start_emitting_children(emitter); // open packages level
        for (size_t i = 0; i < module->packages.count; ++i) {
            kdl_emit_node(emitter, kdl_str_from_cstr(module->packages.items[i]));
        }
        kdl_finish_emitting_children(emitter); // close packages level
        kdl_emit_node(emitter, kdl_str_from_cstr("aur_packages"));
        kdl_start_emitting_children(emitter); // open aur_packages level
        for (size_t i = 0; i < module->aur_packages.count; ++i) {
            kdl_emit_node(emitter, kdl_str_from_cstr(module->aur_packages.items[i]));
        }
        kdl_finish_emitting_children(emitter); // close aur_packages level
        kdl_emit_node(emitter, kdl_str_from_cstr("services"));
        kdl_start_emitting_children(emitter); // open services level
        for (size_t i = 0; i < module->services.count; ++i) {
            kdl_emit_node(emitter, kdl_str_from_cstr(module->services.items[i].name));
        }
        kdl_finish_emitting_children(emitter); // close services level
        kdl_emit_node(emitter, kdl_str_from_cstr("hooks"));
        kdl_start_emitting_children(emitter); // open hooks level
        for (size_t i = 0; i < module->hooks.count; ++i) {
            kdl_emit_node(emitter, kdl_str_from_cstr(module->hooks.items[i].name));
            kdl_value value = { .type=KDL_TYPE_BOOLEAN, .boolean=module->hooks.items[i].root };
            kdl_emit_property(emitter, kdl_str_from_cstr("root"), &value);
        }
        kdl_finish_emitting_children(emitter); // close hooks level
        kdl_finish_emitting_children(emitter); // close example_module level
    } else goto invalid_state;
    kdl_finish_emitting_children(emitter); // close module_state level
    kdl_emit_end(emitter);

    kdl_destroy_emitter(emitter);
    return EXIT_SUCCESS;

    invalid_state:
        puts("Not a valid module state for writing module_state.kdl\n");
        kdl_destroy_emitter(emitter);
        return EXIT_FAILURE;
}

// TODO: how to handle failure in the cleanup chain?
int free_module(struct module module) {
    DYNAMIC_ARRAY_FREE(module.packages);
    DYNAMIC_ARRAY_FREE(module.aur_packages);
    DYNAMIC_ARRAY_NAME_FREE(module.services);
    DYNAMIC_ARRAY_NAME_FREE(module.hooks);
    
    return EXIT_SUCCESS;
}
