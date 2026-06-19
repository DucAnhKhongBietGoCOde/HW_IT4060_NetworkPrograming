#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 8192

typedef struct {
    int socket_fd;
    char nickname[50];
    bool is_op;
    char read_buffer[BUFFER_SIZE];
    int buffer_len;
} Client;

Client clients[MAX_CLIENTS];
char current_topic[256] = "No topic set yet";

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket_fd = 0;
        memset(clients[i].nickname, 0, sizeof(clients[i].nickname));
        clients[i].is_op = false;
        memset(clients[i].read_buffer, 0, BUFFER_SIZE);
        clients[i].buffer_len = 0;
    }
}

void send_to(int client_fd, const char *message) {
    send(client_fd, message, strlen(message), 0);
}

// Broadcast gửi đến TẤT CẢ các kết nối đang mở, không quan tâm đã có nick hay chưa
void broadcast_all(const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd > 0) {
            send_to(clients[i].socket_fd, message);
        }
    }
}

// Broadcast gửi đến mọi socket đang mở NGOẠI TRỪ socket người ra lệnh
// Chỉ broadcast tới các socket đã đặt tên (đã JOIN thành công) và không phải người gửi
void broadcast_except(int except_fd, const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd > 0 && clients[i].socket_fd != except_fd) {
            
            if (strlen(clients[i].nickname) > 0) {
                send_to(clients[i].socket_fd, message);
            }
        }
    }
}

bool has_operator() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd > 0 && clients[i].is_op) {
            return true;
        }
    }
    return false;
}

bool is_valid_nickname(const char *nick) {
    if (nick == NULL || strlen(nick) == 0) return false;
    for (int i = 0; nick[i] != '\0'; i++) {
        if (!islower(nick[i]) && !isdigit(nick[i])) {
            return false; 
        }
    }
    return true;
}

bool is_nickname_in_use(const char *nick) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd > 0 && strcmp(clients[i].nickname, nick) == 0) {
            return true;
        }
    }
    return false;
}

void disconnect_client(int index, bool send_quit_broadcast, int except_fd) {
    if (clients[index].socket_fd <= 0) return;

    int client_fd = clients[index].socket_fd;
    char leaving_nick[50];
    strncpy(leaving_nick, clients[index].nickname, sizeof(leaving_nick));

    if (send_quit_broadcast && strlen(leaving_nick) > 0) {
        char quit_msg[128];
        snprintf(quit_msg, sizeof(quit_msg), "QUIT %s\n", leaving_nick);
        if (except_fd > 0) {
            broadcast_except(except_fd, quit_msg);
        } else {
            broadcast_all(quit_msg);
        }
    }

    close(client_fd);
    clients[index].socket_fd = 0;
    memset(clients[index].nickname, 0, sizeof(clients[index].nickname));
    bool was_op = clients[index].is_op;
    clients[index].is_op = false;
    memset(clients[index].read_buffer, 0, BUFFER_SIZE);
    clients[index].buffer_len = 0;

    if (was_op && !has_operator()) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket_fd > 0 && strlen(clients[i].nickname) > 0) {
                clients[i].is_op = true;
                char op_msg[128];
                snprintf(op_msg, sizeof(op_msg), "OP %s\n", clients[i].nickname);
                broadcast_all(op_msg);
                break;
            }
        }
    }
}

char* get_message_content(char *original_line, int skip_tokens) {
    int spaces_found = 0;
    char *p = original_line;
    while (*p != '\0') {
        if (*p == ' ') {
            spaces_found++;
            if (spaces_found == skip_tokens) {
                p++;
                while (*p == ' ') p++;
                return p;
            }
        }
        p++;
    }
    return p;
}

