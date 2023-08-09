/* getaddrinfo.c: Minh hoạ phân giải tên miền bằng getaddrinfo() */
#include <stdio.h>                 /* printf() */
#include <sys/types.h>             /* getaddrinfo() */
#include <sys/socket.h>            /* getaddrinfo() */
#include <netdb.h>                 /* getaddrinfo() */
#include <netinet/in.h>            /* sockaddr_in */

const char *domain = "google.com";  // Tên miền muốn phân giải

int main()
{
    struct addrinfo *res = NULL;    // Kết quả phân giải tên miền.
    /* Thực hiện phân giải tên miền. */
    int error = getaddrinfo(domain, NULL, NULL, &res);
    /* Kết quả trả về là 0 nếu phân giải thành công hoặc mã lỗi khác 0 nếu thất bại.
     * Nếu thành công, `res` trỏ đến đầu một danh sách liên kết 
     * của cấu trúc `addrinfo` là kết quả phân giải tên miền.
     * Định nghĩa cấu trúc `addrinfo`:
        struct addrinfo {
            int              ai_flags;       // Tuỳ chọn phân giải
            int              ai_family;      // Loại địa chỉ
            int              ai_socktype;    // Loại socket
            int              ai_protocol;    // Loại giao thức giao vận
            socklen_t        ai_addrlen;     // Kích thước cấu trúc địa chỉ
            struct sockaddr *ai_addr;        // Cấu trúc địa chỉ, phù hợp dùng ngay vào các hàm như connect()
            char            *ai_canonname;   // Tên miền chính thức
            struct addrinfo *ai_next;        // Con trỏ đến cấu trúc kế tiếp
        };
     */
    if (error == 0) {
        /* In ra các địa chỉ tương ứng. */

        /* Ta cần giữ lại `res` để giải phóng bộ nhớ cấp phát bởi getaddrinfo() sau khi xong.
         * Do đó, dùng một con trỏ khác để duyệt qua danh sách.
         */
        struct addrinfo *result = res;

        /* Danh sách các kết quả được kết thúc bởi con trỏ NULL. */
        while (result != NULL) {
            printf("...");    
            /* Các cấu trúc địa chỉ có thể khác nhau, cần dựa vào loại ̣địa chỉ để ép kiểu đúng. */
            if (result->ai_family == AF_INET) {
                printf("IPv4\n");
                struct sockaddr_in* sin = (struct sockaddr_in*)result->ai_addr; 
                unsigned char* ip = (unsigned char*)&(sin->sin_addr.s_addr);
                printf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
            } else if (result->ai_family == AF_INET6) {
                printf("IPv6\n");
            }
            result = result->ai_next;
        }           
        /* Giải phóng bộ nhớ cấp phát bởi getaddrinfo() */
        freeaddrinfo(res);
    } else {
        printf("Error\n");
    }
}
