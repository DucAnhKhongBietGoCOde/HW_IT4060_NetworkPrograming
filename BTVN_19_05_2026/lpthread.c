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
#include <pthread.h>
#include <sys/stat.h>
#include <sys/select.h>



#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int client1;
    int client2;
} ChatPair;

int waiting_client = -1;
pthread_mutex_t queue_mutex;

void *chat_room(void *arg)
{
    ChatPair *pair = (ChatPair *)arg;

    int c1 = pair->client1;
    int c2 = pair->client2;

    char buffer[BUFFER_SIZE];

    fd_set readfds;

    printf("Chat room created: %d <-> %d\n", c1, c2);

    while (1)
    {
        FD_ZERO(&readfds);

        FD_SET(c1, &readfds);
        FD_SET(c2, &readfds);

        int maxfd = (c1 > c2 ? c1 : c2) + 1;

        int activity = select(maxfd, &readfds, NULL, NULL, NULL);

        if (activity < 0)
            break;

        // client1 gửi
        if (FD_ISSET(c1, &readfds))
        {
            int n = recv(c1, buffer, sizeof(buffer), 0);

            if (n <= 0)
            {
                printf("Client %d disconnected\n", c1);
                close(c1);
                close(c2);
                break;
            }

            send(c2, buffer, n, 0);
        }

        // client2 gửi
        if (FD_ISSET(c2, &readfds))
        {
           int n = recv(c2, buffer, sizeof(buffer), 0);

            if (n <= 0)
            {
                printf("Client %d disconnected\n", c2);
                close(c1);
                close(c2);
                break;
            }

            send(c1, buffer, n, 0);
        }
    }

    free(pair);

    pthread_exit(NULL);
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

    pthread_mutex_init(&queue_mutex, NULL);

    printf("Server is listening on port 8080...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        if (client == -1) {
            perror("accept() failed");
            continue;
        }

        printf("New client connected: %d\n", client);

        pthread_mutex_lock(&queue_mutex);

        if (waiting_client == -1) {
            waiting_client = client;
            char *waitmsg = "Waiting for another client...\n";
            send(client, waitmsg, strlen(waitmsg), 0);
        } else {
            ChatPair *pair = malloc(sizeof(ChatPair));
            pair->client1 = waiting_client;
            pair->client2 = client;

            pthread_t thread_id;
            pthread_create(&thread_id, NULL, chat_room, pair);
            pthread_detach(thread_id);

            char *msg = "Partner found! You can start chatting.\n";
    
            send(waiting_client, msg, strlen(msg), 0);
            send(client, msg, strlen(msg), 0);

            waiting_client = -1;
        }
        pthread_mutex_unlock(&queue_mutex);
    }
    close(listener);
    return 0;
}