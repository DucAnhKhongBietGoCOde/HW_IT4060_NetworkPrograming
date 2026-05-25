/*******************************************************************************
 * @file    chat_server_thread.c
 * @brief   Multi-thread Chat Server
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define MAX_CLIENTS 1000
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char *id;
    char *name;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_thread(void *);

void get_current_time(char *buf, int size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, size, "%Y/%m/%d %I:%M:%S%p", t);
}

int main()
{
    signal(SIGPIPE, SIG_IGN);
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        return 1;
    }

    listen(listener, 5);
    memset(clients, 0, sizeof(clients));

    printf("Chat Server listening on port 8080...\n");

    while (1)
    {
        int client = accept(listener, NULL, NULL);
        if (client < 0)
        {
            perror("accept");

            continue;
        }

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
    int client_fd = *(int *)params;
    free(params);

    int my_index = -1;
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd == 0)
        {
            clients[i].fd = client_fd;
            clients[i].id = NULL;
            clients[i].name = NULL;
            my_index = i;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    if (my_index == -1)
    {
        char *msg = "Server full.\n";
        send(client_fd, msg, strlen(msg), 0);
        close(client_fd);
        return NULL;
    }

    char *welcome = "Nhap theo cu phap: client_id: client_name\n";

    send(client_fd, welcome, strlen(welcome), 0);

    char buf[BUFFER_SIZE];

    while (1)
    {
        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (len <= 0)
            break;

        buf[len] = 0;

        if (buf[len - 1] == '\n')
            buf[len - 1] = 0;

        if (strlen(buf) > 0 && buf[strlen(buf) - 1] == '\r')
        {
            buf[strlen(buf) - 1] = 0;
        }

        printf("Log from %d: %s\n", client_fd, buf);
        pthread_mutex_lock(&clients_mutex);
        char *my_id = clients[my_index].id;
        pthread_mutex_unlock(&clients_mutex);

        // chưa login
        if (my_id == NULL)
        {
            char id[50];
            char name[50];

            int n = sscanf(buf, "%49[^:]: %49s", id, name);

            if (n == 2)
            {
                pthread_mutex_lock(&clients_mutex);
                clients[my_index].id = malloc(strlen(id) + 1);
                strcpy(clients[my_index].id, id);
                clients[my_index].name = malloc(strlen(name) + 1);
                strcpy(clients[my_index].name, name);

                pthread_mutex_unlock(&clients_mutex);
                char *ok = "Dang nhap thanh cong.\n";
                send(client_fd, ok, strlen(ok), 0);
                printf("Client login: %s (%s)\n", id, name);
            }
            else
            {
                char *err = "ERROR Sai cu phap. Dung: client_id: client_name\n";
                send(client_fd, err, strlen(err), 0);
            }
        }

        // đã login
        else
        {
            char timebuf[100];
            get_current_time(timebuf, sizeof(timebuf));
            char out_buf[BUFFER_SIZE + 200];
            snprintf(out_buf, sizeof(out_buf), "%s %s: %s\n", timebuf, clients[my_index].name, buf);
            pthread_mutex_lock(&clients_mutex);

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].fd != 0 &&
                    clients[i].id != NULL &&
                    i != my_index)
                {
                    send(clients[i].fd, out_buf, strlen(out_buf), 0);
                }
            }

            pthread_mutex_unlock(&clients_mutex);

            printf("%s", out_buf);
        }
    }

    pthread_mutex_lock(&clients_mutex);

    if (clients[my_index].id != NULL)
        free(clients[my_index].id);

    if (clients[my_index].name != NULL)
        free(clients[my_index].name);

    clients[my_index].fd = 0;
    clients[my_index].id = NULL;
    clients[my_index].name = NULL;

    pthread_mutex_unlock(&clients_mutex);

    printf("Client %d disconnected\n", client_fd);

    close(client_fd);

    return NULL;
}