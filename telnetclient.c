/* telnetclient.c: Minh hoạ gửi lệnh đến máy chủ qua TCP. */
#include <stdio.h>          /* printf() */
#include <stdlib.h>         /* atoi(), realloc() */
#include <sys/socket.h>     /* sockaddr, inet_addr() */
#include <netinet/in.h>     /* sockaddr_in, inet_addr() */
#include <arpa/inet.h>      /* inet_addr() */ 
#include <string.h>         /* memset(), strlen(), strcpy() */
#include <unistd.h>         /* socket(), connect(), send(), recv(), close() */

//#define DEBUGGING
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
//#define DEBUGGING
int main(int argc, char** argv) {
#ifdef DEBUGGING
    argc = 3;
#endif
    if (argc < 3) {
        printf("Parameter missing\n");
        return 1;
    }
#ifdef DEBUGGING
    in_addr_t ip = inet_addr("127.0.0.1"); // Địa chỉ kết nối đến
    short port = 8888; // Cổng kết nối đến
#else
    in_addr_t ip = inet_addr(argv[1]); // Địa chỉ kết nối đến
    short port = (short)atoi(argv[2]); // Cổng kết nối đến
#endif
    /* Socket tiến hành kết nối. */
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr; // Địa chỉ kết nối tớ́i
    saddr.sin_family = AF_INET;   // IPv4
    saddr.sin_port = htons(port); // Cổng
    saddr.sin_addr.s_addr = ip;   // Địa chỉ IP
    int error = connect(s, (SOCKADDR*)&saddr, sizeof(SOCKADDR));
    if (error < 0) {
        /* Kết nối không thành công. */
        printf("Connection refused\n");
        return 1;
    }
    // Receive welcome message from server
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int received = 0, tmp = 0, bytes_to_receive = 0;
    do {
        bytes_to_receive = sizeof(buffer) - received - 1;
        tmp = recv(s, buffer + received, bytes_to_receive, 0);
        received += tmp;
    } while ((tmp >= 0) && (tmp == bytes_to_receive));
    printf("%s\n", buffer);

    /* Nhận lệnh từ người dùng và gửi tới server để xử lý */
    while (0 == 0) {

        // Receive command from stdin
        memset(buffer, 0, sizeof(buffer));
#ifdef DEBUGGING
        strcpy(buffer,"cat telnetclient.cpp\n");
#else
        fgets(buffer, sizeof(buffer) - 1, stdin);
#endif
        send(s, buffer, strlen(buffer), 0);

        // Receive reponse from server
        received = 0;
        char* response = NULL;
        do {
            /* Kết quả đầu ra của lệnh có thể dài hơn một gói tin. 
             * Thực hiện nhận liên tiếp cho đến hết.
             */
            memset(buffer, 0, sizeof(buffer));
            tmp = recv(s, buffer, sizeof(buffer) - 1, 0);
            if (tmp >= 0) {
                response = (char*)realloc(response, received + tmp + 1);
                sprintf(response + strlen(response), "%s", buffer);
                response[received + tmp] = 0; // Byte ket thuc xau
                received += tmp;
            }
        } while ((tmp >= 0) && (tmp == sizeof(buffer) - 1));
        printf("%s\n", response);
        /* Giải phóng bộ nhớ cấp phát. */
        free(response);
        response = NULL;
    }
}
