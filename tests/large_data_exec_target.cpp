#include <cstring>

const int sz = 1024 * 1024;

int a[sz] = {0}; // 4 MB

int main() {
    memset(a, 0, sz * sizeof(int));
    while(true) {};
}
