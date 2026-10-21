#ifndef EXTENSION_STEP4_H
#define EXTENSION_STEP4_H

#include "Application.cpp"
#include <fstream>
#include <sstream>
#include <cmath>
using namespace std;

// Xuất báo cáo dạng excel/csv
class AuditReportExporter{
    public: 
        static bool exportToCSV(const vector<ShipmentDTO>& history, const string& filename = "ColdChain_Compliance_Report.csv"){
            ofstream outFile(filename);
            if(!outFile.is_open()) return false;
            auto csvEscape = [](const string& value) {
                string escaped;
                for (char ch : value) {
                    if (ch == '\"') escaped += '\"';
                    escaped += ch;
                }
                return string("\"") + escaped + "\"";
            };
            outFile << "Ma lo hang,Ten san pham,Trang thai,Nhiet do cuoi,So lan vi pham,Vi tri\n";
            for(const auto& item : history){
                outFile << item.getShipmentId() << ","
                        << csvEscape(item.getProductName()) << ","
                        << item.getStatus() << ","
                        << item.getLastRecordedTemp() << ","
                        << item.getViolationCount() << ","
                        << csvEscape(to_string(item.getLastLocation().getLatitude()) + ", " + to_string(item.getLastLocation().getLongitude())) << "\n";
            }
            return outFile.good(); 
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
        void displayAlertFeed() const{
            cout << "\n==========================================================================" << endl;
        cout << "|                  DANH SACH CANH BAO (ALERT FEED)                        |" << endl;
        cout << "==========================================================================" << endl;

        if (alertLogs.empty()) {
            cout << "Khong co canh bao nao duoc ghi nhan.\n";
            return;
        }

        for (size_t i = 0; i < alertLogs.size(); ++i) {
            const auto& event = alertLogs[i];
            // Render màu đỏ cho cảnh báo (\033[31m)
            cout << "\033[31m"
                 << "[" << i + 1 << "] [SU CO: " << event.getScenarioName() << "] "
                 << "Moc thoi gian: " << event.getTimestamp() << " | "
                 << "Lo hang: " << event.getShipmentId() << " | "
                 << "Nhiet do vi pham: " << event.getExceededTemp() << " C (Nguong: " 
                 << event.getSafeMin() << " - " << event.getSafeMax() << " C) | "
                 << "Toa do: (" << event.getLocation().getLatitude() << ", " << event.getLocation().getLongitude() << ")"
                 << "\033[0m" << endl;
        }
        cout << "--------------------------------------------------------------------------\n" << endl;
        }
};
// View hien thi co mau sac
class DashboardView{
    public: 
        void renderColorMap(const vector<ShipmentDTO>& list){
            cout << "\n==================================================" << endl;
        cout << "|      BAN DO DIEU PHOI TRANG THAI (COLOR MAP)   |" << endl;
        cout << "==================================================" << endl;

        for (const auto& item : list) {
            bool isAlert = (item.getStatus() == "ALERTED" || item.getViolationCount() > 0);

            // \033[31m: Đỏ | \033[32m: Xanh lá | \033[0m: Reset màu
            string colorCode = isAlert ? "\033[31m" : "\033[32m";
            string statusIcon = isAlert ? "[CANH BAO / MAT AN TOAN]" : "[AN TOAN / BINH THUONG]";

            cout << colorCode
                 << "| Xe/Container ID: " << item.getShipmentId() << " " << statusIcon << "\n"
                 << "|   + San pham: " << item.getProductName() << "\n"
                 << "|   + Toa do hien tai: (" << item.getLastLocation().getLatitude() << ", " << item.getLastLocation().getLongitude() << ")\n"
                 << "|   + Nhiet do: " << item.getLastRecordedTemp() << " C\n"
                 << "\033[0m";
            cout << "--------------------------------------------------" << endl;
        }
    }
        // Biểu đồ chuỗi thời gian đơn giản dạng Console Text Chart
    void renderTimeSeriesChart(float currentTemp, float minSafe, float maxSafe){
        cout << "\n--- BIEU DO CHUOI THOI GIAN (TIME-SERIES CHART) ---" << endl;
        cout << "Upper Limit: " << maxSafe << " C | Lower Limit: " << minSafe << " C" << endl;
        cout << "Nhiet do hien tai: " << currentTemp << " C  -> Graph: [";
        
        int barLength = 0;
        if (maxSafe > minSafe) {
            const float normalized = (currentTemp - minSafe) / (maxSafe - minSafe);
            barLength = static_cast<int>(normalized * 40.0f + 0.5f);
            if (barLength < 0) barLength = 0;
            if (barLength > 40) barLength = 40;
        }

        bool isViolation = (currentTemp < minSafe || currentTemp > maxSafe);
        string color = isViolation ? "\033[31m" : "\033[32m";

        cout << color;
        for (int i = 0; i < barLength; ++i) cout << "|";
        cout << "\033[0m";
        
        for (int i = barLength; i < 40; ++i) cout << " ";
        cout << "]\n";
    }

};
#endif 