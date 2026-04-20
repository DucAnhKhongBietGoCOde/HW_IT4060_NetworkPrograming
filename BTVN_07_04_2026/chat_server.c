#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8080
#define MAX_NAME 50
#define BUF_SIZE 1024

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) return 1;

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) return 1;
    if (listen(listener, 10) < 0) return 1;

    fd_set master, readfds;
    FD_ZERO(&master);
    FD_SET(listener, &master);
    int maxfd = listener;

    char client_ids[FD_SETSIZE][MAX_NAME];
    memset(client_ids, 0, sizeof(client_ids));
    char buf[BUF_SIZE];

    printf("Chat server listening on port %d...\n", PORT);

    while (1) {
        readfds = master;
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) continue;

        for (int i = 0; i <= maxfd; i++) {
            if (!FD_ISSET(i, &readfds)) continue;

            if (i == listener) {
                int client = accept(listener, NULL, NULL);
                if (client < 0) continue;
                FD_SET(client, &master);
                if (client > maxfd) maxfd = client;
                char *msg = "Nhap theo cu phap: client_id: NguyenVanA\n";
                send(client, msg, strlen(msg), 0);
            } else {
                int ret = recv(i, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    if (client_ids[i][0]) {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "%s da roi phong chat.\n", client_ids[i]);
                        for (int j = 0; j <= maxfd; j++) {
                            if (FD_ISSET(j, &master) && j != listener && j != i && client_ids[j][0]) send(j, msg, strlen(msg), 0);
                        }
                    }
                    close(i);
                    FD_CLR(i, &master);
                    client_ids[i][0] = 0;
                } else {
                    buf[ret] = 0;
                    buf[strcspn(buf, "\r\n")] = 0;

                    if (client_ids[i][0] == 0) {
                        char key[MAX_NAME], name[MAX_NAME];
                        if (sscanf(buf, "%49[^:]: %49s", key, name) == 2 && strcmp(key, "client_id") == 0){
                            strncpy(client_ids[i], name, MAX_NAME - 1);
                            send(i, "Dang nhap thanh cong!\n", 22, 0);
                            char joinmsg[128];
                            snprintf(joinmsg, sizeof(joinmsg), "%s da tham gia phong chat.\n", client_ids[i]);
                            for (int j = 0; j <= maxfd; j++) {
                                if (FD_ISSET(j, &master) && j != listener && j != i && client_ids[j][0]) send(j, joinmsg, strlen(joinmsg), 0);
                            }
                        } else {
                            send(i, "Sai cu phap! Dung: abc: NguyenVanA\n", 36 , 0);
                        }
                    } else {
                        time_t now = time(NULL);
                        struct tm *t = localtime(&now);
                        char timestr[64];
                        strftime(timestr, sizeof(timestr), "%Y/%m/%d %I:%M:%S%p", t);

                        char out[2048];
                        snprintf(out, sizeof(out), "%s %s: %s\n", timestr, client_ids[i], buf);

                        for (int j = 0; j <= maxfd; j++) {
                            if (FD_ISSET(j, &master) && j != listener && j != i && client_ids[j][0]) {
                                send(j, out, strlen(out), 0);
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
