#include <stdio.h>
#include <kdl/kdl.h>
#include "array.c"
#include "memory.c"
#include "../context.h"

Context parse_config_kdl(FILE *fid, Context context)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   String name = { .data = nullptr };
   String node_d2 = { .data = nullptr };
   String node_d3 = { .data = nullptr };
   String config = String("config");
   String aur_helper_str = String("aur_helper");
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
        long long integer = parsed_event->value.number.integer;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                name = String((char *)name_data);
                node_depth += 1;
                if (node_depth == 1) {
                    // TODO: set up a goto error block
                    if (!string_equal(name, config)) printf("goto Error: Not a valid config...\n");
                } else if (node_depth == 2) {
                    node_d2 = string_copy(name);
                } else if (node_depth == 3) {
                    node_d3 = string_copy(name);
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN || value_type == KDL_TYPE_NUMBER) {
                    if (integer > 1) integer = 1; // ensure integer is always 0 or 1
                    else if (integer < 0) integer = 0;
                    if (value_type == KDL_TYPE_NUMBER && integer == 1) boolean = true;
                    if (string_equal(node_d2, aur_helper_str)) {
                        if (boolean) context.aur_helper = node_d3;
                    } else {
                        Host host = { node_d2, boolean };
                        array_append((context.hosts), host);
                    }
                }
                break;
            } else if (event == KDL_EVENT_EOF) {
                eof = true;
                break;
            }
        }

        if (eof) break; // outer while break
    }

   if (node_d2.data != nullptr) {
        free_sized(node_d2.data, node_d2.len); // manual clean up for copied data
        node_d2.data = nullptr;
    }
    kdl_destroy_parser(parser); // parser cleans up

    return context;
}

Context parse_host_kdl(FILE *fid, Context context)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   String name = { .data = nullptr };
   String node_d2 = { .data = nullptr };
   String node_d3 = { .data = nullptr };
   String host = String("host");
   String modules_str = String("modules");
   String services_str = String("services");
   String hooks_str = String("hooks");
   bool user_type_flag;
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
        long long integer = parsed_event->value.number.integer;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                name = String((char *)name_data);
                node_depth += 1;
                if (node_depth == 1) {
                    // TODO: set up a go to error block
                    if (!string_equal(name, host)) printf("goto Error: Not a valid host...\n");
                    else user_type_flag = false; // false for system
                } else if (node_depth == 2) {
                    node_d2 = string_copy(name);
                } else if (node_depth == 3) {
                    node_d3 = string_copy(name);
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN || value_type == KDL_TYPE_NUMBER) {
                    if (integer > 1) integer = 1; // ensure integer is always 0 or 1
                    else if (integer < 0) integer = 0;
                    if (value_type == KDL_TYPE_NUMBER && integer == 1) boolean = true;
                    if (string_equal(node_d2, modules_str)) {
                        Module module = { node_d3, boolean };
                        array_append((context.modules), module);
                    } else if (string_equal(node_d2, services_str)) {
                        Service service = { node_d3, boolean, user_type_flag };
                        array_append((context.services), service);
                    } else if (string_equal(node_d2, hooks_str)) {
                        Hook hook = { node_d3, boolean, user_type_flag };
                        array_append((context.hooks), hook);
                    }
                }
                break;
            } else if (event == KDL_EVENT_EOF) {
                eof = true;
                break;
            }
        }

        if (eof) break; // outer while break
   }

   if (node_d2.data != nullptr) {
        free_sized(node_d2.data, node_d2.len); // manual clean up for copied data
        node_d2.data = nullptr;
    }
   kdl_destroy_parser(parser); // parser cleans the rest
                            
   return context;
}

Context parse_module_kdl(FILE *fid, Context context)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   String name = { .data = nullptr };
   String node_d2 = { .data = nullptr };
   String node_d3 = { .data = nullptr };
   String module = String("module");
   String packages_str = String("packages");
   String aur_packages_str = String("aur_packages");
   String services_str = String("services");
   String hooks_str = String("hooks");
   bool user_type_flag;
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
        long long integer = parsed_event->value.number.integer;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                name = String((char *)name_data);
                node_depth += 1;
                if (node_depth == 1) {
                    // TODO: set up a go to error block
                    if (!string_equal(name, module)) printf("goto Error: Not a valid module...\n"); 
                    else user_type_flag = true; // true for user 
                } else if (node_depth == 2) {
                    // manual clean up before copying new data
                    node_d2 = string_copy(name);
                } else if (node_depth == 3) {
                    node_d3 = string_copy(name);
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN || value_type == KDL_TYPE_NUMBER) {
                    if (integer > 1) integer = 1; // ensure integer is always 0 or 1
                    else if (integer < 0) integer = 0;
                    if (value_type == KDL_TYPE_NUMBER && integer == 1) boolean = true;
                    if (string_equal(node_d2, packages_str)) {
                        Package package = { node_d3, boolean };
                        array_append((context.packages), package);
                    }else if (string_equal(node_d2, aur_packages_str)) {
                        Package package = { node_d3, boolean };
                        array_append((context.aur_packages), package);
                    } else if (string_equal(node_d2, services_str)) {
                        Service service = { node_d3, boolean, user_type_flag };
                        array_append((context.services), service);
                    } else if (string_equal(node_d2, hooks_str)) {
                        Hook hook = { node_d3, boolean, user_type_flag };
                        array_append((context.hooks), hook);
                    }
                }
                break;
            } else if (event == KDL_EVENT_EOF) {
                eof = true;
                break;
            }
        }

        if (eof) break; // outer while break
   }

   if (node_d2.data != nullptr) {
        free_sized(node_d2.data, node_d2.len); // manual clean up for copied data
        node_d2.data = nullptr;
    }
   kdl_destroy_parser(parser); // parser cleans the rest
                            
   return context;
}

