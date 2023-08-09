/* udpex1.c: Minh họa gửi và nhận trên giao thức UDP */
#include <stdio.h>           /* printf(), fgets() */
#include <sys/socket.h>      /* inet_addr() */
#include <netdb.h>           /* inet_addr() */
#include <arpa/inet.h>       /* inet_addr() */
#include <string.h>          /* memset() */
#include <unistd.h>          /* socket(), bind(), sendto(), recvfrom(), close() */

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
char msg[1024]; // Chứa thông điệp gửi & nhận
int main() {
    /* Socket để gửi & nhận thông điệp. */
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    /* Gán địa chỉ nhận thông điệp. */
    SOCKADDR_IN myaddr;
    myaddr.sin_family = AF_INET;     // IPv4
    myaddr.sin_port = htons(6000);   // Cổng 6000
    myaddr.sin_addr.s_addr = 0;      // 0.0.0.0: Địa chỉ bất kỳ của máy tính này
    bind(s, (SOCKADDR*)&myaddr, sizeof(SOCKADDR_IN));

    /* Địa chỉ gửi thông điệp tới. */
    SOCKADDR_IN toaddr; // Địa chỉ gửi thông điệp tới
    toaddr.sin_family = AF_INET;
    toaddr.sin_port = htons(5000);
    toaddr.sin_addr.s_addr = inet_addr("172.22.80.1");
    
    while (0 == 0) {
        /* Gửi thông điệp đi. */
        printf("Input a message to send: ");
        memset(msg, 0, sizeof(msg));
        fgets(msg, sizeof(msg) - 1, stdin);
        sendto(s, msg, strlen(msg), 0, (SOCKADDR*)&toaddr, sizeof(SOCKADDR_IN));

        /* Nhận thông điệp trả lời. */
        SOCKADDR_IN sender;             // Địa chỉ của người gửi
        int slen = sizeof(SOCKADDR_IN); // Kích thước cấu trúc địa chỉ
        memset(msg, 0, sizeof(msg));
        int received = recvfrom(s, msg, sizeof(msg) - 1, 0, (SOCKADDR*)&sender, (socklen_t*)&slen);
        printf("Received %d bytes from %s: %s\n", received, inet_ntoa(sender.sin_addr), msg);
    }
    close(s);
}
