#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define FTP_SERVER "lebavui.io.vn"
#define FTP_PORT 21

#define USERNAME "user_20235256" 
#define PASSWORD "525601"       

int main() {
    int control_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    getaddrinfo(FTP_SERVER, "21", &hints, &res);
    if (connect(control_sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connecting to control channel failed");
        return 1;
    }
    freeaddrinfo(res);

    char buf[4096];
    int len;
    
    len = recv(control_sock, buf, sizeof(buf) - 1, 0);
    buf[len] = 0; printf("[Server] %s", buf);

    char cmd[256];
    sprintf(cmd, "USER %s\r\n", USERNAME);
    send(control_sock, cmd, strlen(cmd), 0);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); 
    buf[len] = 0; 
    printf("[Server] %s", buf);

    sprintf(cmd, "PASS %s\r\n", PASSWORD);
    send(control_sock, cmd, strlen(cmd), 0);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); 
    buf[len] = 0; 
    printf("[Server] %s", buf);

    if (strstr(buf, "230") == NULL) {
        printf("Login failed! Please check your username/password.\n");
        close(control_sock);
        return 1;
    }

    strcpy(cmd, "PASV\r\n");
    send(control_sock, cmd, strlen(cmd), 0);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); 
    buf[len] = 0; 
    printf("[Server] %s", buf);

    int h1, h2, h3, h4, p1, p2;
    char *pasv_info = strchr(buf, '(');
    sscanf(pasv_info, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    int data_port = p1 * 256 + p2;
    char data_ip[64];
    sprintf(data_ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    printf("[Client Log] Data channel opened at IP: %s, Port: %d\n", data_ip, data_port);

    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in data_addr = {0};
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = inet_addr(data_ip);
    data_addr.sin_port = htons(data_port);
    
    if (connect(data_sock, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        perror("Connecting to data channel failed");
        close(control_sock);
        return 1;
    }

    strcpy(cmd, "LIST\r\n");
    send(control_sock, cmd, strlen(cmd), 0);
    
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); 
    buf[len] = 0; 
    printf("[Server] %s", buf);

    char list_buf[8192] = {0};
    char temp[1024];
    while ((len = recv(data_sock, temp, sizeof(temp) - 1, 0)) > 0) {
        temp[len] = 0;
        strcat(list_buf, temp);
    }
    close(data_sock);

    len = recv(control_sock, buf, sizeof(buf) - 1, 0); 
    buf[len] = 0; 
    printf("[Server] %s", buf);

    char question_file[128] = {0};
    char *match = strstr(list_buf, "question_");
    if (match == NULL) {
        printf("File question_xxxxxx.txt not found!\n");
        close(control_sock);
        return 1;
    }
    sscanf(match, "%s", question_file);
    question_file[strcspn(question_file, "\r\n")] = 0;
    printf("[Client Log] Found question file: %s\n", question_file);

    strcpy(cmd, "PASV\r\n");
    send(control_sock, cmd, strlen(cmd), 0);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); 
    buf[len] = 0; 
    printf("[Server] %s", buf);

    pasv_info = strchr(buf, '(');
    sscanf(pasv_info, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    data_port = p1 * 256 + p2;
    
    data_sock = socket(AF_INET, SOCK_STREAM, 0);
    data_addr.sin_port = htons(data_port);
    if (connect(data_sock, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        perror("Connecting to data channel failed");
        close(control_sock);
        return 1;
    }

    sprintf(cmd, "RETR %s\r\n", question_file);
    send(control_sock, cmd, strlen(cmd), 0);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); 
    buf[len] = 0; 
    printf("[Server] %s", buf);

    char question_content[256] = {0};
    int total_bytes = 0;
    while ((len = recv(data_sock, temp, sizeof(temp) - 1, 0)) > 0) {
        temp[len] = 0;
        strcat(question_content, temp);
        total_bytes += len;
    }
    close(data_sock);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); 
    buf[len] = 0; 
    printf("[Server] %s", buf);
    
    printf("[Client Log] Content of question file: %s\n", question_content);

    question_content[strcspn(question_content, "\r\n")] = 0;
    int content_len = strlen(question_content);
    
    char answer_content[256] = {0};
    for (int i = 0; i < content_len; i++) {
        answer_content[i] = question_content[content_len - 1 - i];
    }
    answer_content[content_len] = '\0';
    printf("[Client Log] Content of answer file: %s\n", answer_content);

    char answer_file[128];
    char *id_part = strchr(question_file, '_');
    sprintf(answer_file, "answer%s", id_part);
    printf("[Client Log] Name of answer file: %s\n", answer_file);

    strcpy(cmd, "PASV\r\n");
    send(control_sock, cmd, strlen(cmd), 0);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); buf[len] = 0; printf("[Server] %s", buf);

    pasv_info = strchr(buf, '(');
    sscanf(pasv_info, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    data_port = p1 * 256 + p2;

    data_sock = socket(AF_INET, SOCK_STREAM, 0);
    data_addr.sin_port = htons(data_port);

    if (connect(data_sock, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        perror("Connecting to data channel failed");
        close(control_sock);
        return 1;
    }

    sprintf(cmd, "STOR %s\r\n", answer_file);
    send(control_sock, cmd, strlen(cmd), 0);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); buf[len] = 0; printf("[Server] %s", buf);

    send(data_sock, answer_content, strlen(answer_content), 0);
    close(data_sock);

    len = recv(control_sock, buf, sizeof(buf) - 1, 0); buf[len] = 0; printf("[Server] %s", buf);

    strcpy(cmd, "QUIT\r\n");
    send(control_sock, cmd, strlen(cmd), 0);
    len = recv(control_sock, buf, sizeof(buf) - 1, 0); buf[len] = 0; printf("[Server] %s", buf);
  
    printf("\n[Client Log] Write an answer file %s successfully!\n", answer_file);
    close(control_sock);
    return 0;
}