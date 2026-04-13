#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 64

#define STATE_ASK_NAME 0
#define STATE_ASK_MSSV 1

typedef struct {
    int fd;
    int state;
    char name[BUF_SIZE];
    char mssv[BUF_SIZE];
} Client;

Client clients[MAX_CLIENTS];
int nclients = 0;

void generate_email(char *name, char *mssv, char *email) {
    char words[10][50];
    int count = 0;

    char tmp[100];
    strcpy(tmp, name);

    char *token = strtok(tmp, " \n");
    while (token != NULL) {
        strcpy(words[count++], token);
        token = strtok(NULL, " \n");
    }

    // tên (từ cuối)
    char firstName[50];
    strcpy(firstName, words[count - 1]);
    for (int i = 0; firstName[i]; i++)
        firstName[i] = tolower(firstName[i]);

    // chữ cái đầu họ + đệm
    char initials[50] = "";
    for (int i = 0; i < count - 1; i++) {
        int len = strlen(initials);
        initials[len] = tolower(words[i][0]);
        initials[len + 1] = '\0';
    }

    // xử lý MSSV
    mssv[strcspn(mssv, "\n")] = 0;
    char *id = mssv;
    if (strncmp(mssv, "20", 2) == 0)
        id = mssv + 2;

    sprintf(email, "%s.%s%s@sis.hust.edu.vn", firstName, initials, id);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);

    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Server non-blocking dang chay (port 8080)...\n");

    char buf[BUF_SIZE];

    while (1) {
        // accept
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd != -1) {
            if (nclients >= MAX_CLIENTS) {
                close(client_fd);
                continue;
            }

            ioctl(client_fd, FIONBIO, &ul);

            clients[nclients].fd = client_fd;
            clients[nclients].state = STATE_ASK_NAME;

            send(client_fd, "Nhap ho ten:\n", 14, 0);

            nclients++;
        }

        // xử lý client
        for (int i = 0; i < nclients; i++) {
            int fd = clients[i].fd;

            int len = recv(fd, buf, BUF_SIZE - 1, 0);

            if (len == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    continue;
                // lỗi thật
                close(fd);
                clients[i] = clients[nclients - 1];
                nclients--;
                i--;
                continue;
            }

            if (len == 0) {
                // client đóng
                close(fd);
                clients[i] = clients[nclients - 1];
                nclients--;
                i--;
                continue;
            }

            buf[len] = 0;

            if (clients[i].state == STATE_ASK_NAME) {
                strcpy(clients[i].name, buf);
                send(fd, "Nhap MSSV:\n", 12, 0);
                clients[i].state = STATE_ASK_MSSV;

            } else if (clients[i].state == STATE_ASK_MSSV) {
                strcpy(clients[i].mssv, buf);

                char email[BUF_SIZE];
                generate_email(clients[i].name,
                               clients[i].mssv,
                               email);

                send(fd, "Email cua ban la:\n", 19, 0);
                send(fd, email, strlen(email), 0);

                close(fd);

                // remove client
                clients[i] = clients[nclients - 1];
                nclients--;
                i--;
            }
        }
    }

    return 0;
}