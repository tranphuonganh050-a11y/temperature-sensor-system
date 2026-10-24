# Bức tranh tổng quan: 
- Chương trình quản lý lô hàng chuỗi cung ứng lạnh. Cảm biến IoT đo nhiệt độ, nếu nhiệt độ vượt ngưỡng an toàn thì chương trình cảnh báo 

Chương trình chia thành 4 layer chính: 
- Layer 1: Domain: Định nghĩa các class chính
- Layer 2: Application: Điều phối: nhận yêu cầu, gọi Domain xử lý, phát cảnh báo
- Layer 3: Infrastructure: Lưu file, cảm biến giả lập 
- Layer 4: Presentation: Nhập dữ liệu từ bàn phím, in kết quả ra màn hình 

- Quy tắc chung: tầng trên dùng tầng dưới (Presentation --> Application --> Domain), còn Domain không biết gì về các tầng khác 

