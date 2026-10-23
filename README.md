# temperature-sensor-system
Hệ thống giám sát và điều phối các sản phẩm nhạy cảm với nhiệt độ (dược phẩm, thực phẩm tươi sống) trong chuỗi cung ứng 

# Kết quả hiện tại: 
- Nếu chọn (1): tạo lô hàng mới với id tăng dần sau mỗi lần tạo (chương trình sẽ truy xuất lại lịch sử để xem đang tạo đến id nào)

- Sau khi chọn (1): 
+ Nếu không chạy (2) - Mocking, mà chạy (3) - Xem lịch sử: Lô hàng mới tạo sẽ trong trạng thái IN_TRANSIT, còn các lô hàng đã chạy trước đó DELIVERED/COMPROMIZED
+ Nếu chọn (2) - Mocking: CT yêu cầu nhập lô hàng muốn test, nếu id chưa tồn tại, hoặc lô hàng đã ở trạng thái DELIVERED/COMPROMIZED sẽ không chạy nữa và yêu cầu chọn lại. Tất cả các lô hàng ngoài 2 tình huống trên đều có thể chạy mock

- Mock: Chạy 6 lần: mặc định 2 lần đầu bình thường, 4 lần sau lỗi (chạy thử để biết)
+ Lần nào lỗi mới in warning đỏ 
+ Hết 6 lần: in 1 dòng vi phạm bao nhiêu lần (vì code cố định 4 lần nên luôn in 4 lần) 

- Chạy (2) - Mocking xong mà chạy (3) - Lịch sử: Lô hàng vừa chạy sẽ chuyển về trạng thái COMPROMIZED (vì code để mặc định vượt quá 3 lần vi phạm thì lô hàng bị hủy) 

- Chạy (4): data sẽ được xuất sang file excel: trong file là code html (giống ở EXtention.cpp), muốn xem thì mở file này trong folder trên máy tính riêng để xem dưới dạng excel, vscode không hỗ trợ giao diện này. 

- Chạy (5): Chươgn trình in ra bảng tổng kết của các lô hàng vừa chạy mocking (chạy thử để biết) 

# Vấn đề hiện tại: 
1. Các lô hàng trong trạng thái IN_TRANSIT luôn bị mặc định tọa độ, nhiệt độ = 0 (vì lúc này chưa chạy mock nên chưa có data làm mẫu) 
--> Sửa để tọa độ, nhiệt độ IN_TRANSIT = tọa độ, nhiệt độ Mock 1 (hoặc như nào cũng được miễn khác 0 và không cảnh báo :))

2. Vì cố định 6 lần mock như trên, nên khi kết thúc mock, các lô hàng luôn trong trạng thái COMPROMIZED (có 4 lần vi phạm) 
--> Sửa để mỗi lần chạy lại mock là 1 kết quả ngẫu nhiên khác nhau (1 trong 3 trạng thái: DELIVERED, COMPROMIZED, ALERTED)
--> Này hơi khó tí vì liên quan đến tự động 

3. Sửa cái giao diện in ra cho nó cân đối và đẹp tí =)) 

4. Thông tin các lần mock của 1 sản phẩm chưa up được vào file excel nên chưa vẽ được biểu đồ 
--> Sửa để mỗi cần mock data của lô hàng đó sẽ được lưu vào file.xls để đưa sang excel rồi vẽ chart bên excel 