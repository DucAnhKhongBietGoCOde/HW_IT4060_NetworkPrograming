#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <hello file> <output file>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    char *hello_file = argv[2];
    char *output_file = argv[3];

    // 1. Tạo socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // 2. THÊM SO_REUSEADDR VÀO SOCKET
    // Cho phép khởi động lại server ngay lập tức trên cùng một cổng
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_fd);
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 3. Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    // 4. Listen
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(1);
    }

    printf("Server đang chạy tại cổng %d (chế độ REUSEADDR đã bật)...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        // Gửi lời chào từ file
        FILE *f_hello = fopen(hello_file, "r");
        if (f_hello) {
            char hello_buf[BUFFER_SIZE];
            while (fgets(hello_buf, sizeof(hello_buf), f_hello)) {
                send(client_fd, hello_buf, strlen(hello_buf), 0);
            }
            fclose(f_hello);
        }

        // Nhận dữ liệu và ghi vào file
        FILE *f_output = fopen(output_file, "a");
        if (f_output) {
            char recv_buf[BUFFER_SIZE];
            int bytes_received;
            while ((bytes_received = recv(client_fd, recv_buf, sizeof(recv_buf), 0)) > 0) {
                fwrite(recv_buf, 1, bytes_received, f_output);
            }
            fclose(f_output);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}