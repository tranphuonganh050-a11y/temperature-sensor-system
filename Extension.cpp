#ifndef EXTENSION_STEP4_H
#define EXTENSION_STEP4_H

#include "Application.cpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip> 
using namespace std;

// Xuất báo cáo excel định dạng html 
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
            static bool exportToExcel(const vector<ShipmentDTO>& history, const vector<TemperatureExceededEvent>& alertLogs ={}, const string& filename = "ColdChain_Compliance_Report.xls"){
        ofstream outFile(filename); 
        if(!outFile.is_open()) return false;

        outFile << "<html xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:x=\"urn:schemas-microsoft-com:office:excel\" xmlns=\"http://www.w3.org/TR/REC-html40\">\n";
        outFile << "<head>\n";
        outFile << "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n";
        outFile << "<!--[if gte mso 9]><xml><x:ExcelWorkbook><x:ExcelWorksheets><x:ExcelWorksheet>\n";
        outFile << "<x:Name>Bao Cao Lich Su</x:Name>\n";
        outFile << "<x:WorksheetOptions><x:DisplayGridlines/></x:WorksheetOptions>\n";
        outFile << "</x:ExcelWorksheet></x:ExcelWorksheets></x:ExcelWorkbook></xml><![endif]-->\n";
        outFile << "<style>\n";
        outFile << "  body { font-family: 'Segoe UI', Tahoma, sans-serif; }\n";
        outFile << "  table { border-collapse: collapse; width: 100%; margin-bottom: 30px; }\n";
        outFile << "  th, td { border: 1px solid #c6b0b0; padding: 8px 12px; text-align: center; }\n";
        outFile << "  th { background-color: #1F4E78; color: white; font-weight: bold; }\n";
        outFile << "  .sub-header { background-color: #2F5597; color: white; font-weight: bold; }\n";
        outFile << "  .alert { background-color: #FFC7CE; color: #9C0006; font-weight: bold; }\n";
        outFile << "</style>\n";
        outFile << "</head>\n";
        outFile << "<body>\n";

       
        outFile << "<table>\n";
        outFile << "  <tr>\n";
        outFile << "    <th>Ma lo hang</th>\n";
        outFile << "    <th>Ten san pham</th>\n";
        outFile << "    <th>Trang thai</th>\n";
        outFile << "    <th>Nhiet do cuoi (C)</th>\n";
        outFile << "    <th>So lan vi pham</th>\n";
        outFile << "    <th>Vi tri (Lat, Lon)</th>\n";
        outFile << "  </tr>\n";

        for(const auto& item : history){
            bool hasViolation = item.getViolationCount() > 0;
            if (hasViolation) {
                outFile << "  <tr class=\"alert\">\n";
            } else {
                outFile << "  <tr>\n";
            }
            
            outFile << "    <td>" << item.getShipmentId() << "</td>\n";
            outFile << "    <td>" << item.getProductName() << "</td>\n";
            outFile << "    <td>" << item.getStatus() << "</td>\n";
            outFile << "    <td>" << fixed << setprecision(1) << item.getLastRecordedTemp() << "</td>\n";
            outFile << "    <td>" << item.getViolationCount() << "</td>\n";
            outFile << "    <td>" << fixed << setprecision(4) 
                    << item.getLastLocation().getLatitude() << ", " 
                    << item.getLastLocation().getLongitude() << "</td>\n";
            outFile << "  </tr>\n";
        }
        outFile << "</table>\n";
        outFile << "<h3>BANG DU LIEU BIEN DONG NHIET DO </h3>\n";
        outFile << "<table>\n";
        outFile << "  <tr class=\"sub-header\">\n";
        outFile << "    <th>Ma lo hang</th>\n";
        outFile << "    <th>Ten san pham</th>\n";
        outFile << "    <th>Lan do (Step)</th>\n";
        outFile << "    <th>Thoi gian</th>\n";
        outFile << "    <th>Nhiet do do (C)</th>\n";
        outFile << "    <th>Nguong Min (C)</th>\n";
        outFile << "    <th>Nguong Max (C)</th>\n";
        outFile << "    <th>Kich ban (Scenario)</th>\n";
        outFile << "  </tr>\n";

        for(const auto& item : history){
            int step = 1;
            for(const auto& tele : item.getTelemetryHistory()){
                bool isViolated = (tele.getTemperature() < item.getMinTemp() || tele.getTemperature() > item.getMaxTemp());
                if (isViolated) {
                    outFile << "  <tr class=\"alert\">\n";
                } else {
                    outFile << "  <tr>\n";
                }

                outFile << "    <td>" << item.getShipmentId() << "</td>\n";
                outFile << "    <td>" << item.getProductName() << "</td>\n";
                outFile << "    <td>" << step++ << "</td>\n";
                outFile << "    <td>" << tele.getTimestamp() << "</td>\n";
                outFile << "    <td>" << fixed << setprecision(1) << tele.getTemperature() << "</td>\n";
                outFile << "    <td>" << fixed << setprecision(1) << item.getMinTemp() << "</td>\n";
                outFile << "    <td>" << fixed << setprecision(1) << item.getMaxTemp() << "</td>\n";
                outFile << "    <td>" << tele.getScenarioName() << "</td>\n";
                outFile << "  </tr>\n";
            }
        }
        outFile << "</table>\n";

        outFile << "</body>\n";
        outFile << "</html>\n";

        outFile.close();
        return true; 
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
        void displayAlertFeed(const vector<ShipmentDTO>& shipmentHistory = {}) const {

        const int TABLE_WIDTH = 105;

        cout << "\n=========================================================================================" << endl;
        cout << "|                             DANH SACH CANH BAO (ALERT FEED)                            |" << endl;
        cout << "=========================================================================================" << endl;

        if (alertLogs.empty()) {
            cout << "| Khong co canh bao nao duoc ghi nhan.                                                   |" << endl;
            cout << "=========================================================================================" << endl;
            return;
        }

        // In header của bảng
        cout << left 
            << "| " << setw(5)  << "STT"
            << "| " << setw(10) << "Ma hang" 
            << "| " << setw(18) << "Ten san pham" 
            << "| " << setw(22) << "Thoi gian" 
            << "| " << setw(16) << "Toa do" 
            << "| " << setw(10) << "Nhiet do" 
            << "| " << setw(25) << "Canh bao" << " |" << endl;

        string line(TABLE_WIDTH, '-');
        cout << line << endl;

        for (size_t i = 0; i < alertLogs.size(); ++i) {
            const auto& event = alertLogs[i];
            
            // Tìm tên sản phẩm dựa vào mã lô hàng
            string productName = "Unknown";
            for (const auto& item : shipmentHistory) {
                if (item.getShipmentId() == event.getShipmentId()) {
                    productName = item.getProductName();
                    break;
                }
            }

            // Định dạng tọa độ ngắn gọn và đẹp mắt
            stringstream coordSs;
            coordSs << fixed << setprecision(2) << "(" << event.getLocation().getLatitude() 
                    << ", " << event.getLocation().getLongitude() << ")";
            string coordStr = coordSs.str();

            // Định dạng nhiệt độ
            stringstream tempSs;
            tempSs << fixed << setprecision(2) << event.getExceededTemp() << "C";
            string tempStr = tempSs.str();

            // Đổ dữ liệu vào chuỗi trước để tính độ dài chính xác
            stringstream rowSs;
            rowSs << left 
                << "| " << setw(5)  << (i + 1)
                << "| " << setw(10) << event.getShipmentId()
                << "| " << setw(18) << productName
                << "| " << setw(22) << event.getTimestamp()
                << "| " << setw(16) << coordStr
                << "| " << setw(10) << tempStr
                << "| " << setw(25) << event.getScenarioName() << " |";

            // Bọc màu đỏ ANSI quanh toàn bộ dòng đã được căn chỉnh chuẩn
            cout << "\033[31m" << rowSs.str() << "\033[0m" << endl;
        }

        cout << string(TABLE_WIDTH, '=') << "\n" << endl;
    }
};
#endif 