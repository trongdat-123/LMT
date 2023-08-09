/* tcp_server1.c: Minh hoạ máy chủ TCP đa tiến trình. */
#include <stdio.h>          /* printf(), fopen(), fseek(), ftell(), fread(), fclose(), feof() */
#include <sys/socket.h>     /* sockaddr */
#include <netinet/in.h>     /* sockaddr_in */
#include <string.h>         /* memset(), strlen() */
#include <unistd.h>         /* getpid(), kill(), socket(), bind(), listen(), send(), recv(), close(), fork() */
#include <stdlib.h>         /* exit(), calloc() */
#include <signal.h>         /* signal() */

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
void sighandler(int signum) {
    if (signum == SIGTERM) {
        /* Một tiến trình con vừa nhận dữ liệu.
         * Chuyển tiếp dữ liệu đến các kết nối hiện hoạt. */
        /* Đọc dữ liệu vừa được viết. */
        FILE* dataf = fopen("data.dat","rb");
        fseek(dataf, 0, SEEK_END);
        int len = ftell(dataf);
        fseek(dataf, 0, SEEK_SET);
        char* buffer = (char*)calloc(len + 1, 1);
        fread(buffer, 1, len, dataf);
        fclose(dataf);

        /* Đọc danh sách các kết nối hiện hoạt */
        FILE* f = fopen("clients.dat","rb");
        while (!feof(f)) {
            int tmp = 0;
            fread(&tmp, sizeof(int), 1, f);
            printf("Sending to %d\n", tmp);
            send(tmp, buffer, strlen(buffer), 0);
        }
        fclose(f);

        free(buffer);
        buffer = NULL;
    }
}
int main() {
    /* Số hiệu tiến trình hiện tai.
     * Tiến trình con cần số hiệu này để thông báo
     * đã nhận thông điệp tới tiến trình cha. */
    int parentid = getpid();
    /* Thủ tục xử lý khi có tiến trình con yêu cầu. */
    signal(SIGTERM, sighandler);
    /* Socket nhận kết nối */
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr;
    /* Địa chỉ nhận */
    saddr.sin_family = AF_INET;   // IPv4
    saddr.sin_port = htons(8888); // Cổng 8888 ́́́́
    saddr.sin_addr.s_addr = 0;    // 0.0.0.0: Địa chỉ bất kỳ của máy tính này
    bind(s, (SOCKADDR*)&saddr, sizeof(saddr));
    listen(s, 10);

    /* Tệp lưu danh sách kết nối hiện hoạt. */
    FILE* f = fopen("clients.dat","wb"); // Reset FILE Content
    while (0 == 0) {
        SOCKADDR_IN caddr;       // Địa chỉ của người gửi
        int len = sizeof(caddr); // Kích thước cấu trúc địa chỉ
        /* Chờ kết nối từ người dùng. */
        int c = accept(s, (SOCKADDR*)&caddr, (socklen_t*)&len);
        /* Lưu số hiệu kết nối vào danh sách kết nối hiện hoạt. */
        fwrite(&c, sizeof(int), 1, f);
        fflush(f);
        /* Sinh tiến trình con để xử lý kết nối này. */
        if (fork() == 0) {
            // Child process code 
            /* Tiến trình con không cần đọc danh sách kết nối. */
            fclose(f);
            /* Tiến trình con không cần nhận thêm kết nối. */
            close(s);
            /* Do hai tiến trình con và cha là độc lập nên tiến trình con
             * không thể đơn giản truyền một vùng nhớ cho tiến trình cha.
             * Ở đây sử dụng một tệp trung gian, lưu dữ liệu nhận được 
             * từ kết nối hiện tại, sau đó nhắc tiến trình cha đọc từ tệp này.
             */
            printf("Receiving data...\n");
            while (0 == 0) {
                char buffer[1024] = { 0 };
                int r = recv(c, buffer, sizeof(buffer) - 1, 0);  
                if (r > 0) {
                    printf("Received %d bytes from %d: %s\n", r, c, buffer);
                    /* Ghi dữ liệu nhận được vào tệp trung gian. */
                    f = fopen("data.dat", "wb"); // Đồng thời sẽ loại bỏ dữ liệu hiện có
                    fwrite(buffer, 1, strlen(buffer), f);
                    fclose(f);
                    /* Báo nhận tới tiến trình cha. */
                    kill(parentid, SIGTERM);
                } else {
                    break;
                }
            }
            exit(0);
        }
    }
    fclose(f);
}
