/* forksharingclient.c: Minh họa xử lý đa tiến trình (máy trạm) */
#include <stdio.h>           /* fgets(), printf() */
#include <unistd.h>          /* socket(), connect(), fork(), bind(), sendto(), recvfrom(), sleep() */
#include <stdlib.h>          /* malloc() */
#include <sys/socket.h>      /* sockaddr */
#include <sys/types.h>       /* inet_addr() */
#include <netdb.h>           /* inet_addr() */
#include <arpa/inet.h>       /* inet_addr() */
#include <string.h>          /* strlen(), memset() */
#include <signal.h>          /* signal() */
#include <sys/wait.h>        /* wait() */

int child = 0;  // Số hiệu tiến trình con
void kill_child(int signum) {
    if (child != 0)
        kill(child, SIGTERM);
    exit(signum);
}

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
int main() {
    char name[1024] = { 0 };
    printf("Client name: ");
    fgets(name, sizeof(name) - 1, stdin);

    child = fork();
    if (child == 0) {
        /* Tiến trình con: Dò tìm máy chủ trong mạng. */

        /* Socket gửi yêu cầu thăm dò. */
        int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        /* Chuẩn bị địa chỉ gửi. */
        SOCKADDR_IN baddr;            // Địa chỉ dò tìm
        baddr.sin_family = AF_INET;   // IPv4
        baddr.sin_port = htons(6000); // Cổng 6000
        baddr.sin_addr.s_addr = inet_addr("255.255.255.255"); // Địa chỉ quảng bá trong mạng
        /* Cho phép gửi quảng bá. */
        int on = 1;
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

        /* Mỗi 5s, gửi một yêu cầu thăm dò trong mạng,
         * kèm theo tên của chúng ta.
         */
        while (0 == 0) {
            sendto(s, name, strlen(name), 0, (SOCKADDR*)&baddr, sizeof(baddr));
            sleep(5);
        }
    } else {
        /* Tiến trình cha: Giao tiếp với máy chủ. */

        /* Trước khi thoát, ta cần kết thúc tiến trình còn lại.
         * Đăng ký xử lý Ctrl-C trong trường hợp người dùng
         * yêu cầu thoát trước.
         */
        signal(SIGINT, kill_child);

        /* Socket nhận thông điệp đồng thuận. */
        int udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        /* Chuẩn bị socket để nhận. */
        SOCKADDR_IN myaddr;   // Địa chỉ của máy này
        myaddr.sin_family = AF_INET;    // IPv4
        myaddr.sin_port = htons(6000);  // Cổng 6000
        myaddr.sin_addr.s_addr = 0;     // 0.0.0.0: Địa chỉ bất kỳ
        bind(udp, (SOCKADDR*)&myaddr, sizeof(myaddr));

        /* Kiểm tra xem máy chủ (nếu có) đã biết đến
         * sự tồn tại của chúng ta hay chưa.
         */
        SOCKADDR_IN server;        // Địa chỉ máy chủ
        int slen = sizeof(server); // Kích thước địa chỉ trên
        int ack = 0; // Đã nhận được đồng thuận chưa?
        char msg[1024] = { 0 }; // Thông điệp nhận được
        /* Mỗi 1s, thử nhận một thông điệp bất kỳ. */
        while (ack == 0) {
            recvfrom(udp, msg, sizeof(msg) - 1, 0, (SOCKADDR*)&server, (socklen_t*)&slen);
             /* Nếu nội dung là "ACK", cho rằng đây là máy chủ 
              * đang trả lời yêu cầu thăm dò của chúng ta.
              * Nếu nội dung không phải là "ACK",
              * cho rằng đây không phải là máy chủ, và
              * xoá trắng vùng đệm để dùng lại vào lần sau.
             */
            if (!strcmp(msg, "ACK")) {
                ack = 1;
            } else {
                memset(msg, 0, strlen(msg));
                sleep(1);
            }
        }
        close(udp);

        /* Đã nhận diện một máy chủ trong mạng.
         * Địa chỉ của máy chủ đã nằm trong `server`. 
         * Sử dụng lại địa chỉ này để kết nối.
         */

        /* Socket trao đổi dữ liệu. */
        int c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        server.sin_port = htons(5000); // Cổng 5000
        connect(c, (SOCKADDR*)&server, sizeof(server));

        /* Nhận dữ liệu và in ra màn hình. */
        char data[1024] = { 0 };
        while (0 == 0) {
            int tmp = recv(c, data, sizeof(data) - 1, 0);
            if (tmp <= 0) {
                /* Máy chủ đã dừng trao đổi hoăc kết nối bị ngắt. */
                break;
            } else {
                printf("%s", data);
                memset(data, 0, sizeof(data));    
            }
        }
        close(c);
        printf("Press Enter to quit...");
        getchar();
        kill_child(0);
        return 0;
    }
}
