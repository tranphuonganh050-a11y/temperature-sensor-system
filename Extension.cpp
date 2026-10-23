#ifndef EXTENSION_STEP4_H
#define EXTENSION_STEP4_H

#include "Application.cpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip> 
using namespace std;

// Xuất báo cáo dạng excel/csv
class AuditReportExporter{
    public: 
    // Xuất file csv gốc làm bộ đệm 
        static bool exportToCSV(const vector<ShipmentDTO>& history, const string& filename = "ColdChain_Compliance_Report.csv"){
            ofstream outFile(filename);
            if(!outFile.is_open()) return false; 
            outFile << "Ma lo hang, Ten san pham, Trang thai, Nhiet do cuoi, So lan vi pham, Vi tri\n";
            for(const auto& item : history){
                    outFile << item.getShipmentId() << ","
                            << "\"" << item.getProductName() << "\","
                    << item.getStatus() << ","
                    << item.getLastRecordedTemp() << ","
                    << item.getViolationCount() << ","
                    << "\"" << item.getLastLocation().getLatitude() << ", " << item.getLastLocation().getLongitude() << "\"\n";
                }
            outFile.close();
            return true; 
        }
        //Xuất dạng excel 
        static bool exportToExcel(const vector<ShipmentDTO>& history, const vector<TemperatureExceededEvent>& alertLogs ={}){
            // ghi data vi pham ra file tam 
            ofstream vFile("temp_violations.txt");
            if(vFile.is_open()){
                for(const auto& ev : alertLogs){
                    vFile << ev.getShipmentId() << "|"
                        << ev.getTimestamp() << "|"
                        << ev.getExceededTemp() << "|"
                        << "Lat " << ev.getLocation().getLatitude() << ", Lon " << ev.getLocation().getLongitude() << "\n";
                }
                vFile.close(); 
            }
            // ghi data lo hang ra file tam 
            ofstream sFile("temp_shipments.txt");
            if(sFile.is_open()){
                for(const auto& item : history){
                    sFile << item.getShipmentId() << "|"
                          << item.getProductName() << "|"
                          << item.getStatus() << "|"
                          << item.getViolationCount() << "|"
                          << "Lat " << item.getLastLocation().getLatitude() << ", Lon " << item.getLastLocation().getLongitude() << "\n";
                }
                sFile.close();
            }
            // chuyen doi thanh Excel 
            
        }
};

// Danh sach canh bao mat an toan
class AlertFeedLogger:public ITemperatureObserver{
    private: 
        vector<TemperatureExceededEvent> alertLogs;
    public: 
        void onTemperatureExceeded(const TemperatureExceededEvent& event) override {
            alertLogs.push_back(event);
        }
        void displayAlertFeed(const vector<ShipmentDTO>& shipmentHistory = {}) const{
        cout << "\n==========================================================================" << endl;
        cout << "|                  DANH SACH CANH BAO (ALERT FEED)                        |" << endl;
        cout << "==========================================================================" << endl;

        if (alertLogs.empty()) {
            cout << "Khong co canh bao nao duoc ghi nhan.\n";
            cout << "-----------------------------------------------------------------------\n" << endl;
            return;
        }
        cout << left 
             << "| " << setw(6) << "STT"
             << "| " << setw(10) << "Ma mang" 
             << "| " << setw(18) << "Ten san pham" 
             << "| " << setw(18) << "Thoi gian" 
             << "| " << setw(14) << "Toa do" 
             << "| " << setw(10) << "Nhiet do" 
             << "| " << setw(22) << "Canh bao" << " |" << endl;
        cout << "-------------------------------------------------------------------------------" << endl;

        for (size_t i = 0; i < alertLogs.size(); ++i) {
            const auto& event = alertLogs[i];
            
            // Tìm tên sản phẩm dựa vào mã lô hàng từ lịch sử
            string productName = "Unknown";
            for (const auto& item : shipmentHistory) {
                if (item.getShipmentId() == event.getShipmentId()) {
                    productName = item.getProductName();
                    break;
                }
            }

            string coordStr = "(" + to_string(event.getLocation().getLatitude()).substr(0, 4) + ", " + 
                              to_string(event.getLocation().getLongitude()).substr(0, 4) + ")";

            // Render màu đỏ cho dòng cảnh báo
            cout << "\033[31m" << left 
                 << "| " << setw(6) << (to_string(i + 1))
                 << "| " << setw(10) << event.getShipmentId()
                 << "| " << setw(18) << productName
                 << "| " << setw(18) << event.getTimestamp()
                 << "| " << setw(14) << coordStr
                 << "| " << setw(8) << (to_string(event.getExceededTemp()).substr(0, 4) + "C")
                 << "| " << setw(22) << event.getScenarioName() << " |"
                 << "\033[0m" << endl;
        }
        cout << "--------------------------------------------------------------------------\n" << endl;
        }
};
#endif 