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
                    try {
                        handleCreateShipment();
                    } catch (const exception& e) {
                        cout << "Khong the tao lo hang: " << e.what() << endl;
                    }
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
                        cout << "\n>> [THANH CONG] Da xuat bao cao: ColdChain_Compliance_Report.csv\n";
                    } else {
                        cout << "\n>> [LOI] Khong the xuat file CSV!\n";
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
        if (!(cin >> thietBiId)) {
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            throw invalid_argument("Container ID phai la so nguyen.");
        }
        cout << "Nhiet do an toan min (do C): ";
        if (!(cin >> min)) {
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            throw invalid_argument("Nhiet do toi thieu khong hop le.");
        }
        cout << "Nhiet do an toan max (do C): ";
        if (!(cin >> max)) {
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            throw invalid_argument("Nhiet do toi da khong hop le.");
        }

        CreateShipmentDTO donHangMoi(ten, gui, nhan, thietBiId, min, max);
        lastCreatedShipmentId = shipmentService.createShipment(donHangMoi);
        cout << "Tao lo hang thanh cong. Ma lo hang: " << lastCreatedShipmentId << endl; 
    }

    // Mo phong cam bien cho lo hang do nguoi dung chon.
    void handleRunSensorSimulation()
    {
        int maHang = -1;
        cout << "Nhap ma lo hang muon mo phong";
        if (lastCreatedShipmentId != -1) cout << " (lo vua tao: " << lastCreatedShipmentId << ")";
        cout << ": ";
        if (!(cin >> maHang)) {
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Ma lo hang khong hop le.\n";
            return;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        float minSafe = 0.0f, maxSafe = 0.0f;
        if (!shipmentService.getTemperatureRange(maHang, minSafe, maxSafe)) {
            cout << "Khong tim thay lo hang " << maHang << ". Hay tao lo hang truoc.\n";
            return;
        }

        MockSensor sensor("Sensor-01", "Container-" + to_string(maHang), (minSafe + maxSafe) / 2.0f);
        DashboardView dashboardView;
        const int totalSteps = 6;

        cout << "\n==================================================" << endl;
        cout << "|              MO PHONG CAM BIEN IoT              |" << endl;
        cout << "| Nguong an toan: " << minSafe << " den " << maxSafe << " do C" << endl;
        cout << "==================================================" << endl;

        for (int step = 1; step <= totalSteps; ++step) {
            SimulationScenario scenario;
            float temp = sensor.readCurrentTemp(step, scenario, minSafe, maxSafe);
            string scenarioName = sensor.scenarioToString(scenario);
            Location loc = sensor.readCurrentLocation();
            string timestamp = sensor.getCurrentTime();
            bool isSafe = (temp >= minSafe && temp <= maxSafe);
            string color = isSafe ? "\033[32m" : "\033[31m";

            try {
                shipmentService.processSensorData(maHang, temp, loc, timestamp, scenarioName);
                cout << color;
                cout << "| Buoc mo phong: " << step << endl;
                cout << "| Su kien: " << scenarioName << endl;
                cout << "| Nhiet do do duoc: " << fixed << setprecision(1) << temp << " do C" << endl;
                cout << "| Ma lo hang: " << maHang << endl;
                cout << "| Toa do xe: Lat " << loc.getLatitude() << ", Lon " << loc.getLongitude() << endl;
                cout << "| Thoi gian do: " << timestamp << endl;
                cout << "| Ket qua: " << (isSafe ? "BINH THUONG" : "CANH BAO") << endl;
                cout << "\033[0m";
                dashboardView.renderTimeSeriesChart(temp, minSafe, maxSafe);
            } catch (const exception& e) {
                cout << "LOI: " << e.what() << endl;
                break;
            }
            this_thread::sleep_for(chrono::seconds(2));
        }
        cout << "HOAN THANH MO PHONG !!!!!" << endl;
    }
};

#endif 
