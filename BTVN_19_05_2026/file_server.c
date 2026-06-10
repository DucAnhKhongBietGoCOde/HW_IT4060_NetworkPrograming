#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>


#define PORT 8080
#define FOLDER "files"


void send_file_list(int client)
{
    DIR *dir = opendir(FOLDER);

    if (dir == NULL)
    {
        char *err = "ERROR No files to download\r\n";
        send(client,
             err,
             28,
             0);
        return;
    }

    struct dirent *entry;

    char list[4096] = "";

    int count = 0;

    // đếm file
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            count++;
        }
    }

    // không có file
    if (count == 0)
    {
        closedir(dir);
        char *err = "ERROR No files to download\r\n";

        send(client,
             err,
             28,
             0);

        return;
    }

    // gửi OK N
    sprintf(list,
            "OK %d\r\n",
            count);

    rewinddir(dir);

    // gửi tên file
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            strcat(list, entry->d_name);

            strcat(list, "\r\n");
        }
    }

    strcat(list, "\r\n");

    send(client,
         list,
         strlen(list),
         0);

    closedir(dir);
}

void send_file(int client, char *filename)
{
    char path[512];

    snprintf(path,
             sizeof(path),
             "%s/%s",
             FOLDER,
             filename);

    FILE *f = fopen(path, "rb");

    // file không tồn tại
    if (f == NULL)
    {
        char *err =
        "ERROR File not found\r\n";

        send(client,
             err,
             strlen(err),
             0);

        return;
    }

    // lấy size file
    fseek(f, 0, SEEK_END);

    long size = ftell(f);

    rewind(f);

    // gửi OK N
    char header[100];

    sprintf(header,
            "OK %ld\r\n",
            size);

    send(client,
         header,
         strlen(header),
         0);

    // gửi nội dung file
    char buf[1024];

    int n;

    while ((n = fread(buf,
                      1,
                      sizeof(buf),
                      f)) > 0)
    {
        send(client,
             buf,
             n,
             0);
    }

    fclose(f);
}

void handle_client(int client)
{
    char filename[256];

    // gửi danh sách file
    send_file_list(client);

    while (1)
    {
        int ret = recv(client,
                       filename,
                       sizeof(filename) - 1,
                       0);

        if (ret <= 0)
            break;

        filename[ret] = '\0';

        filename[strcspn(filename,
                         "\r\n")] = 0;

        printf("Client requests: %s\n",
               filename);

        send_file(client,
                  filename);
    }

    close(client);
}

void sigchld_handler(int signum) {
    (void)signum; 
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    
    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Server is listening on port 8080...\n");
    signal(SIGCHLD, sigchld_handler);

    while (1) {
        struct sockaddr_in client_addr;
        
        socklen_t client_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        if (client == -1) {
            perror("accept() failed");
            continue;
        }
        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (fork() == 0) {
            close(listener);
            handle_client(client);
            exit(0);
        } else {
            close(client); 
        }
    }
    close(listener);
    return 0;
}