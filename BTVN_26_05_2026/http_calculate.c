#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 9090

int main()
{
    int server = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server, (struct sockaddr *)&addr, sizeof(addr));

    listen(server, 5);

    printf("Server running on port %d\n", PORT);

    while (1)
    {
        int client = accept(server, NULL, NULL);

        char buf[4096] = {0};

        int len = recv(client, buf, sizeof(buf) - 1, 0);

        if (len <= 0){
            close(client);
            continue;
        }

        buf[len] = '\0';

        printf("\n========== REQUEST ==========\n");
        printf("%s\n", buf);

        if (strstr(buf, "GET /favicon.ico"))
        {
            char *resp =
                "HTTP/1.1 404 Not Found\r\n"
                "Connection: close\r\n\r\n";

            send(client, resp, strlen(resp), 0);

            close(client);
            continue;
        }

        double a = 0;
        double b = 0;

        char op[20] = "";

        /* GET */
        if (strncmp(buf, "GET", 3) == 0)
        {
            char *query = strchr(buf, '?');
            if (query) {
                sscanf(query, "?a=%lf&b=%lf&op=%19[^ ]", &a, &b, op);
            }
        }
        /* POST */
        else if (strncmp(buf, "POST", 4) == 0)
        {
            char *body = strstr(buf, "\r\n\r\n");
            if (body) {
                body += 4;
                printf("BODY = %s\n", body);
                sscanf(body, "a=%lf&b=%lf&op=%19s", &a, &b, op);
            }
        }

        printf("a = %.2lf\n", a);
        printf("b = %.2lf\n", b);
        printf("op = %s\n", op);

        double result = 0;
        int error = 0;

        if (strcmp(op, "add") == 0){
            result = a + b;
        } else if (strcmp(op, "sub") == 0){
            result = a - b;
        } else if (strcmp(op, "mul") == 0){
            result = a * b;
        } else if (strcmp(op, "div") == 0) {
            if (b == 0)
                error = 1;
            else
                result = a / b;
        }

        char symbol = '?';

        if (strcmp(op, "add") == 0)
            symbol = '+';
        else if (strcmp(op, "sub") == 0)
            symbol = '-';
        else if (strcmp(op, "mul") == 0)
            symbol = '*';
        else if (strcmp(op, "div") == 0)
            symbol = '/';

        char response[4096];

        if (error) {
            sprintf(response,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "<html>"
                    "<head><title>Calculator</title></head>"
                    "<body>"
                    "<h1>Calculator Result</h1>"
                    "<p><b>Loi:</b> Khong the chia cho 0</p>"
                    "</body>"
                    "</html>");
        } else {
            sprintf(response,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "<html>"
                    "<head><title>Calculator</title></head>"
                    "<body>"
                    "<h1>Calculator Result</h1>"
                    "<p>%.2lf %c %.2lf = %.2lf</p>"
                    "</body>"
                    "</html>",
                    a,
                    symbol,
                    b,
                    result);
        }

        send(client, response, strlen(response), 0);
        close(client);
    }

    close(server);

    return 0;
}