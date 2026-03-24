
/*
Usage: ./tcp_client <IP> <Port>
Example: ./tcp_client
Write a TCP client that connects to the server at the specified IP and port, receives messages from the server, and prints them to the console. The client should handle disconnections gracefully.
*/


#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {
    // Kiểm tra đối số dòng lệnh 
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    
    // Tạo socket TCP
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Kết nối đến server
    int ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    // Nhận dữ liệu từ server
    char buf[256];
    while (1) {
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) {
            printf("Server disconnected\n");
            break;
        }
        buf[n] = '\0';
        printf("Received: %s", buf);
    }

    close(sock);
}
    
