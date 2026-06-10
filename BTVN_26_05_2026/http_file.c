#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#define PORT 9090

int main() {
    signal(SIGPIPE, SIG_IGN); 

    int server = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    
    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        return 1;
    }
    listen(server, 5);
    
    printf("HTTP File Server running on http://127.0.0.1:%d ...\n", PORT);
    
    char buf[4096]; // Bộ đệm nhận HTTP Request
    
    while (1) {
        int client = accept(server, NULL, NULL);
        memset(buf, 0, sizeof(buf));
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) { close(client); continue; }
        
        char method[16], path[512], version[16];
        sscanf(buf, "%s %s %s", method, path, version);
        
        char root_dir[512];
        getcwd(root_dir, sizeof(root_dir));
        
        char decoded_path[512] = "";
        int j = 0;
        for (int i = 1; path[i] != '\0' && path[i] != '?'; i++) {
            if (path[i] == '%' && path[i+1] != '\0' && path[i+2] != '\0') {
                char tmp[3] = {path[i+1], path[i+2], '\0'};
                decoded_path[j++] = (char)strtol(tmp, NULL, 16);
                i += 2;
            } else {
                decoded_path[j++] = path[i];
            }
        }
        decoded_path[j] = '\0';

        char full_sys_path[2048];
        snprintf(full_sys_path, sizeof(full_sys_path), "%s/%s", root_dir, decoded_path);
        
        // LOG CHÍNH 1: Nhận yêu cầu từ Trình duyệt
        printf("[%s] %s\n", method, path);
        
        struct stat st;
        if (stat(full_sys_path, &st) != 0) {

            printf("  --> [STATUS] 404 Not Found\n\n");
            
            char *err404 = "HTTP/1.1 404 Not Found\r\n"
                           "Content-Length: 22\r\n"
                           "Connection: close\r\n\r\n"
                           "<h1>404 Not Found</h1>";
            send(client, err404, strlen(err404), 0);
            close(client);
            continue;
        }
        
        // TRƯỜNG HỢP 1: ĐƯỜNG DẪN LÀ THƯ MỤC
        if (S_ISDIR(st.st_mode)) {
            char html_body[16384] = {0};
            snprintf(html_body, sizeof(html_body), 
                "<html><head><title>File Explorer</title><meta charset='UTF-8'>"
                "<style>body{font-family:Arial; margin:30px;} li{margin:8px 0; font-size:16px;}</style>"
                "</head><body>"
                "<h2>Thư mục: /%s</h2><hr><ul>", decoded_path);
                               
            DIR *dir = opendir(full_sys_path);
            struct dirent *entry;
            
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0) continue;
                
                char url_path[1024];
                if (strlen(decoded_path) == 0) snprintf(url_path, sizeof(url_path), "/%s", entry->d_name);
                else snprintf(url_path, sizeof(url_path), "/%s/%s", decoded_path, entry->d_name);
                
                char child_sys_path[4096];
                snprintf(child_sys_path, sizeof(child_sys_path), "%s/%s", full_sys_path, entry->d_name);
                
                struct stat sub_st;
                stat(child_sys_path, &sub_st);
                
                if (S_ISDIR(sub_st.st_mode)) {
                    snprintf(html_body + strlen(html_body), sizeof(html_body) - strlen(html_body), 
                    "<li><a href='%s'><b>%s/</b></a></li>", url_path, entry->d_name);
                } else {
                    snprintf(html_body + strlen(html_body), sizeof(html_body) - strlen(html_body), 
                    "<li><a href='%s'><i>%s</i></a></li>", url_path, entry->d_name);
                }
            }
            closedir(dir);
            strcat(html_body, "</ul><hr></body></html>");
            
            printf("  --> [STATUS] 200 OK (Duyệt thư mục)\n\n");

            char response[18000];
            snprintf(response, sizeof(response), 
            "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=UTF-8\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n\r\n%s", 
                    (int)strlen(html_body), html_body);
            send(client, response, strlen(response), 0);
        }
        
        // TRƯỜNG HỢP 2: ĐƯỜNG DẪN LÀ FILE
        else if (S_ISREG(st.st_mode)) {
            char *ext = strrchr(full_sys_path, '.');
            char content_type[128] = "application/octet-stream";
            
            if (ext != NULL) {
                if (strcasecmp(ext, ".txt") == 0 || strcasecmp(ext, ".c") == 0 || strcasecmp(ext, ".h") == 0) 
                    strcpy(content_type, "text/plain; charset=UTF-8");
                else if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0) 
                    strcpy(content_type, "text/html; charset=UTF-8");
                else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) 
                    strcpy(content_type, "image/jpeg");
                else if (strcasecmp(ext, ".png") == 0) 
                    strcpy(content_type, "image/png");
                else if (strcasecmp(ext, ".gif") == 0) 
                    strcpy(content_type, "image/gif");
                else if (strcasecmp(ext, ".mp3") == 0) 
                    strcpy(content_type, "audio/mpeg");
                else if (strcasecmp(ext, ".mp4") == 0) 
                    strcpy(content_type, "video/mp4");
            }
            
            FILE *f = fopen(full_sys_path, "rb");
            if (f != NULL) {
                long file_size = st.st_size;
                long start_byte = 0;
                long end_byte = file_size - 1;
                int is_partial = 0;

                char *range_header = strstr(buf, "Range: bytes=");
                if (range_header != NULL) {
                    is_partial = 1;
                    sscanf(range_header, "Range: bytes=%ld-%ld", &start_byte, &end_byte);
                    if (end_byte <= 0 || end_byte >= file_size) {
                        end_byte = file_size - 1;
                    }
                }

                long content_length = end_byte - start_byte + 1;
                char header[1024];

                if (is_partial) {
                    printf("  --> [STATUS] 206 Partial Content | Loại: %s\n\n", content_type);

                    snprintf(header, sizeof(header),
                        "HTTP/1.1 206 Partial Content\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %ld\r\n"
                        "Content-Range: bytes %ld-%ld/%ld\r\n"
                        "Connection: close\r\n\r\n", content_type, content_length, start_byte, end_byte, file_size);
                    
                    fseek(f, start_byte, SEEK_SET);
                } else {
                    printf("  --> [STATUS] 200 OK | Kích thước: %ld bytes | Loại: %s\n\n", file_size, content_type);

                    snprintf(header, sizeof(header),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %ld\r\n"
                        "Accept-Ranges: bytes\r\n"
                        "Connection: close\r\n\r\n", content_type, file_size);
                }
                
                send(client, header, strlen(header), 0);
                
                char video_buf[65536];
                long total_sent = 0;
                int n;
                
                while (total_sent < content_length && (n = fread(video_buf, 1, sizeof(video_buf), f)) > 0) {
                    if (total_sent + n > content_length) {
                        n = content_length - total_sent;
                    }
                    if (send(client, video_buf, n, 0) < 0) {
                        break; 
                    }
                    total_sent += n;
                }
                fclose(f);
            }
        }
        close(client);
    }
    close(server);
    return 0;
}