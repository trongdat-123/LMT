/* scandir.c: Minh họa duyệt thư mục bằng scandir().  */
#include <stdio.h>        /* printf(), sprintf(), fopen(), fputs(), fclose() */
#include <dirent.h>       /* scandir() */
#include <malloc.h>       /* realloc(), free() */
#include <string.h>       /* memset(), strlen() */
char* html = NULL; // Kết quả duyệt thư mục, dưới dạng html

/* Chèn xâu `name` vào sau xâu `phtml` */
void Append(char** phtml, const char* name) {
    int oldLen = *phtml == NULL ? 0 : strlen(*phtml); // Tính số ký tự cữ
    (*phtml) = (char*)realloc(*phtml, oldLen + strlen(name) + 1); // Cấp phát thêm
    memset(*phtml + oldLen, 0, strlen(name) + 1); // Điền 0 vào các byte nhớ được cấp phát thêm
    sprintf(*phtml + oldLen, "%s", name); // Nối xâu
}

int main() {
    struct dirent **namelist;  // Kết quả duyệt cây thư mục
    int n;                     // Số lượng bản ghi trong thư mục

    /* Duyệt thư mục hiện tại. */
    n = scandir(".", &namelist, NULL, NULL);
    /* Kết quả trả về là số lượng bản ghi trong thư mục hoặc -1
     * nếu có lỗi.
     * Nếu thành công, `namelist` trỏ đến danh sách các
     * cấu trúc `dirent` mô tả các bản ghi trong thư mục.
     * Định nghĩa cấu trúc `dirent`:
        struct dirent {
            ino_t          d_ino;       // Số hiệu Inode
            off_t          d_off;
            unsigned short d_reclen;
            unsigned char  d_type;      // Loại bản ghi
            char           d_name[256]; // Tên bản ghi
        };
     * Các kết quả trả về không có thứ tự cụ thể.
     * Cần giải phóng bộ nhớ cấp phát bởi scandir() sau khi xong,
     * bao gồm:
     *   - Cấp phát cho mảng `namelist`
     *   - Cấp phát cho từng cấu trúc `dirent`
     */

    if (n == -1) {
        printf("Error\n");
    } else {
        /* Định dạng đầu ra:
         * <html>
         *    <a href=(tên thư mục)><b>(tên thư mục)</b></a><br>
         *    <a href=  (tên tệp)  ><i>  (tên tệp)  </i></a><br>
         * </html>
         * */
        Append(&html,"<html>\n");
        while (n--) {
            Append(&html,"<a href = \"");
            Append(&html, namelist[n]->d_name);
            switch (namelist[n]->d_type) {
                case DT_DIR: // Thư mục
                    Append(&html,"\"><b>");
                    Append(&html, namelist[n]->d_name);
                    Append(&html,"</b></a><br>\n");
                    break;
                case DT_REG: // Tệp
                    Append(&html,"\"><i>");
                    Append(&html, namelist[n]->d_name);
                    Append(&html,"</i></a><br>\n");
            }
            /* Giải phóng bộ nhớ cấp phát cho một cấu trúc */
            free(namelist[n]);
        }
        Append(&html,"</html>\n");
        /* Giải phóng bộ nhớ cấp phát cho mảng */
        free(namelist);
        namelist = NULL;

        /* Lưu kết quả vào tệp `scandir.html` */
        FILE* f = fopen("scandir.html", "wt");
        fputs(html, f);
        fclose(f);

        free(html);
        html = NULL;
    }
}
