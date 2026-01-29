# define debug_active(xs)\
    printf("%s count: %d\n", #xs, (int)xs.count);\
    for (int i = 0; i < (int)xs.count; ++i) {\
        printf("%s - is active: %b\n", xs.items[i].item.data, xs.items[i].active);\
    }

# define debug_active_user_type(xs)\
    printf("%s count: %d\n", #xs, (int)xs.count);\
    for (int i = 0; i < (int)xs.count; ++i) {\
        printf("%s\n  - is active: %b\n  - is user_type: %b\n", xs.items[i].item.data, xs.items[i].active, xs.items[i].user_type);\
    }

