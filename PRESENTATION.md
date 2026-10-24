# Presentation: hiển thị thông tin ra màn hình, nhận dữ liệu người dùng gõ vào, chuyển yêu cầu cho ShipmentService trong application, rồi trình bày kết quả nhận về 
# Nguyên tắc quan trọng của tầng này: không chứa nghiệp vụ. Nó không quyết định lô hàng bị hủy hay không, chỉ hỏi Service rồi in kết quả.
#ifndef PRESENTATION_H
#define PRESENTATION_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <limits>
#include "Application.cpp"
#include "Infrastructure.cpp"
#include "Extension.cpp"

using namespace std;
// Presentation không #include "Domain.cpp" trực tiếp, nhưng vẫn dùng được Location vì các tệp kia đã nạp nó.

# 1. class ShipmentViewFormatter: định dạng dữ liệu thành bảng chữ để in, không nhập liệu, không gọi Service, không biết người dùng chọn gì. Tách riêng việc in ra một lớp khác giúp ConsoleMenu gọn hơn, và nếu sau này muốn đổi cách in (ví dụ bảng khác, màu khác) chỉ sửa ở đây.
class ShipmentViewFormatter
{
public:
    void renderTable(const vector<ShipmentDTO>& danhSach)
    {
        cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
        cout << "|           DANH SACH LO HANG            |\n";
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
        
        

        for (const auto& item : danhSach)
        {
            cout << "| Ma lo hang: " << item.getShipmentId() << '\n';
            cout << "| Ten san pham: " << item.getProductName() << '\n';
            cout << "| Trang thai: \033[1m" << item.getStatus() << "\033[0m" << '\n';
            cout << "| Nhiet do cuoi cung: " << item.getLastRecordedTemp() << " do C" << '\n';
            cout << "| So lan vi pham: " << item.getViolationCount() << '\n';
            cout << "----------------------------------------\n";
        }

        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    }
};
# 2. class ConsoleMenu: hiển thị menu, đọc lựa chọn, rồi giao việc cho hàm tương ứng
class ConsoleMenu
{
private:
    ShipmentService& shipmentService;
    ShipmentViewFormatter view;
    int lastCreatedShipmentId; //lô hàng vừa tạo gần nhất 

public:
    explicit ConsoleMenu(ShipmentService& service) : shipmentService(service), lastCreatedShipmentId(-1) {}

* Hàm displayMainMenu(): vòng lặp chính của chương trình. Hiển thị menu và lặp lại cho đến khi người dùng chọn thoát.
    void displayMainMenu()
    {
        int Luachon = 0;
        do
        {
            cout << "\n========================================\n";
            cout << "|                 MENU                  |\n";
            cout << "========================================\n";
            cout << "|  (1) Tao lo hang moi                  |\n";
            cout << "|  (2) Chay mo phong cam bien           |\n";
            cout << "|  (3) Xem lich su lo hang              |\n";
            cout << "|  (4) Xuat du lieu                     |\n"; 
            cout << "|  (5) Thoat chuong trinh               |\n";
            cout << "========================================\n";
            cout << "--> Vui long chon: ";
            cin >> Luachon;

            if (cin.fail())
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Loi! Vui long chi nhap so !" << endl;
                continue;
            }

            switch (Luachon)
            {
                case 1:
                    handleCreateShipment();
                    break;
                case 2:
                    handleRunSensorSimulation();
                    break;
                case 3:
                {
                    vector<ShipmentDTO> data = shipmentService.getShipmentHistory();
                    view.renderTable(data);
                    break;
                }
                case 4: {
                   bool isSuccess = AuditReportExporter::exportToCSV(shipmentService.getShipmentHistory());
                    if (isSuccess) {
                        cout << "\nDa xuat du lieu: ColdChain_Compliance_Report.xls\n";
                    } else {
                        cout << "\nKhong the xuat file Excel!\n";
                    }
                    break;
                }
                case 5:
                    cout << "Ket thuc chuong trinh !" << endl;
                    break;
                default:
                    cout << "Lua chon khong co trong danh muc. Vui long chon lai!\n";
            }
        } while (Luachon != 5);
    }

* Hàm tạo lô hàng mới: Người dùng nhập thông tin lô hàng mới muốn tạo 
    void handleCreateShipment()
    {
        string ten, gui, nhan;
        int thietBiId = 0;
        float min = 0.0f, max = 0.0f;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Nhap ten: ";
        getline(cin, ten);
        cout << "Nguoi gui: ";
        getline(cin, gui);
        cout << "Nguoi nhan: ";
        getline(cin, nhan);
        cout << "Container ID: ";
        cin >> thietBiId;
        cout << "Nhiet do an toan min (do C): ";
        cin >> min;
        cout << "Nhiet do an toan max (do C): ";
        cin >> max;

// Sau khi tạo thông tin lô hàng mới được gói vào 1 DTO  
        CreateShipmentDTO donHangMoi(ten, gui, nhan, thietBiId, min, max);
        lastCreatedShipmentId = shipmentService.createShipment(donHangMoi);
        cout << "Tao lo hang thanh cong. Ma lo hang: " << lastCreatedShipmentId << '\n'; 
    }

