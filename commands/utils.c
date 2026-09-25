#include <pwd.h>
#include <unistd.h>

static char* get_user() {
    struct passwd* pwd = getpwuid(geteuid());
    return pwd->pw_name;
}

