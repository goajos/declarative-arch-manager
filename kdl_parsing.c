#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kdl/kdl.h>
#include "utils.c"

void parse_config_kdl(FILE *fid, Hosts *hosts)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   const char *node_data = nullptr;
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                node_depth += 1;
                if (node_depth == 1) {
                    // TODO: set up a goto error block
                    if (strcmp(name_data, "config") != 0) printf("goto Error: Not a valid config...\n"); 
                } else if (node_depth == 2) {
                    node_data = name_data;
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN) {
                    char *dest = copy_data(node_data);
                    if (dest) {
                        Host host = { dest, boolean };
                        da_append((*hosts), host);
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

   kdl_destroy_parser(parser); // parser cleans up
}

void parse_host_kdl(FILE *fid, Modules *modules, Services *services, Hooks *hooks)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   char *node_data_d2 = nullptr;
   const char *node_data_d3 = nullptr;
   bool user_type_flag;
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                node_depth += 1;
                if (node_depth == 1) {
                    // TODO: set up a go to error block
                    if (strcmp(name_data, "host") != 0) printf("goto Error: Not a valid host...\n"); 
                    else user_type_flag = false; // false for system
                } else if (node_depth == 2) {
                    // manual clean up before copying new data
                    node_data_d2 = copy_data(name_data);
                } else if (node_depth == 3) {
                    node_data_d3 = name_data;
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN) {
                    char *dest = copy_data(node_data_d3);
                    if (dest) {
                        if (strcmp(node_data_d2, "modules") == 0) {
                            Module module = { dest, boolean };
                            da_append((*modules), module);
                        } else if (strcmp(node_data_d2, "services") == 0) {
                            Service service = { dest, boolean, user_type_flag };
                            da_append((*services), service);
                        } else if (strcmp(node_data_d2, "hooks") == 0) {
                            Hook hook = { dest, boolean, user_type_flag };
                            da_append((*hooks), hook);
                        }
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

   if (node_data_d2 != nullptr) {
        free(node_data_d2); // manual clean up for copied data
        node_data_d2 = nullptr;
    }
   kdl_destroy_parser(parser); // parser cleans the rest
}

void parse_module_kdl(FILE *fid, Packages *packages, Services *services, Hooks *hooks)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   char *node_data_d2 = nullptr;
   const char *node_data_d3 = nullptr;
   bool user_type_flag;
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                node_depth += 1;
                if (node_depth == 1) {
                    // TODO: set up a go to error block
                    if (strcmp(name_data, "module") != 0) printf("goto Error: Not a valid module...\n"); 
                    else user_type_flag = true; // true for user 
                } else if (node_depth == 2) {
                    // manual clean up before copying new data
                    node_data_d2 = copy_data(name_data);
                } else if (node_depth == 3) {
                    node_data_d3 = name_data;
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN) {
                    char *dest = copy_data(node_data_d3);
                    if (dest) {
                        if (strcmp(node_data_d2, "packages") == 0) {
                            Package package = { dest, boolean };
                            da_append((*packages), package);
                        } else if (strcmp(node_data_d2, "services") == 0) {
                            Service service = { dest, boolean, user_type_flag };
                            da_append((*services), service);
                        } else if (strcmp(node_data_d2, "hooks") == 0) {
                            Hook hook = { dest, boolean, user_type_flag };
                            da_append((*hooks), hook);
                        }
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

   if (node_data_d2 != nullptr) {
        free(node_data_d2); // manual clean up for copied data
        node_data_d2 = nullptr;
    }
   kdl_destroy_parser(parser); // parser cleans the rest
}

