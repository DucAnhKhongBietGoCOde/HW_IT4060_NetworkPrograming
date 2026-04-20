#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

int main() {
    int server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8000);

    bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr));

    char buf[1024];
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    while (1) {
        int ret = recvfrom(server, buf, sizeof(buf), 0,
            (struct sockaddr *)&client_addr, &client_addr_len);
        if (ret <= 0) continue;
        buf[ret] = '\0';
        printf("Received %d bytes from %s:%d - Content: %s\n", ret, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf);
        sendto(server, buf, ret, 0, (struct sockaddr *)&client_addr, client_addr_len);
    }

    close(server);
    return 0;
}