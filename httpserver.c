/* httpserver-v2.c: 
 * Máy chủ cung cấp giao diện quản lý hệ thống tệp tin qua HTTP, phiên bản 2. 
 */

/* Các cải tiến mới trong phiên bản này:
 * - Cải tiến hỗ trợ đa người truy cập với kiến trúc mới
 * - Sửa các lỗi vặt
 * - Hổ trợ tải lên tập tin
 */

/* Lưu ý dành cho bạn đọc:
 * Các chú thích trong tài liệu này được viết dưới giả thiết bạn đọc đã quen thuộc với
 * máy chủ phiên bản trước, do đó sẽ không giải thích lại các phần giống nhau.
 * Xem lại mã nguồn httpserver-v1.c nếu cần tìm hiểu thêm.
 */

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
char currentPath[1024] = {0}; // Đường dẫn mặc định
/* Xem lại httpserver-v1.c */
int CompareDR(const struct dirent **A, const struct dirent **B) {
    if ((*A)->d_type == DT_DIR) {
        if ((*B)->d_type == DT_DIR)
            return strcasecmp((*B)->d_name, (*A)->d_name);        
        else
            return 1;
    } else {
        if ((*B)->d_type != DT_DIR)
            return strcasecmp((*B)->d_name, (*A)->d_name);        
        else
            return 0;
    }
}
void Append(char** phtml, const char* name) {
    int oldLen = *phtml == NULL ? 0 : strlen(*phtml);
    (*phtml) = (char*)realloc(*phtml, oldLen + strlen(name) + 1);
    memset(*phtml + oldLen, 0, strlen(name) + 1);
    sprintf(*phtml + oldLen, "%s", name);
}
/* Cải tiến ở phiên bản 2:
 * 
 *  - Cải tiến: Hệ thống duyệt thư mục
 *     - Trong phiên bản 1, mục lục được cấu trúc như sau:
 *
 *       .       --> liên kết đến ".*"
 *       ..      --> liên kết đến "..*"
 *       dir1    --> liên kết đến "dir1*"
 *       dir2    --> liên kết đến "dir2*"
 *       < ... các thư mục tiếp ... >
 *       file1    --> liên kết đến "file1"
 *       file2    --> liên kết đến "file2"
 *       < ... các tệp tin tiếp ... >
 *     
 *      Cấu trúc này yêu cầu phải duy trì trạng thái thư mục hiện tại cho từng kết nối,
 *      làm quá trình xử lý trở nên phức tạp hơn.
 *     
 *     
 *      Trong phiên bản 2, thiết kế mục lục được thay đổi như sau:
 *     
 *       .       --> liên kết đến "<đường-dẫn-đầy-đủ>/.*"
 *       ..      --> liên kết đến "<đường-dẫn-đầy-đủ>/..*"
 *       dir1    --> liên kết đến "<đường-dẫn-đầy-đủ>/dir1*"
 *       dir2    --> liên kết đến "<đường-dẫn-đầy-đủ>/dir2*"
 *       < ... các thư mục tiếp ... >
 *       file1    --> liên kết đến "<đường-dẫn-đầy-đủ>/file1"
 *       file2    --> liên kết đến "<đường-dẫn-đầy-đủ>/file2"
 *       < ... các tệp tin tiếp ... >
 *       <biểu mẫu tải lên tệp tin>
 *   
 *      Việc sử dụng đường dẫn đầy đủ đã loại bỏ ràng buộc duy trì trạng thái,
 *      đồng nghĩa với việc tính năng duyệt thư mục đã trở nên hoàn toàn 
 *      "vô trạng thái" ("stateless") - tức các truy cập hoàn toàn độc lập 
 *      với nhau và không tác động đến nhau.
 *   
 *      Điều này cũng có nghĩa rằng có thể truy cập trực tiếp bất kỳ tài nguyên nào
 *      mà không cần phải đi qua từng mức của cây thư mục.
 *   
 *      Tuy nhiên, vẫn còn một hạn chế là hệ thống vẫn không thể tự phân biệt 
 *      thư mục và tệp tin, mà vẫn nhờ vào các chỉ thị đặc biệt.
 *   
 * - Cải tiến: Hỗ trợ định dạng tệp tin
 *    - Phiên bản 1 bỏ qua định dạng tệp tin và coi tất cả là văn bản.
 *    - Phiên bản 2 có thể hiểu một số loại tệp tin phổ biến,
 *       cho phép các trình duyệt nhận dạng đúng nội dung.
 *
 * - Cải tiến: Tải lên tệp tin
 *     Phiên bản 2 hỗ trợ tải lên tệp tin trên giao diện duyệt thư mục.
 *
 */
