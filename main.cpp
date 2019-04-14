#include <libsbox.h>
#include <iostream>
using namespace std;

void fatal_handler(const char *msg) {
    printf("Fatal error: %s\n", msg);
}

int main() {
    libsbox::fatal_handler = fatal_handler;
    libsbox::init();
    cout << libsbox::box_id << endl;

    char buf[1024];
    getcwd(buf, 1024);
    cout << buf << endl;

    while (true) {}

    libsbox::die("Everything is OK!");
}
