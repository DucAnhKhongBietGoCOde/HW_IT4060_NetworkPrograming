#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

void *client_thread(void *);

int check_login(char *user, char *pass)
{
    FILE *f = fopen("users.txt", "r");
    if (f == NULL)
        return 0;
    char line[64], cred[64];
    sprintf(cred, "%s %s", user, pass);
    while (fgets(line, sizeof(line), f))
    {
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0;
        if (line[strlen(line) - 1] == '\r')
            line[strlen(line) - 1] = 0;
        if (strcmp(line, cred) == 0)
        {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

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

    printf("Telnet Server listening on port 8080...\n");

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

    char buf[256];
    int logged_in = 0;
    char *login_prompt = "Please enter your username and password:\n";
    send(client, login_prompt, strlen(login_prompt), 0);

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

        if (!logged_in)
        {
            char user[32], pass[32];
            int n = sscanf(buf, "%s %s", user, pass);

            if (n != 2)
            {
                char *msg = "Invalid syntax. Use: user pass\n";
                send(client, msg, strlen(msg), 0);
                printf("Client %d: Invalid syntax (n=%d)\n", client, n);
            }
            else if (check_login(user, pass))
            {
                logged_in = 1;
                char *msg = "Login successful. You can now execute commands.\n";
                send(client, msg, strlen(msg), 0);
                printf("Client %d: Login successful for user '%s'\n", client, user);
            }
            else
            {
                char *msg = "Login failed. Try again.\n";
                send(client, msg, strlen(msg), 0);
                printf("Client %d: Login failed for user '%s'\n", client, user);
            }
        }
        else
        {
            char filename[64], cmd[512], out_buf[512];
            sprintf(filename, "out_%lu.txt", (unsigned long)pthread_self());
            sprintf(cmd, "%s > %s 2>&1", buf, filename);
            system(cmd);

            FILE *f = fopen(filename, "r");
            if (f)
            {
                while (fgets(out_buf, sizeof(out_buf), f) != NULL)
                {
                    send(client, out_buf, strlen(out_buf), 0);
                }
                fclose(f);
            }
        }
    }
    printf("Client %d disconnected\n", client);
    close(client);
    return NULL;
}