char* BuildHTML(char* path) {
    char* html = NULL;
    struct dirent **namelist;
    int n;

    /* Giao diện duyệt thư mục phiên bản 2: Ah shit, here we go again. */
    n = scandir(path, &namelist, NULL, CompareDR);
    Append(&html,"<html>\n");
    while (n--) {
        if (namelist[n]->d_type == DT_DIR) {
            Append(&html,"<a href = \"");
            Append(&html, path);
            if (currentPath[strlen(path) - 1] != '/')
                Append(&html, "/");
            Append(&html, namelist[n]->d_name);
            Append(&html, "*");
            Append(&html,"\"><b>");
            Append(&html, namelist[n]->d_name);
            Append(&html,"</b></a><br>\n");
        } else {
            Append(&html,"<a href = \"");
            Append(&html, path);
            if (currentPath[strlen(path) - 1] != '/')
                Append(&html, "/");
            Append(&html, namelist[n]->d_name);
            Append(&html,"\"><i>");
            Append(&html, namelist[n]->d_name);
            Append(&html,"</i></a><br>\n");
        }
        free(namelist[n]);
    }
    /* Biểu mẫu cho phếp tải lên tệp tin */
    Append(&html, "<form action=\"");
    Append(&html, path);
    if (currentPath[strlen(path) - 1] != '/')
        Append(&html, "/");
    Append(&html, "\" method=\"post\" enctype=\"multipart/form-data\">");
    Append(&html, "<label>File: </label>");
    Append(&html, "<input name=\"file\" id=\"file\" type=\"file\"/><br>");
    Append(&html, "<input type=\"submit\" value=\"Submit\">");
    Append(&html, "</form>");
    Append(&html, "</html>\n");
    free(namelist);
    namelist = NULL;

    return html;
}

/* Xem lại httpserver-v1.c */
void Send(int c, char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int tmp = send(c, data + sent, len - sent, 0);
        if (tmp > 0)
            sent += tmp;
        else
            break;
    }
}

/* Tìm trong `data`, bắt đầu từ `offset`, một chuỗi `pattern` dài `length` byte.
 * Trả về vị trí tìm thấy hoặc NULL nếu không tìm thấy. 
 */
char* FindPattern(char* pattern, char* data, int offset, int length) {
    for (int i = offset; i < length; i++)
        if (pattern[0] == data[i]) {
            int k = 0;
            for (k = 0;k < strlen(pattern);k++)
                if (pattern[k] != data[i + k])
                    break;
            if (k == strlen(pattern))
                return data + i;
        }
    return NULL; 
}

