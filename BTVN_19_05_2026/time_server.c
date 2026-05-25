/*******************************************************************************
 * @file    time_server_thread.c
 * @brief   Ứng dụng Time Server sử dụng kĩ thuật multithread
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

void *client_thread(void *);

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
        return 1;
    listen(listener, 5);

    printf("Time Server listening on port 8080...\n");

    while (1)
    {
        int client = accept(listener, NULL, NULL);
        printf("New client connected: %d\n", client);

        int *pclient = malloc(sizeof(int));
        *pclient = client;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, pclient);
        pthread_detach(thread_id);
    }
    close(listener);
    return 0;
}

void *client_thread(void *params)
{
    int client = *(int *)params;
    free(params);

    char *welcome_msg = "Welcome! This is the Time Server.\n"
                        "Please enter command in the format: GET_TIME [format]\n"
                        "Supported formats:\n"
                        "- dd/mm/yyyy\n"
                        "- dd/mm/yy\n"
                        "- mm/dd/yyyy\n"
                        "- mm/dd/yy\n";
    send(client, welcome_msg, strlen(welcome_msg), 0);

    char buf[256];
    while (1)
    {
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0)
            break;

        buf[len] = 0;
        if (buf[len - 1] == '\n')
            buf[len - 1] = 0;
        if (buf[strlen(buf) - 1] == '\r')
            buf[strlen(buf) - 1] = 0;
        printf("Log from %d: %s\n", client, buf);

        char cmd[16], format[32], tmp[16];
        int n = sscanf(buf, "%s %s %s", cmd, format, tmp);

        if (n != 2 || strcmp(cmd, "GET_TIME") != 0)
        {
            char *err = "Error: Wrong command. Use: GET_TIME [format]\n";
            send(client, err, strlen(err), 0);
            continue;
        }

        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char res[64];
        int valid = 1;

        if (strcmp(format, "dd/mm/yyyy") == 0)
            strftime(res, sizeof(res), "%d/%m/%Y\n", t);
        else if (strcmp(format, "dd/mm/yy") == 0)
            strftime(res, sizeof(res), "%d/%m/%y\n", t);
        else if (strcmp(format, "mm/dd/yyyy") == 0)
            strftime(res, sizeof(res), "%m/%d/%Y\n", t);
        else if (strcmp(format, "mm/dd/yy") == 0)
            strftime(res, sizeof(res), "%m/%d/%y\n", t);
        else
            valid = 0;

        if (valid)
            send(client, res, strlen(res), 0);
        else
            send(client, "Error: Time format not supported.\n", 36, 0);
    }
    printf("Client %d disconnected\n", client);
    close(client);
    return NULL;
}