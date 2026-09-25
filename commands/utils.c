#include <pwd.h>
#include <unistd.h>
 
char* get_user() {
    struct passwd* pwd = getpwuid(geteuid());
    return pwd->pw_name;
}
