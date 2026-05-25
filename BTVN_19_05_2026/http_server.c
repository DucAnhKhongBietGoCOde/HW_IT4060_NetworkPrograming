
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 8
#define PORT 8080

int listener;
pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker_thread(void *);

int main()
{
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
        return 1;
    listen(listener, 10);

    printf("Prethreading HTTP Server listening on port %d with %d threads...\n", PORT, NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    getchar();
    close(listener);
    return 0;
}

void *worker_thread(void *params)
{
    char buf[1024];
    while (1)
    {
        pthread_mutex_lock(&accept_mutex);
        int client = accept(listener, NULL, NULL);
        pthread_mutex_unlock(&accept_mutex);

        if (client == -1)
            continue;

        printf("[Log] Thread %lu accepted client socket: %d\n", (unsigned long)pthread_self(), client);
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret > 0)
        {
            buf[ret] = 0;
            printf("Request:\n%s\n", buf);
            char response[2048];
            char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                        "<html><body><h1>Hello everyone</h1>"
                        "<p>Processed by Thread ID: %lu</p></body></html>";
            sprintf(response, msg, (unsigned long)pthread_self());
            send(client, response, strlen(response), 0);
        }
        close(client);
    }
    return NULL;
}