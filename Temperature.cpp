// Main: lắp ráp các thành phần lại với nhau rồi cho chạy
#include "Infrastructure.cpp"
#include "Extension.cpp"
#include "Presentation.cpp"
#include <iostream>

int main(){
    // Tạo kho lưu trữ (Infrastructure)
    FileShipmentRepository repo("LICH_SU_KIEM_TRA.txt");

    // Tạo bên phát sự kiện (Application) // Tạo Publisher trước khi đăng kí 
    TelemetryProcessor telemetryProcessor; 
    
    // Tạo các Observer: 
    AlertService alertService;   // Domain: Tạo ra nhưng không đăng ký, thân hàm rỗng
    ShipmentAggregator aggregatorService;  // Domain: Tạo ra nhưng không đăng ký, thân hàm rỗng
    // Đăng ký thêm Feed Logger vào telemetryProcessor sẵn có
    AlertFeedLogger alertFeedLogger; // Extension: Được đăng ký, đang hoạt động thật

    // Đăng kí đối tượng
    telemetryProcessor.subscribe(&alertFeedLogger);
    
    // Tạo Server và thêm các phụ thuộc 
    ShipmentService shipmentService(&repo, &telemetryProcessor); 
    // Dependency Injection: tham số truyền vào &repo
    //Kiểu thật: FileShipmentRepository*
    //Kiểu mà Service nhìn thấy: IShipmentRepository* (interface)

    // Tạo giao diện và chạy CT 
    ConsoleMenu menu(shipmentService);
    menu.displayMainMenu(); 

    // Sau khi thoát menu: 
    // Khi cần xem danh sách cảnh báo (Alert Feed):
    alertFeedLogger.displayAlertFeed(shipmentService.getShipmentHistory ());

    if(AuditReportExporter::exportToCSV(shipmentService.getShipmentHistory())){
        cout << "Da xuat bao cao thanh cong !\n"; 
    }
    return 0; 
}
/*
# Nhận xét và hạn chế (đưa vào phần đánh giá)
1. Hai Observer chưa được đăng ký: alertService và aggregatorService được tạo nhưng không subscribe, thân hàm lại rỗng, nên chưa tham gia luồng nào. Trong báo cáo có thể trình bày là phần dự phòng để mở rộng.
2. ConsoleLoggerObserver chưa được dùng: lớp này có trong Infrastructure nhưng main không tạo.
3. Mọi thứ nối bằng #include các tệp .cpp: cách thông thường là tách tệp khai báo (.h) và tệp cài đặt (.cpp). Cách hiện tại gộp tất cả vào một lần biên dịch, đơn giản cho đề tài nhỏ nhưng khó mở rộng.
4. Việc lắp ráp ghi cứng trong main: tên tệp "LICH_SU_KIEM_TRA.txt" và danh sách observer đều viết thẳng vào code. Muốn đổi phải sửa và biên dịch lại. Có thể cải tiến bằng tệp cấu hình.
5. Không có xử lý ngoại lệ ở mức main: ConsoleMenu chỉ bắt lỗi trong phần mô phỏng; một ngoại lệ xảy ra ở chỗ khác sẽ làm chương trình dừng đột ngột.
6. Vòng lặp menu có thể lặp vô hạn nếu đầu vào bị đóng: nếu luồng nhập kết thúc (ví dụ nhấn Ctrl+D hoặc Ctrl+Z), mỗi lần đọc đều lỗi, cin.clear() xóa lỗi rồi lặp lại. Với người dùng bình thường, điều này không xảy ra.
7. Báo cáo cuối không báo lỗi: nếu exportToCSV thất bại (ví dụ tệp đang được mở bằng Excel nên bị khóa), main không in gì.
8. Bảng cảnh báo chỉ gồm phiên hiện tại, vì AlertFeedLogger giữ dữ liệu trong RAM và không nạp lại từ tệp.
9. Lỗi chính tả nhỏ trong chú thích: “Presentatio” (thiếu chữ n).
*/