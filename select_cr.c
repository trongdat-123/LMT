/* select_cr.c: Minh hoạ xử lý đa kênh bằng select(). */
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <stdlib.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int* g_clients = NULL; // Danh sách máy trạm đang kết nối
int* g_status = NULL;  // Trạng thái của từng máy
int g_count = 0;       // Số lượng máy trạm đang kết nối
int main() {
    /* Thiết lập Socket lễ tân. Xem lại các bài trước nếu cần giải thích thêm. */
    SOCKADDR_IN myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(5000);
    myaddr.sin_addr.s_addr = 0;
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    bind(s, (SOCKADDR*)&myaddr, sizeof(myaddr));
    listen(s, 10);

    SOCKADDR_IN caddr;        // Địa chỉ máy trạm
    int clen = sizeof(caddr); // Độ dài địa chỉ
    fd_set set; // Tập yêu cầu thăm dò trạng thái
    while (0 == 0) {

        /* Cơ bản hoạt động:
         *  - Thăm dò các socket có khả năng đọc,
         *    tức là các socket đã có dữ liệu gửi đến.
         *  - Nhận dữ liệu từ các socket này. Việc nhận dữ liệu là không dừng
         *    (recv() sẽ trả về dữ liệu ngay lập tức).
         *  - ...
         */

        /* Cú pháp select():
         * select(maxfd, readset, writeset, exceptset, timeout)
         *
         *    - maxfd: số hiệu lớn nhất chưa thiết lập trong các set
         *       readset, writeset và exceptset. FD_SETSIZE là con số tối đa.
         *    - readset:  tập thăm dò khả năng đọc (nhận)
         *    - writeset: tập thăm dò khả năng ghi (gửi)
         *    - readset:  tập thăm dò các trường hợp ngoại lệ (lỗi kết nối, ...)
         *    - timeout:  thời gian tối đa chờ đợi, hoặc NULL để chờ vô tận
         *       (tức là chờ đến khi có một tệp thăm dò thay đổi)
         *
         *  Giá trị trả về:
         *  Tổng số số hiệu đã thoả mãn trong mỗi tập, hoặc -1 nếu như có lỗi.
         *  Có thể trả về 0 (không có thay đổi) nếu thời gian chờ không phải là vô tận.
         *
         *  Thiết lập các tập thăm dò như thế nào? 
         *
         *  fd_set set;
         *
         *  FD_SET(fd, &set);   // thêm (fd) vào (set)
         *  FD_CLR(fd, &set);   // xoá (fd) khỏi (set)
         *  FD_ZERO(&set);      // xoá trắng (set)
         *  FD_ISSET(fd, &set); // (fd) có nằm trong (set) ?
         *
         *  Chú ý rằng vì select() thay đổi nội dung các tập truyền vào nên
         *  mỗi lần gọi sẽ cần thiết lập lại các tập cho phù hợp. */


        FD_ZERO(&set);   // khởi tạo tập: rỗng
        FD_SET(s, &set); // Tập thăm dò: socket lễ tân
        /* Tập thăm dò: socket lễ tân + các socket ứng với các máy trạm đang sẵn sàng. */
        for (int i = 0; i < g_count; i++)
            if (g_status[i] == 1) 
                FD_SET(g_clients[i], &set);
        /* FIXME: Quá trình thiết lập trên có thể được tối ưu thêm.
         * Để đảm bảo không thăm dò các kết nối đã huỷ, tập thăm dò được xoá trắng mỗi lần.
         * Tuy nhiên chỉ cần loại bỏ các kết nối đó khỏi tập là đủ,
         * vì phần sau đã có kiểm tra và không thêm lại.
         * Xoá trắng tập làm lãng phí nhiều thời gian hơn. 
         * Việc tối ưu hoá xin dành cho bạn đọc. */ 

        /* Thăm dò với các điều kiện.
         * Tập thăm dò đọc: lễ tân & các máy trạm
         * Tập thăm dò ghi: không
         * Tập thăm dò ngoại lệ: không
         * Thời gian chờ: Vô tận
         */
        select(FD_SETSIZE, &set, NULL, NULL, NULL);

        /* Sau khi select() trả về, các tập thăm dò sẽ được thay đổi 
         * để phản ánh điều kiện thăm dò có thoả mãn hay không.
         * Một số hiệu còn nằm trong tập tương ứng điều kiện thăm dò của số hiệu đó thoả mãn.
         * Chú ý rằng select() sẽ không bao giờ thêm số hiệu vào tập mà chỉ loại bỏ các
         * số hiệu không thoả mãn.
         */
        if (FD_ISSET(s, &set)) {
            /* Socket lễ tân có thể đọc đồng nghĩa với việc
             * có một máy trạm đang chờ phục vụ. 
             * Chấp nhận yêu cầu và thêm vào danh sách hiện hoạt, đồng thời
             * đánh dấu máy trạm này là sẵn sàng. */
            int c = accept(s, (SOCKADDR*)&caddr, (socklen_t*)&clen);
            g_clients = realloc(g_clients, (g_count + 1) * sizeof(int));
            g_status = realloc(g_status, (g_count + 1) * sizeof(int));
            g_clients[g_count] = c;
            g_status[g_count] = 1;
            g_count += 1;
            printf("%d connected\n", c);
        }
        /* Kiểm tra các socket với các máy trạm. 
         * Chỉ quan tâm các máy đang sẵn sàng. */
        for (int i = 0; i < g_count; i++) {
            if (g_status[i] == 1 && FD_ISSET(g_clients[i], &set)) {
                /* Socket có thể đọc, tức máy trạm đã gửi gì đó. */
                char buffer[1024] = { 0 };
                int r = recv(g_clients[i], buffer, sizeof(buffer) - 1, 0);
                if (r > 0) {
                    printf("%d: %s\n", g_clients[i], buffer);
                    /* Chuyển tiếp cho các máy còn lại. */
                    for (int k = 0; k < g_count; k++)
                        if (k != i && g_status[k] == 1)
                            send(g_clients[k], buffer, strlen(buffer), 0);
                } else {
                    /* Máy trạm đã đóng kết nối (r == 0) hoặc có lỗi xảy ra 
                     * với kết nối này (r == -1). 
                     * Đánh dấu máy trạm là bận.
                     * Các lần xử lý sau sẽ không quan tâm đến máy này nữa:
                     * Cũng có thể đóng kết nối ở đây (?)
                     */
                    g_status[i] = 0;
                }
            }
        }
    }
}
