#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#define BUFFER_SIZE 8192

// Hàm xóa khoảng trắng và ký tự xuống dòng ở cuối chuỗi
void trim_newline(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

// Hàm phân tích các phản hồi và tin nhắn nhận được từ Server
void process_server_message(char *message) {
    trim_newline(message);
    if (strlen(message) == 0) return;

    // Tạo bản sao để tách token đầu tiên (Mã lỗi hoặc lệnh hệ thống)
    char temp[BUFFER_SIZE];
    strcpy(temp, message);
    
    char *cmd = strtok(temp, " ");
    char *remain = message + (cmd ? strlen(cmd) : 0);
    if (*remain == ' ') remain++; // Bỏ khoảng trắng đầu dòng dữ liệu còn lại

    // 1. Xử lý các mã phản hồi trạng thái từ Server
    if (strcmp(cmd, "100") == 0) {
        printf("[Hệ thống] Thao tác thành công.\n");
    } 
    else if (strcmp(cmd, "200") == 0) {
        printf("[Lỗi 200] Biệt danh (Nickname) đã có người sử dụng!\n");
    } 
    else if (strcmp(cmd, "201") == 0) {
        printf("[Lỗi 201] Biệt danh không hợp lệ (Chỉ chấp nhận chữ cái thường và số).\n");
    } 
    else if (strcmp(cmd, "202") == 0) {
        printf("[Lỗi 202] Không tìm thấy biệt danh đích trong phòng chat.\n");
    } 
    else if (strcmp(cmd, "203") == 0) {
        printf("[Từ chối 203] Bạn không có quyền OP (Chủ phòng) để thực hiện lệnh này.\n");
    } 
    else if (strcmp(cmd, "999") == 0) {
        printf("[Lỗi 999] Lỗi không xác định từ phía Server.\n");
    }
    
    // 2. Xử lý các gói tin Broadcast từ phòng chat gửi về
    else if (strcmp(cmd, "JOIN") == 0) {
        printf("--> Người dùng [%s] vừa tham gia vào phòng chat.\n", remain);
    } 
    else if (strcmp(cmd, "QUIT") == 0) {
        printf("<-- Người dùng [%s] đã rời khỏi phòng chat.\n", remain);
    } 
    else if (strcmp(cmd, "MSG") == 0) {
        char *sender = strtok(remain, " ");
        char *content = remain + (sender ? strlen(sender) : 0);
        if (*content == ' ') content++;
        printf("[%s]: %s\n", sender, content);
    } 
    else if (strcmp(cmd, "PMSG") == 0) {
        char *sender = strtok(remain, " ");
        char *content = remain + (sender ? strlen(sender) : 0);
        if (*content == ' ') content++;
        printf("[Tin riêng từ %s]: %s\n", sender, content);
    } 
    else if (strcmp(cmd, "TOPIC") == 0) {
        char *op_nick = strtok(remain, " ");
        char *topic_name = remain + (op_nick ? strlen(op_nick) : 0);
        if (*topic_name == ' ') topic_name++;
        printf("[*] Chủ phòng [%s] đã đổi chủ đề phòng thành: \"%s\"\n", op_nick, topic_name);
    } 
    else if (strcmp(cmd, "KICK") == 0) {
        char *kicked_nick = strtok(remain, " ");
        char *op_nick = remain + (kicked_nick ? strlen(kicked_nick) : 0);
        if (*op_nick == ' ') op_nick++;
        printf("[!] Người dùng [%s] đã bị trục xuất khỏi phòng bởi OP [%s].\n", kicked_nick, op_nick);
    } 
    else if (strcmp(cmd, "OP") == 0) {
        printf("[*] Người dùng [%s] đã được phong cấp làm Chủ phòng (OP).\n", remain);
    } 
    else {
        // Dự phòng cho các chuỗi thô khác
        printf("%s\n", message);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Sử dụng: %s <IP_Server> <Cổng_Server> <Biệt_danh>\n", argv[0]);
        printf("Ví dụ: %s 127.0.0.1 8080 alice\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    char *nickname = argv[3];

    int server_socket;
    struct sockaddr_in server_addr;

    // 1. Khởi tạo Socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Không thể tạo Socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Địa chỉ IP không hợp lệ hoặc không hỗ trợ.\n");
        return 1;
    }

    // 2. Kết nối tới Server
    printf("[*] Đang kết nối tới Server %s:%d...\n", server_ip, server_port);
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Kết nối thất bại");
        return 1;
    }
    printf("[+] Kết nối thành công.\n");

    // 3. Đóng gói và gửi lệnh JOIN ngay sau khi kết nối thành công
    char join_cmd[128];
    snprintf(join_cmd, sizeof(join_cmd), "JOIN %s\n", nickname);
    send(server_socket, join_cmd, strlen(join_cmd), 0);

    // Cài đặt bộ đệm nhận để xử lý phân mảnh gói tin TCP
    char read_buffer[BUFFER_SIZE];
    int buffer_len = 0;
    memset(read_buffer, 0, BUFFER_SIZE);

    fd_set readfds;

    printf("====================== CHAT ROOM ======================\n");
    printf(" Hướng dẫn gõ lệnh:\n");
    printf("   - Gửi tin chung: Chỉ cần gõ nội dung rồi nhấn Enter\n");
    printf("   - Lệnh hệ thống: Gõ đúng cú pháp (Ví dụ: TOPIC <tên>, QUIT, KICK <tên>)\n");
    printf("=======================================================\n");

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);    // Theo dõi bàn phím để đọc dữ liệu người dùng gõ
        FD_SET(server_socket, &readfds);   // Theo dõi socket để đọc dữ liệu đổ về từ Server

        int max_fd = (server_socket > STDIN_FILENO) ? server_socket : STDIN_FILENO;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("Lỗi Select");
        }

        // --- TRƯỜNG HỢP 1: NGƯỜI DÙNG GÕ TỪ BÀN PHÍM ---
        // --- TRƯỜNG HỢP 1: NGƯỜI DÙNG GÕ TỪ BÀN PHÍM ---
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char input_buf[BUFFER_SIZE];
            if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
                trim_newline(input_buf); // Xóa \n tạm thời để check chuỗi rỗng và so sánh lệnh
                
                if (strlen(input_buf) == 0) continue;

                char send_buf[BUFFER_SIZE + 64];

                if (strncmp(input_buf, "JOIN ", 5) == 0 || 
                    strncmp(input_buf, "MSG ", 4) == 0 || 
                    strncmp(input_buf, "PMSG ", 5) == 0 || 
                    strncmp(input_buf, "OP ", 3) == 0 || 
                    strncmp(input_buf, "KICK ", 5) == 0 || 
                    strncmp(input_buf, "TOPIC ", 6) == 0 || 
                    strcmp(input_buf, "QUIT") == 0) {
                    
                    // Gõ lệnh thô: Phải có \n ở cuối chuỗi gửi đi
                    snprintf(send_buf, sizeof(send_buf), "%s\n", input_buf);
                } else {
                    // Chat thường: Tự động bọc "MSG " + nội dung + "\n" ở cuối
                    // BẮT BUỘC phải có chữ \n ở đây để Server nhận biết kết thúc dòng
                    snprintf(send_buf, sizeof(send_buf), "MSG %s\n", input_buf);
                    
                    printf("[Tôi]: %s\n", input_buf);
                    fflush(stdout);
                }

                // Gửi toàn bộ chuỗi ĐÃ CÓ \n lên Server
                send(server_socket, send_buf, strlen(send_buf), 0);

                if (strcmp(input_buf, "QUIT") == 0) {
                    printf("[*] Đang đóng kết nối và thoát...\n");
                    break;
                }
            }
        }

        // --- TRƯỜNG HỢP 2: SERVER TRẢ DỮ LIỆU VỀ ---
        if (FD_ISSET(server_socket, &readfds)) {
            char temp_buffer[BUFFER_SIZE];
            memset(temp_buffer, 0, BUFFER_SIZE);
            
            int valread = recv(server_socket, temp_buffer, sizeof(temp_buffer) - 1, 0);

            if (valread <= 0) {
                // Server ngắt kết nối đột ngột hoặc tắt nguồn
                printf("\n[-] Mất kết nối tới Server Chat.\n");
                break;
            }

            // Dồn dữ liệu mới nhận vào bộ đệm tích lũy xử lý stream dòng \n
            if (buffer_len + valread < BUFFER_SIZE) {
                memcpy(read_buffer + buffer_len, temp_buffer, valread);
                buffer_len += valread;
                read_buffer[buffer_len] = '\0';
            } else {
                printf("[!] Lỗi: Tràn bộ đệm nhận dữ liệu.\n");
                break;
            }

            // Cắt chuỗi theo từng dòng ngăn cách bởi '\n'
            char *line_start = read_buffer;
            char *newline_ptr;

            while ((newline_ptr = strchr(line_start, '\n')) != NULL) {
                *newline_ptr = '\0'; // Chuyển thành ký tự kết thúc chuỗi để xử lý từng dòng đơn lẻ
                process_server_message(line_start);
                line_start = newline_ptr + 1;
            }

            // Dịch chuyển phần dữ liệu chưa hoàn chỉnh (nếu có) lên đầu bộ đệm cho lượt nhận kế tiếp
            int consumed_bytes = line_start - read_buffer;
            int remaining_bytes = buffer_len - consumed_bytes;
            if (remaining_bytes > 0) {
                memmove(read_buffer, line_start, remaining_bytes);
                buffer_len = remaining_bytes;
                read_buffer[buffer_len] = '\0';
            } else {
                buffer_len = 0;
                memset(read_buffer, 0, BUFFER_SIZE);
            }
        }
    }

    close(server_socket);
    return 0;
}