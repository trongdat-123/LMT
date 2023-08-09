/* httpserver-v1.c: Máy chủ cung cấp giao diện duyệt hệ thống tệp tin qua giao thức HTTP. */

/* Lưu ý cho bạn đọc:
 * Đây là một tài liệu rất dài. Khuyến khích bắt đầu đọc từ main() và lần lượt theo
 * thứ tự các lời gọi hàm thay vì từ trên xuống.
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

char currentPath[1024] = {0}; // Đường dẫn tới thư mục đang cung cấp

/* BUG:
 * Như có thể thấy, đường dẫn hiện thời là một biến toàn cục.
 * Điều này có nghĩa là tất cả các luồng đều dùng chung một "thư mục hiện hành",
 * dẫn đến hai hệ quả:
 *  - một kết nối thay đổi thư mục sẽ ảnh hưởng tới tất cả các kết nối còn lại.
 *  - các kết nối mới sẽ bắt đầu duyệt tại thư mục hiện hành của kết nối trước đó.
 *
 * Việc sửa lỗi này để đảm bảo các kết nối có thể duyệt cây thư mục độc lập với nhau
 * xin dành cho bạn đọc.
 */

/* Hàm so sánh dùng trong scandir() để sắp xếp các kết quả duyệt thư mục.
 * Khuôn mẫu hàm này là
 *  int compare(const struct dirent **A, const struct dirent **B);
 * Kết quả trả về của hàm này sẽ quyết định thứ tự xuất hiện của A và B:
 *  - nhỏ hơn 0: A xếp trước B
 *  - lớn hơn 0: B xếp trước A
 *  - 0: không quan tâm thứ tự
 *
 * Ở đây, hàm CompareDR luôn ưu tiên xếp thư mục trước các loại tệp tin khác.
 */
int CompareDR(const struct dirent **A, const struct dirent **B)
{
    if ((*A)->d_type == DT_DIR)
        if ((*B)->d_type == DT_DIR)
            /* A và B cùng là thư mục. Sắp xếp theo bảng chữ cái. */
            return strcasecmp((*B)->d_name, (*A)->d_name);        
        else
            /* A là thư mục và B không là thư mục. Sắp xếp A nằm trước B. */
            return 1;
    else
        if ((*B)->d_type != DT_DIR)
            /* A và B cùng không là thư mục. Sắp xếp theo bảng chữ cái. */
            return strcasecmp((*B)->d_name, (*A)->d_name);        
        else
            /* A không là thư mục và B là thư mục. Sắp xếp B nằm trước A. */
            /* BUG: Phải trả về giá trị nhỏ hơn 0. */
            return 0;
}
void Append(char** phtml, const char* name)
{
    int oldLen = *phtml == NULL ? 0 : strlen(*phtml); //Tính số ký tự cữ
    (*phtml) = (char*)realloc(*phtml, oldLen + strlen(name) + 1); //Cấp phát thêm
    memset(*phtml + oldLen, 0, strlen(name) + 1); //Điền 0 vào các byte nhớ đc cấp phát thêm
    sprintf(*phtml + oldLen, "%s", name); //Nối xâu
}

/* Xây dựng nội dung trang HTML.
 * Cách hoạt động giống như mô tả hàm main() của scandir.c,
 * tuy nhiên có hai điểm khác biệt chính:
 *   - sử dụng hàm so sánh trong scandir() để sắp xếp thư mục trước tệp tin.
 *   - các liên kết có sử dụng dấu hiệu để phân biệt
 *     yêu cầu mở thư mục và yêu cầu lấy tệp tin.
 *     Dấu hiệu ở đây được lựa chọn là ký tự "*" ở cuối tên.
 *
 * Trang HTML sẽ có dạng:
 * 
 *  .       --> liên kết đến ".*"
 *  ..      --> liên kết đến "..*"
 *  dir1    --> liên kết đến "dir1*"
 *  dir2    --> liên kết đến "dir2*"
 *  < ... các thư mục tiếp ... >
 *  file1    --> liên kết đến "file1"
 *  file2    --> liên kết đến "file2"
 *  < ... các tệp tin tiếp ... >
 *
 *  Xem thêm mô tả trong ClientThread về hoạt động của máy chủ.
 */
