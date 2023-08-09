/* pthreadchatroom.c: Minh hoạ máy chủ trò chuyện đa luồng. */
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

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
int *online = NULL;    // Số hiệu các kết nối hiện hoạt
int online_count = 0;  // Số lượng các kết nối hiện hoạt
pthread_mutex_t online_lock = PTHREAD_MUTEX_INITIALIZER; // Khoá kiểm soát truy cập danh sách trên

/* Hàm chính của luồng thực hiện nhận tin nhắ́n từ mộ̣t người
 * và chuyển tiếp cho những người đang trực tuyến còn lại. 
 * Tham số: Số hiệu kết nối mà từ đó thực hiện nhận. */
void* ClientThread(void *arg) {
    int self = *(int*)arg;
    free(arg);
    arg = NULL;

    char message[1024];
    int message_length;
    while (0 == 0) {
        memset(message, 0, sizeof(message));
        message_length = recv(self, message, sizeof(message), 0); 
        if (message_length > 0) {
            /* Chuyển tiếp thông điệp cho tất cả những người còn lại. */

            /* Chiếm độc quyền truy cập vào danh sách để
             * đảm bảo không bị sửa đổi trong lúc chuyển tiếp. */
            pthread_mutex_lock(&online_lock);
            for (int i = 0; i < online_count; i++)
                if (online[i] != self) {
                    send(online[i], message, message_length, 0);
                }
            pthread_mutex_unlock(&online_lock);
            printf("[%d] sent: %s\n", self, message);   
        } else {
            break;
        }
    }
    printf("[%d] disconnected\n", self);
    return NULL;
}
int main() {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    SOCKADDR_IN myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(5000);
    myaddr.sin_addr.s_addr = 0;
    bind(s, (SOCKADDR*)&myaddr, sizeof(myaddr));
    listen(s, 10);

    /* Luồng chính nhận các kết nối TCP từ ngoài vào.
     * Với mỗi kết nối, tạo ra một luồng để quản lý kế́t nối đó.
     * Tất cả các luồng đều chung danh sách kết nối hiện hoạt. */
    while (0 == 0) {
        SOCKADDR_IN caddr;
        int clen = sizeof(caddr);
        int c = accept(s, (SOCKADDR*)&caddr, (socklen_t*)&clen);  

        /* Cần chiếm độc quyền truy cập vào danh sách
         * khi thực hiện sửa đổi, do là tài nguyên dùng chung. */
        pthread_mutex_lock(&online_lock);
        online = realloc(online, (online_count + 1) * sizeof(int));
        online[online_count] = c;
        online_count += 1;      
        pthread_mutex_unlock(&online_lock);

        pthread_t tid;
        int* arg = calloc(1, sizeof(int));
        *arg = c;
        pthread_create(&tid, NULL, ClientThread, arg);
        /* Ta không quan tâm kết quả thực thi của luồng.
         * Tách luồng để không tiêu tốn tài nguyên, một luồng đã tách
         * sẽ không trở thành zombie khi kết thúc.
         * Một lựa chọn khác là định kỳ gọi pthread_join() để dọn dẹp
         * các luồng đã kết thúc một cách tương tự như waitpid(). */
        pthread_detach(tid);
    }
}
