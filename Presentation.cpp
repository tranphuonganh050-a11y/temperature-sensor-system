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
                        cout << "\nDa xuat du lieu ra file Excel!\n";
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

        CreateShipmentDTO donHangMoi(ten, gui, nhan, thietBiId, min, max);
        lastCreatedShipmentId = shipmentService.createShipment(donHangMoi);
        cout << "Tao lo hang thanh cong. Ma lo hang: " << lastCreatedShipmentId << '\n'; 
    }
    // Mocking để tự động chạy nhiệt độ thay vì nhập vào
    void handleRunSensorSimulation()
    {
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
        
        auto history = shipmentService.getShipmentHistory();
        const ShipmentDTO* found = nullptr;
        for (const auto& item : history) {
            if (item.getShipmentId() == maHang) {
                found = &item;
                break;
            }
        }
        if(!found){
            cout << "Khong tim thay lo hang ma " << maHang << ". Vui long tao lo hang moi hoac kiem tra lai ma!\n";
            return; 
        }
        const string currentStatus = found->getStatus(); 
        if(currentStatus == "DELIVERED"){
            cout << "Lo hang " << maHang << " da giao thanh cong -> Khong the mo phong lai. Vui long tao lo hang moi hoac chon lo khac!\n";
            return; 
        }
        if(currentStatus == "COMPROMISED"){
            cout << "Lo hang " << maHang << " da bi huy do vi pham qua 3 lan -> Khong the mo phong lai. Vui long tao lo hang moi hoac chon lo khac!\n";
            return; 
        }
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
            SimulationScenario scenario =  plan[step - 1];
            float temp = sensor.readCurrentTemp(scenario, minSafe, maxSafe);
            string scenarioName = sensor.scenarioToString(scenario);
            Location loc = sensor.readCurrentLocation();
            string timestamp = sensor.getCurrentTime(); 
        
            try
            {   
                bool isViolated = shipmentService.processSensorData(maHang, temp, loc, timestamp, scenarioName);
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

        shipmentService.finalSimulation(maHang); 
        
        auto updateHistory = shipmentService.getShipmentHistory(); 
        int violationCount = 0; 
        for(const auto& item: updateHistory){
            if(item.getShipmentId() == maHang){
                violationCount = item.getViolationCount();
                break; 
            }
        }
        cout << "Luot mo phong nay co " << violationsThisRun << " lan vi pham. " << endl; 
        cout << "Lo hang " << maHang << " vi pham nhiet do tong cong " << violationCount << " lan" << endl; 
        if(AuditReportExporter::exportToExcel(shipmentService.getShipmentHistory())){
            cout << "Da xuat du lieu!" << endl; 
        }
        else cout << "Loi xuat file" << endl; 
        }
};

#endif 
