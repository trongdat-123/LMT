/* tcp_server1.c: Minh hoạ máy chủ TCP. */
#include <stdio.h>          /* printf() */
#include <sys/socket.h>     /* sockaddr */
#include <netinet/in.h>     /* sockaddr_in */
#include <string.h>         /* memset(), strlen() */
#include <unistd.h>         /* socket(), listen(), accept(), send(), recv(), close() */

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
int main()
{
    /* Socket nhận kết nối từ bên ngoài. */
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);        
    /* Một máy tính có thể có nhiều ̣địa chỉ IP, 
     * do đó cần chỉ định nhận kết nối tới địa chỉ nào. */

    SOCKADDR_IN saddr;  // Địa chỉ nhận kết nối từ bên ngoài
    saddr.sin_family = AF_INET;    // Loại địa chỉ: IPv4
    saddr.sin_port = htons(8888);  // Cổng: 8888
    saddr.sin_addr.s_addr = 0;     // 0.0.0.0: Bất kỳ địa chỉ nào của máy tính này
    /* Gán địa chỉ cho socket. */
    bind(s, (SOCKADDR*)&saddr, sizeof(SOCKADDR));
    /* Bắt đầu chấp nhận kết nối. */
    listen(s, 10);

    SOCKADDR_IN caddr;        // Địa chỉ phía kết nối tới
    int clen = sizeof(caddr); // Kích thước cấu trúc địa chỉ trên
    /* Đợi một bên kết nối tới */
    int c = accept(s, (SOCKADDR*)&caddr, (socklen_t*)&clen);
    /* Đã có một người dùng kết nối tới máy chủ.
     * Làm Người Từ Tế và chào hỏi họ đàng hoàng.
     */
    const char* welcome = "TCP Server: Hello\n";
    int sent = send(c, welcome, strlen(welcome), 0);
    printf("Sent: %d bytes\n", sent);

    /* Ra về với phần quà từ chương trình. */
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int received = recv(c, buffer, sizeof(buffer) - 1, 0);
    printf("Received: %d bytes\n", received);
    /* Đ̣óng kết nối. */
    close(c);
    /* Dừng phục vụ. */
    close(s);
}
