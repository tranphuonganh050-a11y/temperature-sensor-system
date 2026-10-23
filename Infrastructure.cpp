#ifndef INFRASTRUCTURE_H
#define INFRASTRUCTURE_H

#include "Domain.cpp"
#include <iostream>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <ctime>
#include <vector>
#include <string>

using namespace std;

// Định dạng các dòng trong file LICH_SU_KIEM_TRA.txt:
//   SHIPMENT|id|ten|nguoi gui|nguoi nhan|trang thai|container|min|max
//   VIOLATION|id|thoi gian|nhiet do|lat,lng
//   TELEMETRY|id|thoi gian|nhiet do|lat,lng|kich ban     <- [MỚI] để vẽ biểu đồ
class FileShipmentRepository : public IShipmentRepository {
private:
    string filePath;
    vector<Shipment> inMemoryStore;  // bộ đệm trong RAM để truy xuất nhanh

    // Tách "21.03,105.8" thành lat, lng
    static Location parseLocation(const string& locStr) {
        float lat = 0.0f, lng = 0.0f;
        size_t commaPos = locStr.find(',');
        if (commaPos != string::npos) {
            lat = stof(locStr.substr(0, commaPos));
            lng = stof(locStr.substr(commaPos + 1));
        }
        return Location(lat, lng);
    }

    // Nạp dữ liệu từ file khi chạy lại chương trình
    void loadFromFile() {
        ifstream inFile(filePath);
        if (!inFile.is_open()) return;

        inMemoryStore.clear();
        string line;
        while (getline(inFile, line)) {
            if (line.empty()) continue;
            try {
                stringstream ss(line);
                string type;
                getline(ss, type, '|');

                if (type == "SHIPMENT") {
                    string idStr, prod, sender, rcv, statusStr, containerIdStr, minStr, maxStr;
                    getline(ss, idStr, '|');
                    getline(ss, prod, '|');
                    getline(ss, sender, '|');
                    getline(ss, rcv, '|');
                    getline(ss, statusStr, '|');
                    getline(ss, containerIdStr, '|');
                    getline(ss, minStr, '|');
                    getline(ss, maxStr, '|');

                    int id = stoi(idStr);
                    int containerId = containerIdStr.empty() ? 0 : stoi(containerIdStr);
                    float minTemp = minStr.empty() ? 0.0f : stof(minStr);
                    float maxTemp = maxStr.empty() ? 0.0f : stof(maxStr);

                    ColdContainer container(containerId, "Standard-Model", TemperatureRange(minTemp, maxTemp));
                    Shipment shipment(id, prod, sender, rcv, container, statusFromString(statusStr));

                    // Nếu lô hàng đã có thì thay bằng bản mới (bản mới nằm sau trong file
                    // nên luôn là bản cập nhật nhất; vi phạm/telemetry sẽ được nạp lại ngay bên dưới)
                    Shipment* existing = findById(id);
                    if (existing) *existing = shipment;
                    else inMemoryStore.push_back(shipment);
                }
                else if (type == "VIOLATION") {
                    string idStr, timeStr, tempStr, locStr;
                    getline(ss, idStr, '|');
                    getline(ss, timeStr, '|');
                    getline(ss, tempStr, '|');
                    getline(ss, locStr, '|');
                    

                    Shipment* s = findById(stoi(idStr));
                    if (s) s->addViolation(ViolationLog(stof(tempStr), parseLocation(locStr), timeStr));
                }
                else if (type == "TELEMETRY") {
                    string idStr, timeStr, tempStr, locStr, scenario;
                    getline(ss, idStr, '|');
                    getline(ss, timeStr, '|');
                    getline(ss, tempStr, '|');
                    getline(ss, locStr, '|');
                    getline(ss, scenario, '|');

                    Shipment* s = findById(stoi(idStr));
                    if (s) s->addTelemetry(TelemetryLog(stof(tempStr), parseLocation(locStr), timeStr, scenario));
                }
            } catch (...) {
                continue;  // dòng lỗi -> bỏ qua
            }
        }
        inFile.close();
    }

    void writeAllToFile() {
        ofstream outFile(filePath);  // mặc định là ghi đè (xóa nội dung cũ)
        if (!outFile.is_open()) {
            cout << "LOI! Khong the mo file ghi du lieu: " << filePath << endl;
            return;
        }

        for (const auto& shipment : inMemoryStore) {
            outFile << "SHIPMENT|"
                    << shipment.getShipmentId() << "|"
                    << shipment.getProductName() << "|"
                    << shipment.getSender() << "|"
                    << shipment.getReceiver() << "|"
                    << statusToString(shipment.getStatus()) << "|"
                    << shipment.getContainer().getContainerId() << "|"
                    << shipment.getContainer().getTempRange().getMin() << "|"
                    << shipment.getContainer().getTempRange().getMax() << "\n";

            for (const auto& v : shipment.getViolations()) {
                outFile << "VIOLATION|" << shipment.getShipmentId() << "|"
                        << v.getTimestamp() << "|"
                        << fixed << setprecision(2) << v.getRecordedTemp() << "|"
                        << fixed << setprecision(4) << v.getLocation().getLatitude() << ","
                        << v.getLocation().getLongitude() << "\n";
            }

            for (const auto& t : shipment.getTelemetryHistory()) {
                outFile << "TELEMETRY|" << shipment.getShipmentId() << "|"
                        << t.getTimestamp() << "|"
                        << fixed << setprecision(2) << t.getTemperature() << "|"
                        << fixed << setprecision(4) << t.getLocation().getLatitude() << ","
                        << t.getLocation().getLongitude() << "|"
                        << t.getScenarioName() << "\n";
            }
        }
        outFile.close();
    }

public:
    FileShipmentRepository(const string& path = "LICH_SU_KIEM_TRA.txt") {
        filePath = path;
        loadFromFile();
    }

