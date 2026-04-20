#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

int main () {

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    char *server_ip = "127.0.0.1";
    int server_port = 8000;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s:%d\n", server_ip, server_port);
    printf("Enter content (type 'exit' to quit):\n");

    char buffer[1024];
    while (1) {
        printf("> ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
        buffer[strcspn(buffer, "\n")] = 0;     
        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        int bytes_sent = send(client, buffer, strlen(buffer), 0);
        if (bytes_sent < 0) {
            perror("send() failed");
            break;
        }
    }

    close(client);

    return 0;
}