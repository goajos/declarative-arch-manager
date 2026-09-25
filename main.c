#include <stdio.h>
#include <kdl/kdl.h>


static size_t read_func(void* userdata, char* buf, size_t bufsize)
{
    FILE* fid = (FILE*)userdata;
    return fread(buf, 1, bufsize, fid);
}

// install src/utils/ckdl-cat ~/.local/bin
// gcc main.c -o main -lkdl -lm -Wall -Wpedantic -Werror
int main()
{
    printf("Hello world!\n");
    FILE* fid = fopen("arch.kdl", "r");

    kdl_parser* parser = kdl_create_stream_parser(&read_func, (void*)fid, KDL_DEFAULTS);


    bool eof = false;
    while (1) {
        kdl_event_data* event = kdl_parser_next_event(parser);
        char const* event_name = NULL;
        kdl_event kevent = event->event;
        const char* data = event->name.data;
        kdl_type type = event->value.type;
        switch(kevent) {
            case KDL_EVENT_EOF:
                event_name = "KDL_EVENT_EOF";
                // printf("%s\n", event_name);
                eof = true;
                break;
            case KDL_EVENT_START_NODE:
                event_name = "KDL_EVENT_START_NODE";
                printf("%s\n", event_name);
                if (data) printf("%s\n", data);
                break;
            case KDL_EVENT_END_NODE:
                event_name = "KDL_EVENT_END_NODE";
                printf("%s\n", event_name);
                if (data) printf("%s\n", data);
                break;
            case KDL_EVENT_ARGUMENT:
                event_name = "KDL_EVENT_ARGUMENT";
                printf("%s\n", event_name);
                if (data) printf("%s\n", data);
                if (type == KDL_TYPE_STRING) {
                    kdl_owned_string kstring = kdl_clone_str(&event->value.string);
                    printf("%s\n", kstring.data);
                    kdl_free_string(&kstring);
                } else if (type == KDL_TYPE_NUMBER) {
                    kdl_number knum = event->value.number; 
                    kdl_number_type knum_type = knum.type;
                    switch(knum_type) {
                        case KDL_NUMBER_TYPE_INTEGER:
                            printf("%lld\n", knum.integer);
                        case KDL_NUMBER_TYPE_FLOATING_POINT:
                            printf("%f\n", knum.floating_point);
                        case KDL_NUMBER_TYPE_STRING_ENCODED:
                            kdl_owned_string kstring = kdl_clone_str(&knum.string);
                            printf("%s\n", kstring.data);
                            kdl_free_string(&kstring);
                    }
                }
                break;
            case KDL_EVENT_COMMENT:
                event_name = "KDL_EVENT_COMMENT";
                // printf("%s\n", event_name);
                break;
            case KDL_EVENT_PROPERTY:
                event_name = "KDL_EVENT_PROPERTY";
                // printf("%s\n", event_name);
                break;
            case KDL_EVENT_PARSE_ERROR:
                event_name = "KDL_EVENT_PARSE_ERROR";
                // printf("%s\n", event_name);
                break;
        }

        if (eof) break;

        // kdl_event_data* event = kdl_parser_next_event(parser);
    }

    fclose(fid);
    kdl_destroy_parser(parser);
    return 0;
}
