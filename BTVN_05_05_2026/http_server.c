#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define WORKERS 4

void worker_process(int listener)
{
    while (1)
    {
        int client =
            accept(listener, NULL, NULL);

        if (client < 0)
        {
            perror("accept");
            continue;
        }

        printf("Worker %d accepted client\n",
               getpid());

        char buf[2048];

        int ret = recv(client, buf, sizeof(buf) - 1, 0);

        if (ret <= 0)
        {
            close(client);
            continue;
        }

        buf[ret] = '\0';

        printf("\n===== HTTP REQUEST =====\n");
        printf("%s\n", buf);

        char *msg =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<html>"
            "<body>"
            "<h1>Xin chao cac ban</h1>"
            "<h2>Prefork HTTP Server</h2>"
            "</body>"
            "</html>";

        send(client, msg,strlen(msg), 0);

        close(client);
    }
}

int main()
{
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

    for (int i = 0; i < WORKERS; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            worker_process(listener);
            exit(0);
        }
    }

    while (1)
    {
        pause();
    }
    close(listener);
    return 0;
}