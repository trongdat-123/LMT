/* forkex2.c: Minh họa xử lý tiến trình con */
#include <stdio.h>            /* printf(), getchar() */
#include <unistd.h>           /* fork(), sleep(), getpid() */
#include <signal.h>           /* signal() */
#include <sys/wait.h>         /* wait() */

void handler(int signum) {
    if (signum == SIGCHLD) {
        /* Một tiến trình con đã kết thúc.
         * Thu hồi giá trị trả về của nó để tiêu diệt zombie.
         * (PROGRAMMER VS ZOMBIES 2)
         */
        int status;
        pid_t pid;
        /* waitpid() trả về số hiệu tiến trình con nếu
         * có tiến trình con đã kết thúc, hoặc -1 nếu có lỗi.
         * Nếu có tuỳ chọn WNOHANG, waitpid() trả về 0
         * nếu chưa có tiến trình con nào kết thúc.
         * Nếu giá trị trả về lớn hơn 0, `status` chứa 
         * kết quả thực thi tiến trình.
         * Sử dụng WEXITSTATUS() để lấy giá trị trả về.
         * Lưu ý: Giá trị trả về là số không dấu 8 bit (unsigned char)
         */
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            printf("Process %d returned %d\n", pid, WEXITSTATUS(status));
        };
    }
}
int main() {
    /* Tín hiệu SIGCHLD được gửi tới tiến trình cha 
     * khi có tiến trình con đã kết thúc.
     * Đăng ký xử lý bằng handler() khi nhận được tín hiệu này.
     */
    signal(SIGCHLD, handler);

    /* Sinh ra các tiến trình con */
    if (fork() == 0) {
        printf("Hello World from Child 1 (PID %d)\n", getpid());
        sleep(1); // Mô phỏng tiến trình hoạt động
        printf("Child 1 return 0\n");   
        return 0;
    } else if (fork() == 0) {
        printf("Hello World from Child 2 (PID %d)\n", getpid());
        sleep(2); // Mô phỏng tiến trình hoạt động
        printf("Child 2 return -1\n");   
        return -1;         
    } else if (fork() == 0) {
        printf("Hello World from Child 3 (PID %d)\n", getpid());   
        sleep(1); // Mô phỏng tiến trình hoạt động
        printf("Child 3 return 1\n");   
        return 1;
    }
    getchar();
}