    void save(const Shipment& shipment) override {
        // Cập nhật bộ đệm: có rồi thì thay, chưa có thì thêm
        Shipment* existing = findById(shipment.getShipmentId());
        if (existing) {
            if (existing != &shipment) *existing = shipment;
        } else {
            inMemoryStore.push_back(shipment);
        }
        writeAllToFile();
    }

    Shipment* findById(const int& shipmentId) override {
        for (auto& shipment : inMemoryStore) {
            if (shipment.getShipmentId() == shipmentId) return &shipment;
        }
        return nullptr;
    }

    vector<Shipment> findAll() override {
        return inMemoryStore;
    }

    // In nguyên file lịch sử ra màn hình
    void displayHistoryFromFile() {
        ifstream inFile(filePath);
        if (!inFile.is_open()) {
            cout << "Chua co lich su kiem tra\n";
            return;
        }
        cout << "\n===== LICH SU KIEM TRA ===============" << endl;
        string line;
        while (getline(inFile, line)) cout << line << endl;
        cout << "=========================================\n" << endl;
    }
};

// Các kịch bản mô phỏng
enum class SimulationScenario {
    NORMAL,           // xe di chuyển bình thường
    COOLING_FAILURE,  // hỏng hệ thống làm lạnh (nhiệt độ tăng dần)
    DOOR_OPEN         // mở cửa xe đột ngột (nhiệt độ tăng vọt)
};

class MockSensor {
private:
    string SensorID;
    string containerID;
    float currentTemp;
    mutable mt19937 rng;  // máy tạo số ngẫu nhiên

    // Số thực ngẫu nhiên trong [lo, hi]
    float randomFloat(float lo, float hi) const {
        if (hi <= lo) return lo;  // tránh lỗi khi khoảng bằng 0
        uniform_real_distribution<float> dist(lo, hi);
        return dist(rng);
    }
    // Số nguyên ngẫu nhiên trong [lo, hi]
    int randomInt(int lo, int hi) const {
        uniform_int_distribution<int> dist(lo, hi);
        return dist(rng);
    }

public:
    MockSensor(string id = "Sensor_01", string cId = "Container_01", float initialTemp = 4.0f)
        : SensorID(id), containerID(cId), currentTemp(initialTemp), rng(random_device{}()) {}

    //chọn ngẫu nhiên số bước lỗi và vị trí bước lỗi tương ứng.
    vector<SimulationScenario> planRun(int totalSteps) const {
        vector<SimulationScenario> plan(totalSteps, SimulationScenario::NORMAL);

        int outcome = randomInt(0, 2);
        int faults = 0;                                              // số bước bị lỗi
        if (outcome == 1) faults = randomInt(1, min(3, totalSteps));          // 1-3 lần -> ALERTED
        else if (outcome == 2) faults = randomInt(min(4, totalSteps), totalSteps);  // 4+ lần -> COMPROMISED

        // Xáo trộn thứ tự các bước rồi lấy 'faults' bước đầu làm bước lỗi
        vector<int> idx(totalSteps);
        iota(idx.begin(), idx.end(), 0);
        shuffle(idx.begin(), idx.end(), rng);
        for (int i = 0; i < faults; ++i) {
            plan[idx[i]] = (randomInt(0, 1) == 0) ? SimulationScenario::COOLING_FAILURE
                                                  : SimulationScenario::DOOR_OPEN;
        }
        return plan;
    }

    // Sinh nhiệt độ theo kịch bản.
    float readCurrentTemp(SimulationScenario scenario, float minTemp, float maxTemp) {
        float mid = (minTemp + maxTemp) / 2.0f;
        float noiseLimit = min(0.5f, (maxTemp - minTemp) / 4.0f);  // nhiễu nhỏ hơn nếu khoảng an toàn hẹp

        switch (scenario) {
            case SimulationScenario::NORMAL:
                currentTemp = mid + randomFloat(-noiseLimit, noiseLimit);
                break;
            case SimulationScenario::COOLING_FAILURE:
                currentTemp += randomFloat(1.0f, 2.0f);                       // tăng dần
                if (currentTemp <= maxTemp) currentTemp = maxTemp + randomFloat(0.5f, 1.5f);
                break;
            case SimulationScenario::DOOR_OPEN:
                currentTemp = maxTemp + randomFloat(4.0f, 8.0f);              // tăng vọt
                break;
        }
        return currentTemp;
    }

    string scenarioToString(SimulationScenario scenario) const {
        switch (scenario) {
            case SimulationScenario::NORMAL: return "Xe di chuyen binh thuong !";
            case SimulationScenario::COOLING_FAILURE: return "Hong he thong lam lanh !";
            case SimulationScenario::DOOR_OPEN: return "Mo cua xe dot ngot !";
        }
        return "UNKNOWN";
    }

    float readHumidity() const { return randomFloat(45.0f, 85.0f); }

    Location readCurrentLocation() const {
        return Location(randomFloat(20.9f, 21.1f), randomFloat(105.7f, 105.9f));  // giả lập quanh Hà Nội
    }

    string getCurrentTime() const {
        time_t now = time(nullptr);
        tm* ltm = localtime(&now);
        char buf[32];
        strftime(buf, sizeof(buf), "%H:%M:%S %d/%m/%Y", ltm);
        return string(buf);
    }
    string getSensorID() const { return SensorID; }
    string getContainerID() const { return containerID; }
};
using IoTSensorSimulator = MockSensor;

class ConsoleLoggerObserver : public ITemperatureObserver {
public:
    virtual ~ConsoleLoggerObserver() {}
    void onTemperatureExceeded(const TemperatureExceededEvent& event) override {
        (void)event;
    }
};
#endif