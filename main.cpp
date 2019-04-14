#include <libsbox.h>
#include <iostream>
using namespace std;

int main() {
    libsbox::init();
    cout << libsbox::box_id << endl;

    char buf[1024];
    getcwd(buf, 1024);
    cout << buf << endl;

    int x;
    cin >> x;

    libsbox::die("Everything is OK!");
}
