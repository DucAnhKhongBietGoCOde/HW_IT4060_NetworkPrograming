#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define MAX 1024

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    listen(listener, 5);
    printf("Server listening on port %d...\n", PORT);

    int client = accept(listener, NULL, NULL);
    if (client < 0) {
        perror("accept");
        return 1;
    }

    char buffer[MAX];
    int len;

    printf("Received:\n");

    while ((len = recv(client, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[len] = '\0';
        printf("%s", buffer);
    }

    close(client);
    close(listener);
    return 0;
}