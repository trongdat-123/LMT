/* mytelnet.c: Minh hoạ máy chủ nhận lệnh từ xa qua TCP */
#include <stdio.h>          /* printf(), fopen(), fseek(), ftell(), fread(), fclose(), feof() */
#include <sys/socket.h>     /* sockaddr */
#include <netinet/in.h>     /* sockaddr_in */
#include <string.h>         /* memset(), strlen() */
#include <unistd.h>         /* socket(), bind(), listen(), accept(), send(), recv(), close() */
#include <stdlib.h>         /* system(), calloc() */

//#define DEBUGGING
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
int main(int argc, char** argv) {
#ifdef DEBUGGING
    argc = 2; 
#endif
    if (argc > 1) {
#ifdef DEBUGGING
        short port = 8888; // Cổng nhận kết nối
#else
        short port = (short)atoi(argv[1]); // Cổng nhận kết nối
#endif
        /* Socket nhận kết nối từ bên ngoài. */
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);        
        /* Một máy tính có thể có nhiều ̣địa chỉ IP, 
         * do đó cần chỉ định nhận kết nối tới địa chỉ nào. */
        SOCKADDR_IN saddr;  // Địa chỉ nhận kết nối từ bên ngoài
        saddr.sin_family = AF_INET;    // Loại địa chỉ: IPv4
        saddr.sin_port = htons(port);  // Cổng
        saddr.sin_addr.s_addr = 0;     // 0.0.0.0: Bất kỳ địa chỉ nào của máy tính này

        if (bind(s, (SOCKADDR*)&saddr, sizeof(SOCKADDR)) == 0) {
            /* Gán địa chỉ thành công. Bắt đầu chấp nhận kết nối. */
            listen(s, 10);

            SOCKADDR_IN caddr;   // Địa chỉ phía kết nối tới
            int clen = sizeof(SOCKADDR_IN); // Kích thước cấu trúc địa chỉ trên
            /* Đợi một bên kết nối tới, giống như bạn đợi người yêu vậy. */
            int c = accept(s, (SOCKADDR*)&caddr, (socklen_t*)&clen);
            /* Đã có một người dùng kết nối tới máy chủ.
             * Gửi thông báo tới crush, à nhầm,
             * tới bên vừa kết nối, để họ biết mà nhập lệnh. */
            const char* msg = "Send any command to execute\n";
            send(c, msg, strlen(msg), 0);

            while (0 == 0) {
                /* Nhận lệnh */
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                int received = recv(c, buffer, sizeof(buffer) - 1, 0);
                if (received <= 0)
                    /* Đầu bên kia đã hết lệnh, ngắt kết nối, hoặc kết nối gặp sự cố. */
                    break;
                /* Nhận được một câu lệnh từ xa gồm `received` ký tự.
                 * Tiến hành thực thi và gửi trả lại kết quả. */

                /* Nếu bên gửi dùng nc để kết nối, nội dung nhập sẽ được gửi
                 * khi người dùng nhấn Enter.
                 * Phía máy chủ sẽ nhận được cả ký tự xuống dòng cần được loại bỏ.
                 * Thực hiện cắt ngắn nôi dung bằng ký tự kết thúc (0)
                 */
                if (buffer[received - 1] == '\n' || buffer[received - 1] == '\r')
                    buffer[received - 1] = 0;

                /* Xây dựng câu lệnh để thực thi trên máy chủ. 
                 * Cụ thể hơn là,
                 (người dùng nhập) > command.txt 2>&1
                 *                   ^~~~~~~~~~~~~~~~~~
                 *                   Đưa đầu ra của lệnh (người dùng nhập)
                 *                   vào tệp `command.txt`
                 * Do system() không cung cấp cơ chế đọc đầu ra của lệnh,
                 * cách duy nhất để lấy kết quả là lưu vào tệp tạm và đọc lại.
                 */
                sprintf(buffer + strlen(buffer), " > command.txt 2>&1");
                /* Thực thi lệnh */
                system(buffer);
                /* Đọc đầu ra của lệnh */
                FILE* f = fopen("command.txt","rb");
                fseek(f, 0, SEEK_END);
                int size = ftell(f);

                char* data = (char*)calloc(size, 1);
                fseek(f, 0, SEEK_SET);
                fread(data, 1, size, f);
                fclose(f);

                /* Gừi lại kết quả thực thi */
                int sent = 0, tmp = 0;
                do {
                    tmp = send(c, data, size, 0);
                    sent += tmp;
                } while (sent < size && tmp >= 0);

                free(data);
                data = NULL;
                if (tmp < 0) {
                    /* Kết nối tới phía bên kia đã gặp sự cố hoặc 
                     * phía nhận đã ngắt kết nối.
                     * Có thể dừng phục vụ ngay tại đây hoặc bỏ qua,
                     * vì recv() cũng sẽ báo lỗi và kết thúc.
                     */
                    
                    // break;
                }
            }
            /* Đ̣óng kết nối. */
            close(c);
        } else {
            printf("Failed to bind\n");
        }
        /* Dừng phục vụ. */
        close(s);
    } else {
        printf("Parameter missing\n");
    }
}
