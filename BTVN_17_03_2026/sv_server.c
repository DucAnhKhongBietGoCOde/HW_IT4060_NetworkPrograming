#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_BUF 1024

void get_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", t);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <log_file>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *log_file = argv[2];

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    listen(server, 5);
    printf("Server listening on port %d...\n", port);

    FILE *f = fopen(log_file, "a");
    if (!f) {
        perror("log file");
        return 1;
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int client = accept(server, (struct sockaddr*)&client_addr, &len);
        if (client < 0) {
            perror("accept");
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));

        printf("Client connected: %s\n", ip);

        char buf[MAX_BUF];

        while (1) {
            int n = recv(client, buf, MAX_BUF - 1, 0);
            if (n <= 0) break;

            buf[n] = 0;

            // remove newline
            buf[strcspn(buf, "\n")] = 0;

            char time_str[20];
            get_timestamp(time_str, sizeof(time_str));

            // in ra màn hình
            printf("%s %s %s\n", ip, time_str, buf);

            // ghi file đúng format đề
            fprintf(f, "%s %s %s\n", ip, time_str, buf);
            fflush(f);
        }

        close(client);
        printf("Client disconnected\n");
    }

    fclose(f);
    close(server);
    return 0;
}