char* BuildHTML(char* path)
{
    char* html = NULL;
    struct dirent **namelist;
    int n;
    n = scandir(path, &namelist, NULL, CompareDR);
    /* Ghi chú: Tham số thứ ba của scandir() là một hàm lọc kết quả.
     * Khuôn mẫu hàm này là
     *  int filter(const struct dirent *d);
     * Hàm trả về 0 nếu không muốn giữ lại d hoặc khác 0 nếu muốn giữ lại d.
     * Có thể dùng hàm này để lọc ra các tệp không mong muốn hiển thị.
     * Việc thêm tính năng này xin dành cho bạn đọc.
     */
    Append(&html,"<html>\n");
    while (n--) {
        if (namelist[n]->d_type == DT_DIR) {
            Append(&html,"<a href = \"");
            Append(&html, namelist[n]->d_name);
            Append(&html, "*");   // dấu hiệu thư mục
            Append(&html,"\"><b>");
            Append(&html, namelist[n]->d_name);
            Append(&html,"</b></a><br>\n");
        } else {
            Append(&html,"<a href = \"");
            Append(&html, namelist[n]->d_name);
            Append(&html,"\"><i>");
            Append(&html, namelist[n]->d_name);
            Append(&html,"</i></a><br>\n");
        }
        free(namelist[n]);
    }
    Append(&html,"</html>\n");
    free(namelist);
    namelist = NULL;
    return html;
}
/* Hàm gửi toàn bộ thông điệp, xử lý trường hợp 
 * trả về kết quả dài và cần yêu cầu gửi qua nhiều gói tin.
 */
