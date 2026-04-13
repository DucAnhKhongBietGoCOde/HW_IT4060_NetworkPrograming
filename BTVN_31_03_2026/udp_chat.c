#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]){
    if(argc != 4){
        printf("Usage: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    char buf[BUF_SIZE];

    // Tạo socket UDP
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        perror("socket error");
        return EXIT_FAILURE;
    }

    // set non-blocking
    unsigned long ul = 1;
    ioctl(sock, FIONBIO, &ul);
    ioctl(STDIN_FILENO, FIONBIO, &ul);

    // Cấu hình local
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port_s);

    if(bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) == -1){
        perror("bind error");
        close(sock);
        return EXIT_FAILURE;
    }

    // Cấu hình đích
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_d);
    inet_pton(AF_INET, ip_d, &dest_addr.sin_addr);

    printf("UDP Chat started on port: %d...\n", port_s);

    while(1){
        // ===== RECEIVE =====
        struct sockaddr_in sender_addr;
        socklen_t addr_len = sizeof(sender_addr);

        int n = recvfrom(sock, buf, BUF_SIZE - 1, 0,
                         (struct sockaddr*)&sender_addr, &addr_len);

        if(n > 0){
            buf[n] = '\0';
            printf("\n[RECV] %s:%d: %s\n",
                   inet_ntoa(sender_addr.sin_addr),
                   ntohs(sender_addr.sin_port),
                   buf);
        } else if(n < 0 && errno != EWOULDBLOCK && errno != EAGAIN){
            perror("recvfrom error");
        }

        // ===== SEND =====
        int input_len = read(STDIN_FILENO, buf, BUF_SIZE - 1);

        if(input_len > 0){
            buf[input_len] = '\0';

            sendto(sock, buf, strlen(buf), 0,
                   (struct sockaddr*)&dest_addr,
                   sizeof(dest_addr));

            printf("[SEND]: %s\n", buf);
        } else if(input_len < 0 && errno != EWOULDBLOCK && errno != EAGAIN){
            perror("read error");
        }

        usleep(10000); 
    }

    close(sock);
    return 0;
}