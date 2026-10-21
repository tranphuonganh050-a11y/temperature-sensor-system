# ColdChain C++ — bản đã chỉnh sửa

## Cách biên dịch

Dùng g++ (C++17) tại thư mục này:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic Temperature.cpp -o TemperatureApp_fixed
```

Chỉ biên dịch `Temperature.cpp` vì đây là file chứa `main()`. Không biên dịch đồng thời các file `.cpp` đang được include trực tiếp trong nhau.

## Các chỉnh sửa chính

- `Domain.cpp`: lưu nhiệt độ, tọa độ và thời gian của lần đo mới nhất, kể cả khi nhiệt độ an toàn.
- `Application.cpp`: kiểm tra dữ liệu tạo lô hàng; cung cấp hàm lấy ngưỡng an toàn đúng theo container; DTO lấy dữ liệu đo gần nhất.
- `Infrastructure.cpp`: ghi các vi phạm mới vào audit log thay vì ghi lặp lại toàn bộ vi phạm cũ mỗi lần lưu.
- `Presentation.cpp`: cho phép nhập mã lô hàng cần mô phỏng, kiểm tra mã tồn tại và dùng ngưỡng an toàn của chính lô hàng đó; xử lý một số lỗi nhập liệu.
- `Extension.cpp`: CSV xử lý dấu ngoặc kép trong tên sản phẩm; biểu đồ thanh ánh xạ theo khoảng nhiệt độ thay vì nhân trực tiếp nhiệt độ.
- `Temperature.cpp`: giữ vai trò file `main()` duy nhất.

## Lưu ý còn lại

`LICH_SU_KIEM_TRA.txt` hiện là audit log để xem lại các dòng sự kiện. Bản này **chưa khôi phục đầy đủ các đối tượng Shipment từ file khi khởi động lại**; muốn có persistence hoàn chỉnh cần thiết kế định dạng lưu và hàm đọc/parse tương ứng. File CSV báo cáo được ghi đè mỗi lần xuất.

Trạng thái `ALERTED` được giữ lại như dấu hiệu lô hàng từng vi phạm; bản này chưa có thao tác xác nhận giao hàng hoặc xóa cảnh báo. Cảm biến vẫn là dữ liệu mô phỏng, không phải thiết bị IoT thật.
