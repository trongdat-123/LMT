/* udpex3.c: Minh họa nhận và gừi trên giao thức UDP. */
#include <stdio.h>           /* printf() */
#include <sys/socket.h>      /* inet_ntoa() */
#include <netdb.h>           /* inet_ntoa() */
#include <arpa/inet.h>       /* inet_ntoa() */
#include <string.h>          /* memset(), memcpy() */
#include <unistd.h>          /* socket(), bind(), recvfrom(), sendto(), close() */
#include <malloc.h>          /* realloc(), free() */

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
char msg[1024];  // Thông điệp gửi nhận
SOCKADDR_IN* senders = NULL;  // Danh sách người gửi
int count = 0;                // Số lượng người gửi
int main() {
    /* Socket để gửi & nhận thông điệp. */
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    /* Gán địa chỉ nhận thông điệp. */
    SOCKADDR_IN myaddr; // Địa chỉ của máy này
    myaddr.sin_family = AF_INET;    // IPv4
    myaddr.sin_port = htons(5000);  // Cổng 5000
    myaddr.sin_addr.s_addr = 0;     // 0.0.0.0: Địa chỉ bất kỳ của máy tính này
    bind(s, (SOCKADDR*)&myaddr, sizeof(SOCKADDR_IN));

    while (0 == 0) {
        /* Nhận một thông điệp bất kỳ. */
        SOCKADDR_IN sender;             // Địa chỉ của người gửi
        int slen = sizeof(SOCKADDR_IN); // Kích thước cấu trúc địa chỉ
        memset(msg, 0, sizeof(msg));
        int received = recvfrom(s, msg, sizeof(msg) - 1, 0, (SOCKADDR*)&sender, (socklen_t*)&slen);
        printf("Received %d bytes from %s: %s\n", received, inet_ntoa(sender.sin_addr), msg);

        /* Lưu lại địa chỉ của người gửi vừa rồi. */
        senders = (SOCKADDR_IN*)realloc(senders, (count + 1) * sizeof(SOCKADDR_IN));
        memcpy(senders + count, &sender, sizeof(SOCKADDR_IN));
        count += 1;

        /* Chuyển tiếp thông điệp cho tất cả những người gửi đã biết. */
        for (int i = 0; i < count; i++)
            sendto(s, msg, strlen(msg), 0, (SOCKADDR*)&(senders[i]), sizeof(SOCKADDR_IN));
    }
    close(s);
    /* Giải phóng danh sách người gửi. */
    free(senders);
    senders = NULL;
}