* Hàm này chạy mô phỏng cảm biến: điều phối cảm biến giả (MockSensor) và Service để mô phỏng một chuyến vận chuyển 6 lần đo
    void handleRunSensorSimulation()
    {
    // Nếu mã hàng không phải số thì yêu cầu chọn lại 
        int maHang;
        cout << "Nhap ma lo hang muon mo phong: ";
        cin >> maHang; 
        if(cin.fail()){
            cin.clear(); 
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Loi! Ma l hang phai la so! "<< '\n';
            return; 
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
       
       // Tìm lô hàng trong lịch sử và kiểm tra điều kiện 
        auto history = shipmentService.getShipmentHistory();
        const ShipmentDTO* found = nullptr;
        for (const auto& item : history) {
            if (item.getShipmentId() == maHang) {
                found = &item;
                break;
            }
        }

        // Nếu không tìm thấy mã lô hàng khớp 
        if(!found){
            cout << "Khong tim thay lo hang ma " << maHang << ". Vui long tao lo hang moi hoac kiem tra lai ma!\n";
            return; 
        }

        // Nếu lô hàng đã giao thành công 
        const string currentStatus = found->getStatus(); 
        if(currentStatus == "DELIVERED"){
            cout << "Lo hang " << maHang << " da giao thanh cong -> Khong the mo phong lai. Vui long tao lo hang moi hoac chon lo khac!\n";
            return; 
        }
        // Nếu lô hàng bị hủy 
        if(currentStatus == "COMPROMISED"){
            cout << "Lo hang " << maHang << " da bi huy do vi pham qua 3 lan -> Khong the mo phong lai. Vui long tao lo hang moi hoac chon lo khac!\n";
            return; 
        }

        // Chuẩn bị để mô phỏng: Sao chép tên và ngưỡng ra biến riêng, tạo cảm biến giả, và nhờ planRun lập sẵn kế hoạch 6 bước (bước nào bình thường, bước nào lỗi).
        const string productName = found->getProductName();
        const float minSafe = found->getMinTemp();
        const float maxSafe = found->getMaxTemp(); 

        MockSensor sensor("Sensor-01");
        const int totalSteps = 6;  

        vector<SimulationScenario> plan = sensor.planRun(totalSteps);
        int violationsThisRun = 0;

        cout << "\n==================================================\n";
        cout << "|              MO PHONG CAM BIEN IoT              |\n";
        cout << "==================================================\n";

        for (int step = 1; step <= totalSteps; ++step)
        {
            SimulationScenario scenario =  plan[step - 1];  // lấy kịch bản của bước này từ plan 
             // sinh nhiệt, tọa độ, giờ 
            float temp = sensor.readCurrentTemp(scenario, minSafe, maxSafe); 
            string scenarioName = sensor.scenarioToString(scenario);
            Location loc = sensor.readCurrentLocation();
            string timestamp = sensor.getCurrentTime(); 
        
            try
            {   
                bool isViolated = (temp < minSafe || temp > maxSafe);

                // gửi dữ liệu cho Service xử lý 
                shipmentService.processSensorData(maHang, temp, loc, timestamp, scenarioName);

                if(isViolated) ++violationsThisRun; 

                cout << "| Mock: " << step << endl; 
                cout << "| Ma lo hang               : " << maHang << endl;
                cout << "| Ten hang                 : " << productName << endl; 
                cout << "| Event: " << scenarioName << endl;
                if(isViolated){
                    cout << "| \033[31mWARNING!\033[0m" << endl; 
                }
                cout << "| Nhiet do do duoc hien tai : " << fixed << setprecision(1) << temp << " do C" << endl;
                cout << "| Nhiet do cho phep        : " << fixed << setprecision(1) << minSafe << " - " << maxSafe << " do C" << endl; 
                cout << "| Toa do xe                : Lat " << loc.getLatitude() << ", Lon " << loc.getLongitude() << endl;
                cout << "| Thoi gian do             : " << timestamp << endl;
                cout << "--------------------------------------------------\n";
                
            }
            catch (const exception& e)
            {
                cout << "Exception: " << e.what() << endl;
                break;
            }
            // phát sinh mỗi 2 giây 
            this_thread::sleep_for(chrono::seconds(2));
        }

        cout << "--------------HOAN THANH MO PHONG-----------------\n";

        shipmentService.finalSimulation(maHang);  // nhờ Service chốt trạng thái cuối cùng theo số lần vi phạm 
        
        // Lấy lại lịch sử lô vừa chạy và tính tổng số lần vi phạm 
        auto updateHistory = shipmentService.getShipmentHistory(); 
        int violationCount = 0; 
        for(const auto& item: updateHistory){
            if(item.getShipmentId() == maHang){
                violationCount = item.getViolationCount();
                break; 
            }
        }
            cout << "Lo hang " << maHang << " vi pham nhiet do " << violationCount << " lan" << endl; 
        // Xuất báo cáo Excel
        if(AuditReportExporter::exportToExcel(shipmentService.getShipmentHistory())){
            cout << "Da xuat du lieu!" << endl; 
        }
        else cout << "Loi xuat file" << endl; 
        }
};

#endif 
# Nhận xét và hạn chế (đưa vào phần đánh giá)
4. Presentation phụ thuộc trực tiếp vào Infrastructure (MockSensor). Về mặt kiến trúc chặt chẽ, tầng giao diện chỉ nên gọi Application. Với đề tài mô phỏng thì chấp nhận được, vì MockSensor đóng vai thiết bị ngoài.
5. Chưa kiểm tra dữ liệu nhập khi tạo lô hàng: các lần cin >> cho số không bắt lỗi; tên rỗng vẫn được chấp nhận.
6. Thuộc tính lastCreatedShipmentId chưa được dùng.
7. Mô phỏng chặn chương trình: sleep_for 2 giây × 6 lần làm chương trình đứng chờ khoảng 12 giây, người dùng không thể làm gì khác trong lúc đó.
8. Số bước mô phỏng cố định: totalSteps = 6 ghi cứng trong hàm.
9. Bảng ở lựa chọn 3 chưa hiển thị ngưỡng và vị trí, dù DTO đã có dữ liệu.