void Send(int c, char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int tmp = send(c, data + sent, len - sent, 0);
        if (tmp > 0) {
            sent += tmp;
        } else
            break;
    }
}
/* Hàm chính của luồng. */
void* ClientThread(void* arg)
{
    int* tmp = (int*)arg;
    int c = *tmp;
    free(tmp); 
    tmp = NULL;

    char* buffer = NULL; // Nội dung yêu cầu
    char tmpc;

    /* Đã đến lúc phải nói về giao thức HTTP và hoạt động của trình duyệt Web.
     *
     * Tài liệu định nghĩa chính thức của giao thức HTTP/1.1 là RFC9110 và RFC9112:
     * https://www.rfc-editor.org/rfc/rfc9110.html
     * https://www.rfc-editor.org/rfc/rfc9112.html
     *
     *
     * 1. Giao thức HTTP hoạt động như thế nào?
     * Giao thức HTTP là một giao thức truyền dữ liệu hướng chủ - khách
     * (client - server).
     * Các bên trong giao thức có nhiệm vụ:
     * - Phía máy khách: Gửi yêu cầu (request) và nhận phản hồi (response)
     * - Phía máy chủ: Nhận yêu cầu (request) và gửi phản hồi (response)
     *
     * Các yêu cầu HTTP được gửi qua giao thức TCP.
     * Tuy nhiên, các yêu cầu HTTP là độc lập với nhau và với giao thức tầng
     * giao vận. Tức là trên lý thuyết có thể gửi HTTP qua UDP (tất nhiên,
     * việc này sẽ chỉ có ý nghĩa khi hai bê chủ và khách cùng đồng ý dùng UDP).
     *
     *
     * Quy ước ký hiệu:
     * SP: Dấu cách (ASCII mã 0x20, ' ')
     * CRLF: Dấu trả về (ASCII mã 0x0D, '\r') + Dấu xuống dòng (ASCII mã 0x0A, '\n')
     *
     * <>: biểu thị một trường (một chuỗi byte liên tiếp) trong giao thức.
     * []: biểu thị một trường trong giao thức, có thể có hoặc không.
     * "": biểu thị một chuỗi ký tự sử dụng y nguyên trong giao thức
     * 
     * CHÚ Ý:
     * Các dấu cách và xuống dòng trong cách viết nhằm mục đích phân cách 
     * các trường cho người đọc, KHÔNG PHẢI là dấu cách hoặc xuống dòng 
     * trong giao thức. Toàn bộ các dấu cách hoặc xuống dòng trong 
     * giao thức sẽ được chỉ ra cụ thể bằng các ký hiệu SP và CRLF như trên.
     *
     * 
     * 1.1. Yêu cầu HTTP
     *
     * Một yêu cầu HTTP có dạng như sau:
     *   <dòng-bắt-đầu> CRLF
     *   [<dòng-đề-mục-1> CRLF]
     *   [<dòng-đề-mục-2> CRLF]
     *   [...]
     *   CRLF
     *   [nội-dung]
     * Trong đó
     *   - <dòng-bắt-đầu> có dạng
     *     <phương-thức> SP <đường-dẫn> SP <phiên-bản>
     *     Trong đó
     *       - <phương-thức>: Biểu thị loại yêu cầu HTTP.
     *         Phương thức là một trong các phương thức đã được định nghĩa:
     *           "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT"
     *       - <đường-dẫn>: Một chuỗi ký tự chỉ ra đối tượng liên quan.
     *         Trong trường hợp tên của đối tượng chứa các ký tự đặc biệt,
     *         các ký tự đó được mã hoá theo quy cách trong mục 1.3.
     *       - <phiên-bản>: Biểu thị phiên bản của giao thức mà phía gửi đang sử dụng.
     *         Phiên bản có dạng:
     *           "HTTP/" <phiên-bản-chính> "." <phiên-bản-con>
     *         <phiên-bản-chính> và <phiên-bản-con> là các số nguyên không dấu dưới dạng
     *         ký tự ASCII. Ví dụ: HTTP/1.1
     *   - <dòng-đề-mục> có dạng
     *     <tên-đề-mục> ":" SP <nội-dung-đề-mục>
     *     Các đề mục mang các thông điều khiển giao thức như ngày tháng,
     *     loại nội dung hỗ trợ, hay quy cách nội dung phần sau.
     *     Danh sách và ý nghĩa cụ thể của các đề mục xem trong RFC9110.
     *     Tên đề mục có phân biệt hoa thường.
     *     Các đề mục không có thứ tự cụ thể.
     *
     *   - [nội-dung]: Phụ thuộc vào phương thức cụ thể.
     *
     *
     * 1.2. Phản hồi HTTP
     *
     * Một phản hồi HTTP có dạng như sau:
     *   <dòng-trạng-thái> CRLF
     *   [<dòng-đề-mục-1> CRLF]
     *   [<dòng-đề-mục-2> CRLF]
     *   [...]
     *   CRLF
     *   [nội-dung]
     * Trong đó
     *   - <dòng-trạng-thái> có dạng
     *     <phiên-bản> SP <mã-trạng-thái> SP [mô-tả-lý-do]
     *     Trong đó
     *       - <phiên-bản>: tương tự như phiên bản trong phần yêu cầu.
     *       - <mã-trạng-thái>: chỉ ra kết quả thực hiện yêu cầu.
     *         Mã trạng thái là một số nguyên dưới dạng ba chữ số ASCII.
     *
     *         Chữ số đầu tiên biểu thị các loại phản hồi:
     *
     *         - 1xx: Thông tin thêm
     *         - 2xx: Thành công
     *         - 3xx: Thông báo chuyển hướng
     *         - 4xx: Lỗi do máy khách
     *         - 5xx: Lỗi do máy chủ
     *         
     *       - [mô-tả-lý-do]: một đoạn ký tự tương ứng với <mã-trạng-thái>
     *         Đoạn ký tự này kéo dài đến hết dòng trạng thái và có thể chứa các ký tự
     *         bất kỳ.  
     *         Danh sách, ý nghĩa cụ thể, và mô tả của các mã trạng thái xem trong RFC9110.
     *   - <dòng-đề-mục> và [nội-dung] có ý nghĩa tương tự như đề mục trong phần yêu cầu.
     *
     * 1.3. Mã hoá đường dẫn
     *  
     * Các ký tự ngoài chữ cái, chữ số và một vài ký tự khác trong đường dẫn
     * được biểu diễn dưới dạng "mã hoá phần trăm"
     * (gọi tên như vậy vì cú pháp sử dụng ký hiệu phần trăm).
     * Cú pháp mã hoá:
     *   "%" <mã-ASCII>
     * Với <mã-ASCII> là hai chữ số hệ thập lục phân tương ứng với mã ASCII của ký tự đó.
     * Ví dụ: 
     *   - Dấu cách (ASCII 0x20) được biểu diễn là "%20"
     *   - Một tệp tin có tên "Alice & Bob's 99%" được biểu diễn là
     *      "Alice%20%26%20Bob%20%27s%2099%25"
     *
     * 2. Hoạt động của trình duyệt
     *
     * Trình duyệt Web là một chương trình truy cập tài nguyên trên Internet.
     * Nhiệm vụ của trình duyệt là gửi yêu cầu đến các máy chủ theo yêu cầu của người dùng,
     * và trình bày dữ liệu nhận về cho người dùng.
     * Giao thức chính được sử dụng là HTTP.
     *
     * Khi yêu cầu trình duyệt truy cập đến một máy chủ mà không chỉ định tài nguyên,
     * trình duyệt sẽ gửi một yêu cầu HTTP như sau:
     * 
     *   "GET" SP "/" SP "HTTP/" <phiên-bản> CRLF
     *   [các-đề-mục-nếu-có]
     *   CRLF
     *
     * Khi yêu cầu trình duyệt truy cập đến một máy chủ và có chỉ định tài nguyên,
     * trình duyệt sẽ gửi một yêu cầu HTTP như sau:
     * 
     *   "GET" SP "/" <đường-dẫn-chỉ-định> SP "HTTP/" <phiên-bản> CRLF
     *   [các-đề-mục-nếu-có]
     *   CRLF
     *
     * Ví dụ: Nhập "https://www.google.com/search/howsearchworks/" trên thanh địa chỉ của trình duyệt Chrome,
     *   Trình duyệt gửi yêu cầu tới máy chủ tại "www.google.com" với nội dung:
     *
     *   "GET" SP "/search/howsearchworks/" SP "HTTP/1.1" CRLF
     *   ... các-đề-mục ...
     *   CRLF
     *
     * 3. HTML
     *
     * HTML là một loại tài nguyên văn bản trên Internet.
     * Nhiệm vụ chính của HTML là cung cấp nội dung văn bản, cùng cách để tìm đến các tài nguyên khác.
     *
     * HTML có thể chứa các "liên kết". Các liên kết biểu diễn cách để tìm tài nguyên đó.
     * Khi người dùng yêu cầu truy cập tới một liên kết trên tài liệu HTML, trình duyệt thực hiện truy cập
     * tới tài nguyên giống như truy cập có chỉ định tài nguyên ở mục 2, sử dụng địa chỉ máy chủ trong 
     * liên kết làm địa chỉ để gửi yêu cầu tới, sử dụng đường dẫn trong liên kết làm đường dẫn tới tài nguyên.
     * 
     * Các liên kết có thể chỉ đến tài nguyên nằm trên cùng một máy chủ với tài liệu HTML. Khi đó phần
     * địa chỉ máy chủ có thể được bỏ trống.
     *
     * 4. Kiến trúc của chương trình máy chủ này
     *
     * Chương trình cung cấp một giao diện duyệt cây thư mục qua giao thức HTTP.
     *
     * Tài nguyên được phục vụ là một tài liệu HTML biểu thị nội dung thư mục hiện hành.
     * Tài liệu này được tự sinh ra trong quá trình hoạt động của chương trình.
     * Tài liệu này có chứa các liên kết, được thiết kế để khi trình duyệt truy cập tới 
     * các liên kết đó, các yêu cầu do trình duyệt gửi đến có thể dùng để điều khiển
     * quá trình duyệt cây thư mục, bao gồm
     *  - Chuyển thư mục hiện hành.
     *  - Gửi nội dung tệp tin trong thư mục hiện hành.
     *
     * Tài liệu HTML được sinh ra có dạng:
     * 
     *  .       --> liên kết đến ".*"
     *  ..      --> liên kết đến "..*"
     *  dir1    --> liên kết đến "dir1*"
     *  dir2    --> liên kết đến "dir2*"
     *  < ... các thư mục tiếp ... >
     *  file1    --> liên kết đến "file1"
     *  file2    --> liên kết đến "file2"
     *  < ... các tệp tin tiếp ... >
     * 
     * Trạng thái duyệt thư mục được duy trì thông qua đường dẫn đến thư mục hiện hành.
     * Đường dẫn này được thay đổi qua yêu cầu đến tài nguyên có ký tự cuối là "*".
     * Máy chủ sẽ trả về tài liệu HTML cho phép duyệt thư mục mới.
     *
     * Các yêu cầu còn lại được coi là yêu cầu đến tài nguyên tệp tin và máy chủ
     * sẽ gửi lại nội dung của tệp tin nếu có.
     *
     * Thiết kế hiện tại của máy chủ có nhiều điểm yếu. Xem các chú thích với nội dung
     * "BUG:" và "TODO:" để biết thêm.
     * Khuyến khích bạn đọc cải thiện máy chủ này.
     *
     */ 

    /* Nhận một yêu cầu từ máy khách. */
    while (0 == 0) {
        /* Dần dần nhận yêu cầu bằng cách đọc từng ký tự một từ kết nối.
         * Hiện tại, máy chủ không quan tâm đến các đề mục hay nội dung của
         * yêu cầu, nên dừng nhận ngay khi hết dòng đầu tiên.
         * Lưu ý rằng cách này có hiệu năng cực kỳ thấp do phải gọi hệ thống nhiều lần.
         * Việc dùng vùng đệm để tăng hiệu suất nhận xin dành cho bạn đọc.
         */
        int r = recv(c, &tmpc, 1, 0);
        if (r > 0) {
            if (tmpc != '\r' && tmpc != '\n') {
                int oldLen = buffer == NULL ? 0 : strlen(buffer);
                buffer = (char*)realloc(buffer, oldLen + 2);
                buffer[oldLen] = tmpc;
                buffer[oldLen + 1] = 0; // Lính canh
            } else
                break;
        } else
            /* Kết nối đã đóng hoặc có lỗi khác. 
             * Vẫn thử kiểm tra phần yêu cầu đã nhận được, vì có thể
             * phía máy khách đã kịp gửi đủ một yêu cầu HTTP
             * (nhưng chưa kịp xuống dòng).
             */
            break;
    }
    if (strlen(buffer) > 0) {
        printf("Socket %d, Received: %s\n", c, buffer);

        /* Tách các phần phương thức, đường dẫn và phiên bản của yêu cầu. */
        char method[1024] = {0};
        char path[1024] = {0};
        char ver[1024] = {0};
        sscanf(buffer, "%s%s%s", method, path, ver);

        /* Xác định các thông số của yêu cầu và trả lời một cách phù hợp. */
        if (strcmp(method,"GET") == 0) {
            /* Phương thức GET: Yêu cầu truy hồi tài nguyên tại đường dẫn. */
            /* Giải mã các ký tự đặc biệt nếu có.
             * Ở đây chỉ giải mã dấu cách ("%20")
             */
            while (strstr(path,"%20") != NULL) {
                char* tmp = strstr(path, "%20");
                tmp[0] = ' ';
                strcpy(tmp + 1, tmp + 3);
            }
            if (strcmp(path, "/") == 0) {
                /* Trình duyệt yêu cầu tại đường dẫn mặc định,
                 * nghĩa là không có yêu cầu chuyển hướng. 
                 * Gửi về phía khách nội dung của thư mục hiện hành.
                 *
                 * Thông điệp phản hồi có chứa các đề mục:
                 *   - Content-Type: Quy cách nội dung (text/html)
                 *   - Content-Length: Độ dài nội dung
                 *   - Connection: Phía máy chủ sẽ làm gì với kết nối
                 *      (keep-alive = giữ kết nối mở)
                 */
                char* html = BuildHTML(currentPath);
                char httpok[1024] = {0};
                sprintf(httpok, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\nConnection: Keep-Alive\r\n\r\n", (int)strlen(html));
                Send(c, (char*)httpok, strlen(httpok));
                Send(c, (char*)html, strlen(html));
            } else {
                if (strstr(path, "*") != NULL) { // FOLDER
                    /* Chuyển đường dẫn hiện tại thành thư mục chỉ định trong đường dẫn. */
                    if (currentPath[strlen(currentPath) - 1] == '/')
                        currentPath[strlen(currentPath) - 1] = 0;
                    path[strlen(path) - 1] = 0; // Bo * o cuoi di
                    if (strcmp(path,"/.") == 0) {
                        /* Thư mục chỉ định: Thư mục hiện hành. */
                        // DO NOTHING
                    } else if (strcmp(path,"/..") == 0) {
                        /* Thư mục chỉ định: Thư mục cha của thư mục hiện hành. */ 
                        /* Xoá thành phần cuối của đường dẫn hiện tại nếu có thể. */
                        int i = strlen(currentPath) - 1;
                        while (i >= 0) {
                            if (currentPath[i] != '/')
                                currentPath[i] = 0;
                            else
                                break;
                            i -= 1;
                        }
                    } else {
                        /* Thư mục chỉ định: Một thư mục nằm trong thư mục hiện hành. */ 
                        /* Thêm tên thư mục vào thành phần cuối của đường dẫn hiện tại. */
                        sprintf(currentPath + strlen(currentPath), "%s", path);
                    }
                    /* Để không mất đi thành phần cuối cùng là thư mục gốc ("/"),
                     * nếu kết quả di chuyển là đường dẫn rỗng, thêm lại thư mục gốc. */
                    if (strlen(currentPath) == 0) {
                        strcpy(currentPath,"/");
                    }
                    /* Sau khi chuyển hướng, gửi về nội dung tương tự như ở trên, */
                    char* html = BuildHTML(currentPath);
                    char httpok[1024] = {0};
                    sprintf(httpok, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", (int)strlen(html));
                    Send(c, (char*)httpok, strlen(httpok));
                    Send(c, (char*)html, strlen(html));
                } else { //FILE
                    if (currentPath[strlen(currentPath) - 1] == '/') {
                        currentPath[strlen(currentPath) - 1] = 0;
                    }
                    sprintf(currentPath + strlen(currentPath), "%s", path);
                    FILE* f = fopen(currentPath, "rb");
                    if (f != NULL) {
                        /* Gửi nội dung tệp tin. */
                        fseek(f, 0, SEEK_END);
                        int size = ftell(f);
                        fseek(f, 0, SEEK_SET);
                        char* data = (char*)calloc(size, 1);
                        fread(data, 1, size, f);
                        char httpok[1024] = {0};
                        /* BUG: Máy chủ đang khai man quy cách tệp tin là văn bản.
                         * Như thường lệ, việc sửa lỗi xin dành cho bạn đọc. */
                        sprintf(httpok, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", size);
                        Send(c, httpok, strlen(httpok));
                        Send(c, data, size);
                    } else {
                        // SEND 404 NOT FOUND
                        /* TODO: Gửi kết quả không tìm thấy tệp */
                    }
                    fclose(f);                         
                }
            }
        } else if (strcmp(method, "POST") == 0) {
            /* TODO: Phương thức POST: Yêu cầu truy hồi tài nguyên với tham số. */
        }
    }
    
    /* TODO:
     * Các luồng con hiện chỉ phục vụ một yêu cầu duy nhất của máy khách
     * thay vì liên tục tiếp nhận các yêu cầu tiếp theo. 
     * Lý do là vì cơ chế giải mã thông điệp HTTP chưa hoàn thiện.
     * Máy chủ sẽ cần hiểu cách phân tách các thông điệp HTTP hoàn chỉnh trước khi
     * có thể phục vụ nhiều yêu cầu trên cùng một kết nối.
     */

    /* BUG: Máy chủ không đóng socket khi kết thúc. */
    return NULL;
}
int main() {
    /* Thiết lập trạng thái duyệt cây thư mục. */
    strcpy(currentPath, "/mnt/c");
    /* Tạo socket lễ tân. */
    SOCKADDR_IN myaddr, caddr;
    int clen = sizeof(caddr);
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(5000);
    myaddr.sin_addr.s_addr = 0;
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    bind(s, (SOCKADDR*)&myaddr, sizeof(myaddr));
    listen(s, 10);
    /* Nhận kết nối. */
    while (0 == 0) {
        int c = accept(s, (SOCKADDR*)&caddr, (socklen_t*)&clen);
        /* Sinh ra luồng con để trao đổi dữ liệu với máy khách.
         * Truyền cho tiến trình con tham số là
         * số hiệu của socket ứng với kết nối này.
         */
        pthread_t tid;
        int* arg = (int*)calloc(1, sizeof(int));
        *arg = c;
        pthread_create(&tid, NULL, ClientThread, (void*)arg);
        /* BUG: Máy chủ đang không hợp nhất hay tách các luồng,
         * dẫn đến rò rỉ tài nguyên sau mỗi kết nối. */
    }
}