void execute_single_command(int client_index, char *line) {
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
    if (len == 0) return;

    char original_line[BUFFER_SIZE];
    strcpy(original_line, line);

    char *args[50];
    int arg_count = 0;
    char *token = strtok(line, " ");
    while (token != NULL && arg_count < 50) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }

    if (arg_count == 0) return;
    char *cmd = args[0];
    int client_fd = clients[client_index].socket_fd;

    // ================== 1. LỆNH JOIN ==================
    if (strcmp(cmd, "JOIN") == 0) {
        if (arg_count < 2) {
            send_to(client_fd, "999 UNKNOWN ERROR\n");
            return;
        }
        char *nick = args[1];
        
        if (!is_valid_nickname(nick)) {
            send_to(client_fd, "201 INVALID NICK NAME\n");
            return;
        }
        if (is_nickname_in_use(nick)) {
            send_to(client_fd, "200 NICKNAME IN USE\n");
            return;
        }

        strncpy(clients[client_index].nickname, nick, sizeof(clients[client_index].nickname) - 1);
        if (!has_operator()) {
            clients[client_index].is_op = true;
        }

        // 1. Phản hồi thành công về chính họ
        send_to(client_fd, "100 OK\n");

        // 2. Gửi broadcast thông báo JOIN tới tất cả mọi socket KHÁC đang kết nối
        char broadcast_msg[128];
        snprintf(broadcast_msg, sizeof(broadcast_msg), "JOIN %s\n", nick);
        broadcast_except(client_fd, broadcast_msg);
    }
    
    // ================== 2. LỆNH MSG ==================
    else if (strcmp(cmd, "MSG") == 0) {
        if (arg_count < 2) {
            send_to(client_fd, "999 UNKNOWN ERROR\n");
            return;
        }
        char *msg_content = get_message_content(original_line, 1);
        
        send_to(client_fd, "100 OK\n");

        char response[BUFFER_SIZE + 128];
        snprintf(response, sizeof(response), "MSG %s %s\n", clients[client_index].nickname, msg_content);
        broadcast_except(client_fd, response);
    }

    // ================== 3. LỆNH PMSG ==================
    else if (strcmp(cmd, "PMSG") == 0) {
        if (arg_count < 3) {
            send_to(client_fd, "999 UNKNOWN ERROR\n");
            return;
        }
        char *target_nick = args[1];
        char *msg_content = get_message_content(original_line, 2);

        int target_fd = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket_fd > 0 && strcmp(clients[i].nickname, target_nick) == 0) {
                target_fd = clients[i].socket_fd;
                break;
            }
        }

        if (target_fd != -1) {
            send_to(client_fd, "100 OK\n");
            char response[BUFFER_SIZE + 128];
            snprintf(response, sizeof(response), "PMSG %s %s\n", clients[client_index].nickname, msg_content);
            send_to(target_fd, response);
        } else {
            send_to(client_fd, "202 UNKNOWN NICKNAME\n");
        }
    }

    // ================== 4. LỆNH KICK ==================
    else if (strcmp(cmd, "KICK") == 0) {
        if (arg_count < 2) {
            send_to(client_fd, "999 UNKNOWN ERROR\n");
            return;
        }
        // SỬA LỖI QUYỀN OP: Kiểm tra quyền OP trước khi làm bất cứ việc gì khác
        if (!clients[client_index].is_op) {
            send_to(client_fd, "203 DENIED\n");
            return;
        }

        char *kicked_nick = args[1];
        int target_idx = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket_fd > 0 && strcmp(clients[i].nickname, kicked_nick) == 0) {
                target_idx = i;
                break;
            }
        }

        if (target_idx != -1) {
            send_to(client_fd, "100 OK\n");
            
            char response[128];
            snprintf(response, sizeof(response), "KICK %s %s\n", kicked_nick, clients[client_index].nickname);
            
            // Broadcast đến các thành viên khác
            broadcast_except(client_fd, response);
            
            // Đóng kết nối của người bị kick
            disconnect_client(target_idx, false, 0);
        } else {
            send_to(client_fd, "202 UNKNOWN NICKNAME\n");
        }
    }

    // ================== 5. LỆNH OP ==================
    else if (strcmp(cmd, "OP") == 0) {
        if (arg_count < 2) {
            send_to(client_fd, "999 UNKNOWN ERROR\n");
            return;
        }
        if (!clients[client_index].is_op) {
            send_to(client_fd, "203 DENIED\n");
            return;
        }

        char *target_nick = args[1];
        int target_idx = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket_fd > 0 && strcmp(clients[i].nickname, target_nick) == 0) {
                target_idx = i;
                break;
            }
        }

        if (target_idx != -1) {
            clients[target_idx].is_op = true;
            // Thu hồi quyền OP của người cũ nếu quy định yêu cầu chuyển giao hoàn toàn quyền lực
            clients[client_index].is_op = false; 

            send_to(client_fd, "100 OK\n");
            
            char response[128];
            snprintf(response, sizeof(response), "OP %s\n", target_nick);
            broadcast_except(client_fd, response);
        } else {
            send_to(client_fd, "202 UNKNOWN NICKNAME\n");
        }
    }

    // ================== 6. LỆNH TOPIC ==================
    else if (strcmp(cmd, "TOPIC") == 0) {
        if (arg_count < 2) {
            send_to(client_fd, "999 UNKNOWN ERROR\n");
            return;
        }
        if (!clients[client_index].is_op) {
            send_to(client_fd, "203 DENIED\n");
            return;
        }

        char *msg_content = get_message_content(original_line, 1);
        strncpy(current_topic, msg_content, sizeof(current_topic) - 1);

        send_to(client_fd, "100 OK\n");

        char response[512];
        snprintf(response, sizeof(response), "TOPIC %s %s\n", clients[client_index].nickname, current_topic);
        broadcast_except(client_fd, response);
    }

    // ================== 7. LỆNH QUIT ==================
    else if (strcmp(cmd, "QUIT") == 0) {
        send_to(client_fd, "100 OK\n");
        disconnect_client(client_index, true, client_fd);
    }
    else {
        send_to(client_fd, "999 UNKNOWN ERROR\n");
    }
}