void* ClientThread(void* arg)
{
    int* tmp = (int*)arg;
    int c = *tmp;
    free(tmp); 
    tmp = NULL;
    char* buffer = NULL;
    char tmpc;

    /* Để hỗ trợ tải lên tệp tin, máy chủ cần biết các đề mục mà trình duyệt
     * đã gửi. Do đó, thay vì chỉ nhận hết dòng đầ̀u tiên, nhận hết các đề mục
     * của yêu cầu. 
     */
    while (0 == 0) {
        int r = recv(c, &tmpc, 1, 0);
        if (r > 0) {
            int oldLen = buffer == NULL ? 0 : strlen(buffer);
            buffer = (char*)realloc(buffer, oldLen + 2);
            buffer[oldLen] = tmpc;
            buffer[oldLen + 1] = 0;
            if (strstr(buffer,"\r\n\r\n") != NULL)
                break;
        } else
            break;
    }
    
    if (strlen(buffer) > 0) {
        printf("Socket %d, Received: %s\n", c, buffer);
        char method[1024] = {0};
        char path[1024] = {0};
        char ver[1024] = {0};
        sscanf(buffer, "%s%s%s", method, path, ver);
     
        if (strcmp(method,"GET") == 0) {

            while (strstr(path,"%20") != NULL) {
                char* tmp = strstr(path, "%20");
                tmp[0] = ' ';
                strcpy(tmp + 1, tmp + 3);
            }

            if (strcmp(path,"/") == 0) {
                char* html = BuildHTML(currentPath);
                char httpok[1024] = {0};
                sprintf(httpok, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\nConnection: Keep-Alive\r\n\r\n", (int)strlen(html));
                Send(c, (char*)httpok, strlen(httpok));
                Send(c, (char*)html, strlen(html));
                close(c);

            } else if (strcmp(path,"/favicon.ico") == 0) {
                /* Xử lý riêng với biểu tượng thu nhỏ của trang: luôn trả về 404. */
                char http404[1024] = {0};
                char* html = "<html><H><B>RESOURCE NOT FOUND</B></H></html>";
                sprintf(http404, "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", (int)strlen(html));
                Send(c, (char*)http404, strlen(http404));
                Send(c, (char*)html, strlen(html));
                close(c);

            } else if (strstr(path, "*") != NULL) { //FOLDER
                path[strlen(path) - 1] = 0; //Bo * o cuoi di
                char* html = BuildHTML(path);
                char httpok[1024] = {0};
                sprintf(httpok, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", (int)strlen(html));
                Send(c, (char*)httpok, strlen(httpok));
                Send(c, (char*)html, strlen(html));
                close(c);

            } else { //FILE
                FILE* f = fopen(path, "rb");
                if (f != NULL) {
                    fseek(f, 0, SEEK_END);
                    int size = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    char* data = (char*)calloc(size, 1);
                    int r = 0;
                    while (r < size) {
                        int tmp = fread(data + r, 1, size, f);
                        r += tmp;
                    }

                    char httpok[1024] = {0};
                    /* Chọn đề mục Content-Type phù hợp với định dạng tệp tin. */
                    char ct[128] = {0};
                    if (strstr(path, ".txt") != NULL)
                        strcpy(ct, "text/html");        
                    else if (strstr(path, ".png") != NULL)
                        strcpy(ct, "image/png");    
                    else if (strstr(path, ".mp4") != NULL)
                        strcpy(ct, "application/octet-stream"); // "video/mp4"
                    else if (strstr(path, ".pdf") != NULL)
                        strcpy(ct, "application/pdf");    
                    else 
                        strcpy(ct, "application/octet-stream");    

                    sprintf(httpok, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", ct, size);
                    Send(c, httpok, strlen(httpok));
                    Send(c, data, size);
                    fclose(f);   
                    close(c);                      
                } else {
                    //SEND 404 NOT FOUND
                    char http404[1024] = {0};
                    char* html = "<html><H><B>RESOURCE NOT FOUND</B></H></html>";
                    sprintf(http404, "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", (int)strlen(html));
                    Send(c, (char*)http404, strlen(http404));
                    Send(c, (char*)html, strlen(html));
                    close(c);
                }
            }
        } else if (strcmp(method, "POST") == 0) {
            /* Coi là người dùng đang tải lên tệp tin. */
            char header_name[1024] = { 0 };
            int length = 0;

            /* Đề mục Content-Length: Lượng dữ liệu đang được gửi. 
             * Cần nhận chính xác theo con số này. */
            char* ct = strstr(buffer, "Content-Length:");
            sscanf(ct, "%s%d", header_name, &length);

            char* data = (char*)calloc(length, 1);
            int r = 0;
            /* Nhận phần nội dung của yêu cầu POST,
             * tức là thông tin biểu mẫu.
             * Các đề mục đã được nhận hết từ trước rồi. */
            while (r < length) {
                int tmp = recv(c, data + r, length - r, 0);
                if (tmp > 0)
                    r += tmp;
                else
                    break;
            }       
            if (r == length) {
                /* Tách tên tệp tin tải lên. */
                char* start = strstr(data, "filename=\"") + strlen("filename=\"");
                char* end = strstr(start, "\"") - 1;
                char* filename = (char*)calloc(end - start + 2, 1);
                strncpy(filename, start, end - start + 1);

                /* Đặt tệp tin trong thư mục hiện tại. */
                char* fullpath = (char*)calloc(strlen(path) + strlen(filename) + 1, 1);
                sprintf(fullpath, "%s%s",path, filename);
                free(filename);
                filename = NULL;

                /* Tách dấu phân cách của biểu mẫu. */
                FILE* f = fopen(fullpath, "wb");

                end = strstr(data, "\r\n");
                char* pattern = (char*)calloc(end - data + 1, 1);
                strncpy(pattern, data, end - data);

                /* Tách nội dung tệp tin tải lên và ghi ra đĩa. 
                 * Nội dung tệp là một trường trong biểu mẫu và được phân cách bởi
                 * dấu phân cách ờ trên. */
                start = strstr(data, "\r\n\r\n") + 4;
                end = FindPattern(pattern, start, 0, length);
                fwrite(start, 1, end - start, f);
                fclose(f);
            }
            free(data);
            data = NULL;
            /* Sau khi hoàn tất tải lên, chuyển hướng người dùng đến
             * trang báo thành công. */
            const char* html = "<html>DONE UPLOADING</html>";
            char httpok[1024] = {0};
            sprintf(httpok, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", (int)strlen(html));
            Send(c, httpok, strlen(httpok));
            Send(c, (char*)html, strlen(html));
            close(c); 
        }
    }
    return NULL;
}
int main() {
    strcpy(currentPath, "/");
    SOCKADDR_IN myaddr, caddr;
    int clen = sizeof(caddr);
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(5000);
    myaddr.sin_addr.s_addr = 0;
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    bind(s, (SOCKADDR*)&myaddr, sizeof(myaddr));
    listen(s, 10);
    while (0 == 0) {
        int c = accept(s, (SOCKADDR*)&caddr, (socklen_t*)&clen);
        pthread_t tid;
        int* arg = (int*)calloc(1, sizeof(int));
        *arg = c;
        pthread_create(&tid, NULL, ClientThread, (void*)arg);
    }
}
