#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/un.h>

using namespace std;

static void optimizer_protect(void *p) {
    asm volatile("" : : "g"(p) : "memory");
}

static void optimizer_protect_all() {
    asm volatile("" : : : "memory");
}

void massert_internal(bool cond, int line, const char *msg) {
    if (!cond) {
        cout << "Assertation (" << msg << ") [line:" << line << "] failed: " << strerror(errno) << endl;
        exit(1);
    }
}

#define massert(cond) massert_internal(cond, __LINE__, #cond)

#define SOCKET_NAME "/etc/libsboxd/socket"
#define BUFFER_SIZE 1024

sockaddr_un addr;

string msg;

void run() {
    int client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    massert(client_socket >= 0);

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    massert(connect(client_socket, (const struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == 0);

    int cnt = send(client_socket, msg.c_str(), msg.size() + 1, 0);
    massert(cnt == msg.size() + 1);

    char buf[1024];
    cnt = recv(client_socket, buf, 1023, 0);
    buf[cnt] = '\0';
    massert(cnt > 0);
    // cout << buf << endl;0

    std::cout << buf << std::endl;

    massert(close(client_socket) == 0);
}

int main(int argc, char *argv[]) {
    int cnt;
    if (argc != 2) {
        cnt = 1;
    } else {
        cnt = atoi(argv[1]);
    }
    string line;
    while (getline(cin, line)) {
        msg += line;
        msg += '\n';
    }
    for (int i = 0; i < cnt; ++i) {
        std::cout << i + 1 << std::endl;
        run();
    }
}
