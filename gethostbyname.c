/* gethostbyname.c: Minh hoạ phân giải tên miền bằng gethostbyname() */
#include <stdio.h>            /* printf() */
#include <netdb.h>            /* gethostbyname() */

const char *domain = "google.com";  // Tên miền muốn phân giải
int main() {
    /* Thực hiện phân giải tên miền. */
    struct hostent *phost = gethostbyname(domain);
    /* Kết quả trả về là một cấu trúc `hostent` là kết quả phân giải tên miền,
     * hoặc NULL nếu phân giải thất bại.
     * Định nghĩa cấu trúc `hostent`:
        struct hostent {
           char  *h_name;            // Tên miền chính thức
           char **h_aliases;         // Danh sách các bí danh (Tên miền phụ)
           int    h_addrtype;        // Loại địa chỉ, AF_INET (IPv4) hoặc AF_INET6 (IPv6)
           int    h_length;          // Độ dài cho một địa chỉ
           char **h_addr_list;       // Danh sách các địa chỉ
        }
     * Các địa chỉ trong `h_addr_list` đều thuộc cùng loại địa chỉ.
    */
    if (phost != NULL) {
        /* In ra các địa chỉ tương ứng. */
        int i = 0;
        /* Danh sách các kết quả được kết thúc bởi con trỏ NULL. */
        while (phost->h_addr_list[i] != NULL) {
            /* Giả sử các địa chỉ đều là IPv4. */
            printf("%d.%d.%d.%d\n", (unsigned char)phost->h_addr_list[i][0], 
                                    (unsigned char)phost->h_addr_list[i][1],
                                    (unsigned char)phost->h_addr_list[i][2],
                                    (unsigned char)phost->h_addr_list[i][3]);
            i += 1;
        }
    }
    printf("...");
}
