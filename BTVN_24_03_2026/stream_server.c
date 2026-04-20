#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    int ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    ret = listen(listener, 5);
    if (ret < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for client...\n");
    
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }
    
    char buf[256];
    char *package = NULL;
    int package_size = 0;
    int total = 0;

    while (1) {
        int len = recv(client, buf, sizeof(buf), 0);
        
        if (len <= 0) {
            break;
        }

        char *temp = realloc(package, package_size + len + 1);
        if (temp == NULL) {
            perror("realloc failed");
            break;
        }
        package = temp;
        
        memcpy(package + package_size, buf, len);
        package_size += len;
        package[package_size] = '\0';

        char *p;
        while ((p = strstr(package, "0123456789")) != NULL) {
            total ++;
            memmove(package, p + 10, package_size - (p - package) - 10);
            package_size -= (p - package) + 10;
            package[package_size] = '\0';
            
        }
        
        printf("Total: %d\n", total);
    }

    free(package);
    close(client);
    close(listener);
}