void handle_client_data(int index) {
    int sd = clients[index].socket_fd;
    char temp_buffer[BUFFER_SIZE];
    memset(temp_buffer, 0, BUFFER_SIZE);

    int valread = recv(sd, temp_buffer, BUFFER_SIZE - 1, 0);

    if (valread <= 0) {
        disconnect_client(index, true, 0);
        return;
    }

    if (clients[index].buffer_len + valread < BUFFER_SIZE) {
        memcpy(clients[index].read_buffer + clients[index].buffer_len, temp_buffer, valread);
        clients[index].buffer_len += valread;
        clients[index].read_buffer[clients[index].buffer_len] = '\0';
    } else {
        disconnect_client(index, true, 0);
        return;
    }

    char *line_start = clients[index].read_buffer;
    char *newline_ptr;

    while ((newline_ptr = strchr(line_start, '\n')) != NULL) {
        *newline_ptr = '\0'; 
        execute_single_command(index, line_start);
        line_start = newline_ptr + 1;
    }

    int consumed_bytes = line_start - clients[index].read_buffer;
    int remaining_bytes = clients[index].buffer_len - consumed_bytes;
    if (remaining_bytes > 0) {
        memmove(clients[index].read_buffer, line_start, remaining_bytes);
        clients[index].buffer_len = remaining_bytes;
        clients[index].read_buffer[clients[index].buffer_len] = '\0';
    } else {
        clients[index].buffer_len = 0;
        memset(clients[index].read_buffer, 0, BUFFER_SIZE);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    init_clients();

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[*] Protocol-Perfect Server running on port %d...\n", PORT);

    fd_set readfds;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket_fd;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("Select error\n");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept error");
                exit(EXIT_FAILURE);
            }

            bool added = false;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket_fd == 0) {
                    clients[i].socket_fd = new_socket;
                    memset(clients[i].nickname, 0, sizeof(clients[i].nickname));
                    clients[i].is_op = false;
                    clients[i].buffer_len = 0;
                    memset(clients[i].read_buffer, 0, BUFFER_SIZE);
                    added = true;
                    break;
                }
            }
            if (!added) {
                close(new_socket);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket_fd;
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                handle_client_data(i);
            }
        }
    }
    return 0;
}