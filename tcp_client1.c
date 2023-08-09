/* tcp_client1.c: Minh hoạ kết nối TCP. */
#include <stdio.h>          /* printf() */
#include <sys/socket.h>     /* sockaddr */
#include <netinet/in.h>     /* sockaddr_in */
#include <string.h>         /* memset(), strlen() */
#include <unistd.h>         /* socket(), connect(), close() */
#include <arpa/inet.h>      /* inet_addr() */

const char* ip = "111.65.250.2"; // Địa chì kết nối đến. (Đây là vnexpress.net)
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int main() {
    /* Socket tiến hành kết nối. */
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // TCP/IPv4
    SOCKADDR_IN addr;  // Địa chỉ kết nối tớ́i
    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = htons(80); // Cổng ́80 (HTTP)
    addr.sin_addr.s_addr = inet_addr(ip);
    connect(s, (SOCKADDR*)&addr, sizeof(SOCKADDR));

    /* Gửi yêu cầu HTTP. */
    const char* welcome = "GET / HTTP/1.1\r\n"      // Phương thức GET, yêu cầu tài nguyên "/", phiên bản HTTP/1.1
                          "Host: vnexpress.net\r\n" // Header mô tả nguồn. HTTP/1.1 bắt buộc có header này
                          "\r\n"                    // Kết thúc phần header.
                          "";                       // Phương thức GET không có phần thân.
    int sent = send(s, welcome, strlen(welcome), 0);
    printf("Sent %d bytes\n", sent);

    /* Nhận phản hồi và in ra màn hình. */
    char buffer[1024];
    memset(buffer, 0 ,sizeof(buffer));
    int received = recv(s, buffer, sizeof(buffer) - 1, 0);
    printf("Received %d bytes: %s\n", received, buffer);
    /* Ngắt kết nối. */
    close(s);
}
