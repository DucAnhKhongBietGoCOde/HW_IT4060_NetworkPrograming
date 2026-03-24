/* 
Student Information Client
This program is a TCP client that connects to a server to send student information. 
The client prompts the user to enter details such as Student ID (MSSV), Full Name, Date of Birth, and GPA. 
The entered information is then packed into a single string and sent to the server. 
The client continues to allow the user to enter multiple students until they choose to exit.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_BUF 512

int main(int argc, char *argv[]) {
    // 1. Check command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    // 2. Create TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 3. Connect to server
    printf("Connecting to server %s:%d...\n", ip, port);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Successfully connected to the server!\n");

    char mssv[20], hoten[100], ngaysinh[20], packet[MAX_BUF];
    float diemtb;
    char choice;

    // 4. Main loop for input and data transmission
    do {
        printf("\n---------- STUDENT INFORMATION ENTRY ----------\n");
        
        printf("Student ID (MSSV): ");
        fgets(mssv, sizeof(mssv), stdin);
        mssv[strcspn(mssv, "\n")] = '\0'; 

        printf("Full Name       : ");
        fgets(hoten, sizeof(hoten), stdin);
        hoten[strcspn(hoten, "\n")] = '\0';

        printf("Date of Birth (dd/mm/yyyy): ");
        fgets(ngaysinh, sizeof(ngaysinh), stdin);
        ngaysinh[strcspn(ngaysinh, "\n")] = '\0';

        printf("GPA             : ");
        scanf("%f", &diemtb);
        while (getchar() != '\n'); // Clear buffer after scanf

        // Pack data into a single string
        snprintf(packet, sizeof(packet), "%s | %s | %s | %.2f", mssv, hoten, ngaysinh, diemtb);

        // Send data to server
        if (send(sock, packet, strlen(packet), 0) < 0) {
            perror("Failed to send data");
            break;
        }
        printf("=> Data sent: [%s]\n", packet);

        printf("\nDo you want to enter another student? (y/n): ");
        scanf(" %c", &choice);
        while (getchar() != '\n'); 

    } while (choice == 'y' || choice == 'Y');

    // 5. Close socket and exit
    close(sock);
    printf("Connection closed. Program terminated.\n");

    return 0;
}