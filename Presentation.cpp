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

class ShipmentViewFormatter
{
public:
    void renderTable(const vector<ShipmentDTO>& danhSach)
    {
        cout << "\n========================================" << endl;
        cout << "|           DANH SACH LO HANG         |" << endl;
        cout << "========================================" << endl;

        for (const auto& item : danhSach)
        {
            cout << "| Ma lo hang: " << item.getShipmentId() << endl;
            cout << "| Ten san pham: " << item.getProductName() << endl;
            cout << "| Toa do xe van chuyen: (" << item.getLastLocation().getLatitude() << ", " << item.getLastLocation().getLongitude() << ")" << endl;
            cout << "| Trang thai: " << item.getStatus() << endl;
            cout << "| Nhiet do cuoi cung: " << item.getLastRecordedTemp() << " do C" << endl;
            cout << "| So lan vi pham: " << item.getViolationCount() << endl;
            cout << "----------------------------------------" << endl;
        }

        cout << "========================================" << endl;
    }
};

class ConsoleMenu
{
private:
    ShipmentService& shipmentService;
    ShipmentViewFormatter view;
    int lastCreatedShipmentId; //lô hàng vừa tạo gần nhất 

public:
    explicit ConsoleMenu(ShipmentService& service) : shipmentService(service), lastCreatedShipmentId(-1) {}

    void displayMainMenu()
    {
        int Luachon = 0;
        do
        {
            cout << "\n========================================" << endl;
            cout << "|                 MENU                  |" << endl;
            cout << "========================================" << endl;
            cout << "|  (1) Tao lo hang                      |" << endl;
            cout << "|  (2) Chay mo phong cam bien (Chart)   |" << endl;
            cout << "|  (3) Xem lich su lo hang              |" << endl;
            cout << "|  (4) Xuat bao cao Audit (File CSV)    |" << endl; // Thêm chức năng xuất file CSV
            cout << "|  (5) Thoat                            |" << endl;
            cout << "========================================" << endl;
            cout << "--> Vui long chon: ";
            cin >> Luachon;

            if (cin.fail())
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Loi! Vui long chi nhap SO!" << endl;
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
                        cout << "\nDa xuat bao cao: ColdChain_Compliance_Report.csv\n";
                    } else {
                        cout << "\nKhong the xuat file CSV!\n";
                    }
                    break;
                }
                case 5:
                    cout << "Chao tam biet!" << endl;
                    break;
                default:
                    cout << "San pham khong co trong danh muc. Vui long chon lai!" << endl;
            }
        } while (Luachon != 5);
    }

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
        cout << "Thiet bi (Container ID): ";
        cin >> thietBiId;
        cout << "Nhiet do an toan min (do C): ";
        cin >> min;
        cout << "Nhiet do an toan max (do C): ";
        cin >> max;

        CreateShipmentDTO donHangMoi(ten, gui, nhan, thietBiId, min, max);
        lastCreatedShipmentId = shipmentService.createShipment(donHangMoi);
        cout << "Tao lo hang thanh cong. Ma lo hang: " << lastCreatedShipmentId << endl; 
    }

    // Mocking để tự động chạy nhiệt độ thay vì nhập vào
    void handleRunSensorSimulation()
    {
        int maHang;
        cout << "Nhap ma lo hang muon mo phong: ";
        cin >> maHang; 
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
       
        auto history = shipmentService.getShipmentHistory();
        bool exists = false; 
        float minSafe = 0.0f, maxSafe = 0.0f; 

        for(const auto& item : history){
            if(item.getShipmentId() == maHang){
                exists = true;
                minSafe = item.getMinTemp(); 
                maxSafe = item.getMaxTemp(); 
                break; 
            }
        }
        if (!exists){
            cout << "Khong tim thay lo hang ma: " << maHang << ". Vui long tao lo hang hoac kiem tra lai ma: \n ";
            return; //Dừng chạy 
        }
        MockSensor sensor("Sensor-01");
        int totalSteps = 6;

        cout << "\n==================================================" << endl;
        cout << "|              MO PHONG CAM BIEN IoT              |" << endl;
        cout << "==================================================" << endl;

        for (int step = 1; step <= totalSteps; ++step)
        {
            SimulationScenario scenario;
            float temp = sensor.readCurrentTemp(step, scenario, minSafe, maxSafe);
            string scenarioName = sensor.scenarioToString(scenario);
            Location loc = sensor.readCurrentLocation();
            string timestamp = sensor.getCurrentTime(); 
        
            try
            {   
                bool isViolated = (temp < minSafe || temp > maxSafe || scenario != SimulationScenario::NORMAL);
                // Gọi Application xử lý dữ liệu và bắn cảnh báo vi phạm
                if(isViolated){
                    shipmentService.processSensorData(maHang, temp, loc, timestamp, scenarioName);
                }
                cout << "| Mock: " << step << endl; 
                cout << "| Ma lo hang               : " << maHang << endl;
                cout << "| Event: " << scenarioName << endl;
                if(isViolated){
                    cout << "\033[31m!WARNING!\033[0m" << endl; 
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

        cout << "HOAN THANH MO PHONG" << endl;

        auto updateHistory = shipmentService.getShipmentHistory(); 
        int violationCount = 0; 
        for(const auto& item: updateHistory){
            if(item.getShipmentId() == maHang){
                violationCount = item.getViolationCount();
                break; 
            }
        }
            cout << "Lo hang " << maHang << " vi pham nhiet do " << violationCount << " lan" << endl; 
        if(AuditReportExporter::exportToCSV(shipmentService.getShipmentHistory())){
            cout << "Xuat file thanh cong!" << endl; 
        }
        else cout << "Loi xuat file" << endl; 
        }
};

#endif 
