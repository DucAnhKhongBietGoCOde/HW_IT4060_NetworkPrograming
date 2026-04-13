#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>

#define PORT 8080
#define MAX 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr*)&server, sizeof(server));

    char buffer[MAX] = {0};

    // Lấy đường dẫn thư mục hiện tại
    char cwd[512];
    getcwd(cwd, sizeof(cwd));
    strcat(buffer, cwd);
    strcat(buffer, "\n");

    // Duyệt các file trong thư mục
    DIR *d = opendir(".");
    struct dirent *dir;

    while ((dir = readdir(d)) != NULL) {
        struct stat st;
        if (stat(dir->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
            char line[256];
            snprintf(line, sizeof(line), "%.200s - %lld bytes\n",
        dir->d_name, (long long)st.st_size);
            strcat(buffer, line);
        }
    }

    send(sock, buffer, strlen(buffer), 0);

    close(sock);
}