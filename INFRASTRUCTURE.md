# Infrastructure: đảm nhiệm việc ghi và đọc tệp, tạo dữ liệu cảm biến, đọc giờ hệ thống. Đây là nơi chứa các chi tiết kỹ thuật mà tầng nghiệp vụ không muốn biết 
# Tầng này chỉ phụ thuộc vào Domain. Nó cài đặt các interface mà Domain đã định nghĩa, đúng với nguyên tắc Dependency Inversion: tầng kỹ thuật phải đáp ứng yêu cầu của tầng nghiệp vụ, không phải ngược lại. Gồm 4 thành phần: FileShipmentRepository, SimulationScenario (enum), MockSensor (bí danh IoTSensorSimulator) và ConsoleLoggerObserver.
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
# 1. Class FileShipmentRepository: cài đặt cụ thể của “kho lưu trữ” bằng cách ghi dữ liệu vào tệp văn bản LICH_SU_KIEM_TRA.txt. Đây là ví dụ thực tế của kế thừa interface và đa hình đã học ở phần Domain.
* Cách hoạt động tổng thể: khi khởi động, nạp toàn bộ tệp vào RAM. Trong lúc chạy, mọi thao tác đọc/sửa làm việc trên RAM. Mỗi lần save, ghi toàn bộ RAM xuống tệp. Giống như bạn chép vở nháp (RAM) ra vở sạch (tệp) mỗi khi có thay đổi.
class FileShipmentRepository : public IShipmentRepository {
private:
    string filePath;
    vector<Shipment> inMemoryStore;  // bộ đệm trong RAM để truy xuất nhanh

* Hàm này tách chuỗi "21.03,105.8" thành hai số, rồi tạo Location. Tìm dấu phẩy bằng find(','), lấy phần trước làm vĩ độ, phần sau làm kinh độ, đổi chuỗi sang số bằng stof. Hàm static vì không cần dữ liệu của đối tượng nào.
    static Location parseLocation(const string& locStr) {
        float lat = 0.0f, lng = 0.0f;
        size_t commaPos = locStr.find(',');
        if (commaPos != string::npos) {
            lat = stof(locStr.substr(0, commaPos));
            lng = stof(locStr.substr(commaPos + 1));
        }
        return Location(lat, lng);
    }

* Hàm này nạp dữ liệu khi khởi động: Khi mở tệp, nếu không mở được (lần chạy đầu) thì thoát, kho rỗng. Sau đó Xóa bộ đệm, đọc từng dòng bằng getline, Dùng stringstream để tách chuỗi và dựa vào nhãn đầu dòng để biết dựng đối tượng nào 
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

            // Nếu là SHIPMENT: dựng ColdContainer, rồi Shipment (trạng thái đổi từ chuỗi bằng statusFromString). Nếu lô hàng đã có (findById) thì thay bằng bản mới, chưa có thì thêm.
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

                // VIOLATION: tìm lô hàng theo mã, gọi addViolation(...).
                else if (type == "VIOLATION") {
                    string idStr, timeStr, tempStr, locStr;
                    getline(ss, idStr, '|');
                    getline(ss, timeStr, '|');
                    getline(ss, tempStr, '|');
                    getline(ss, locStr, '|');
                    

                    Shipment* s = findById(stoi(idStr));
                    if (s) s->addViolation(ViolationLog(stof(tempStr), parseLocation(locStr), timeStr));
                }

                // TELEMETRY: tìm lô hàng theo mã, gọi addTelemetry(...).
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
* Hai hàm addViolation và addTelemetry của Shipment (Domain) tồn tại chính để phục vụ việc dựng lại lịch sử này mà không phải chạy lại logic kiểm tra nhiệt độ.
        }
        inFile.close();
* Dùng try catch để tránh sập ct 
    }
* * Tổng kết: Tệp lưu trữ sử dụng định dạng văn bản phân tách bằng ký tự |, mỗi dòng mang một nhãn loại. Dòng SHIPMENT lưu thông tin cố định của lô hàng; dòng TELEMETRY lưu từng điểm dữ liệu cảm biến phục vụ vẽ biểu đồ và báo cáo; dòng VIOLATION lưu riêng các lần vượt ngưỡng phục vụ đếm vi phạm và xác định trạng thái cuối. Ba loại dòng liên kết với nhau qua mã lô hàng. Cách tổ chức này cho phép lưu các danh sách có độ dài thay đổi của đối tượng Shipment và dựng lại đầy đủ đối tượng khi khởi động lại chương trình.

* Hàm writeAllToFile (private): ghi tất cả dữ liệu xuống tệp: lấy toàn bộ lô hàng đang nằm trong RAM (inMemoryStore) rồi ghi đè lên tệp LICH_SU_KIEM_TRA.txt, để dữ liệu không bị mất khi tắt chương trình. 
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

# phương thức 
public:
* Vừa tạo đối tượng là tự nạp dữ liệu cũ lên. Nhờ đó chạy lại chương trình vẫn còn lịch sử.
    FileShipmentRepository(const string& path = "LICH_SU_KIEM_TRA.txt") {
        filePath = path;
        loadFromFile();
    }
* Hàm save: Tìm trong RAM: có rồi thì thay, chưa có thì thêm; sau đó gọi writeAllToFile()
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

* Hàm này Duyệt RAM, trả con trỏ tới lô hàng
    Shipment* findById(const int& shipmentId) override {
        for (auto& shipment : inMemoryStore) {
            if (shipment.getShipmentId() == shipmentId) return &shipment;
        }
        return nullptr;
    }
* Trả về bản sao toàn bộ danh sách
    vector<Shipment> findAll() override {
        return inMemoryStore;
    }

* In nguyên nội dung tệp ra màn hình. Hàm này không thuộc interface, là chức năng riêng của lớp. Hiện chưa được gọi ở đâu trong chương trình.
    void displayHistoryFromFile() {
        ifstream inFile(filePath);
        if (!inFile.is_open()) {
            cout << "Chua co lich su kiem tra\n";
            return;
        }
        cout << "\n===== LICH SU KIEM TRA ===============\n";
        string line;
        while (getline(inFile, line)) cout << line << '\n';
        cout << "=========================================\n";
    }
};

# 2. enum tạo kiểu Tình huống xảy ra với 3 tình huống 

// Các kịch bản mô phỏng
enum class SimulationScenario {
    NORMAL,           // xe di chuyển bình thường
    COOLING_FAILURE,  // hỏng hệ thống làm lạnh (nhiệt độ tăng dần)
    DOOR_OPEN         // mở cửa xe đột ngột (nhiệt độ tăng vọt)
};
# 3. Class MockSensor: giả lập một cảm biến IoT gắn trên xe. Vì chưa có cảm biến thật, lớp này sinh dữ liệu giả gồm nhiệt độ, độ ẩm, vị trí và giờ
class MockSensor {
private:
    string SensorID;
    string containerID;
    float currentTemp;
    mutable mt19937 rng;  // máy tạo số ngẫu nhiên rng 

    // Số thực ngẫu nhiên trong [lo, hi]
    float randomFloat(float lo, float hi) const {
        if (hi <= lo) return lo;  // tránh lỗi khi khoảng bằng 0
        uniform_real_distribution<float> dist(lo, hi);
        return dist(rng); // trả ra số thực rng ngẫu nhiên 
    }
    // Số nguyên ngẫu nhiên trong [lo, hi]
    int randomInt(int lo, int hi) const {
        uniform_int_distribution<int> dist(lo, hi);
        return dist(rng); // trả ra số nguyên rng ngẫu nhiên 
    }

public:
    MockSensor(string id = "Sensor_01", string cId = "Container_01", float initialTemp = 4.0f)
        : SensorID(id), containerID(cId), currentTemp(initialTemp), rng(random_device{}()) {}

* Hàm planRun: tạo kế hoạch cho cả lượt chạy 
* Hàm này hỗ trợ chạy tình huống mock tự động ngẫu nhiên 
    vector<SimulationScenario> planRun(int totalSteps) const {
        vector<SimulationScenario> plan(totalSteps, SimulationScenario::NORMAL); // tạo kế hoạch gồm toàn NORMAL 

        int outcome = randomInt(0, 2); // bốc ngẫu nhiên 1 kết quả: 0,1,2
        int faults = 0;               // số bước bị lỗi
        if (outcome == 1) faults = randomInt(1, min(3, totalSteps));          // 1-3 lần -> ALERTED
        else if (outcome == 2) faults = randomInt(min(4, totalSteps), totalSteps);  // 4+ lần -> COMPROMISED

        // Xáo trộn thứ tự các bước rồi lấy 'faults' bước đầu làm bước lỗi
        vector<int> idx(totalSteps);
        iota(idx.begin(), idx.end(), 0); // xáo trộn danh sách chỉ số 
        shuffle(idx.begin(), idx.end(), rng);
        for (int i = 0; i < faults; ++i) {
            plan[idx[i]] = (randomInt(0, 1) == 0) ? SimulationScenario::COOLING_FAILURE
                                                  : SimulationScenario::DOOR_OPEN;
        }
        return plan; // trả ra kế hoạch kịch bản 
    }

* Hàm giúp sinh nhiệt độ ngẫu nhiên 
    float readCurrentTemp(SimulationScenario scenario, float minTemp, float maxTemp) {
        float mid = (minTemp + maxTemp) / 2.0f;
        float noiseLimit = min(0.5f, (maxTemp - minTemp) / 4.0f);  // nhiễu nhỏ hơn nếu khoảng an toàn hẹp

        switch (scenario) {
        // NORMAL: Lấy giữa khoảng an toàn cộng nhiễu nhỏ ngẫu nhiên.
            case SimulationScenario::NORMAL:
                currentTemp = mid + randomFloat(-noiseLimit, noiseLimit);
                break;
        // COOLING_FAILURE:	Cộng dồn thêm 1 đến 2 độ vào nhiệt độ hiện tại. Nếu vẫn chưa vượt maxTemp thì ép nó lên maxTemp + 0.5..1.5
            case SimulationScenario::COOLING_FAILURE:
                currentTemp += randomFloat(1.0f, 2.0f);                       // tăng dần
                if (currentTemp <= maxTemp) currentTemp = maxTemp + randomFloat(0.5f, 1.5f);
                break;
        // DOOR_OPEN:	Nhảy thẳng lên maxTemp + 4..8
            case SimulationScenario::DOOR_OPEN:
                currentTemp = maxTemp + randomFloat(4.0f, 8.0f);              // tăng vọt
                break;
        }
        return currentTemp;
    }

* Hàm Đổi kịch bản thành câu mô tả
    string scenarioToString(SimulationScenario scenario) const {
        switch (scenario) {
            case SimulationScenario::NORMAL: return "Xe di chuyen binh thuong !";
            case SimulationScenario::COOLING_FAILURE: return "Hong he thong lam lanh !";
            case SimulationScenario::DOOR_OPEN: return "Mo cua xe dot ngot !";
        }
        return "UNKNOWN";
    }
* Hàm Độ ẩm ngẫu nhiên 45 đến 85 (hiện chưa được dùng)
    float readHumidity() const { return randomFloat(45.0f, 85.0f); }

* Hàm tọa độ ngẫu nhiên: Vĩ độ 20.9 đến 21.1, kinh độ 105.7 đến 105.9 (quanh Hà Nội)
    Location readCurrentLocation() const {
        return Location(randomFloat(20.9f, 21.1f), randomFloat(105.7f, 105.9f));  // giả lập quanh Hà Nội
    }
* Hàm lấy thời gian thực 
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
using IoTSensorSimulator = MockSensor; // Từ khóa using IoTSensorSimulator = MockSensor; đặt thêm tên gọi khác cho cùng một lớp.

# 4. Class ConsoleLoggerObserver: một Observer dự phòng, dự kiến để in cảnh báo ra console. Hiện thân hàm chỉ có (void)event;, câu lệnh này chỉ để trình biên dịch khỏi báo “tham số không dùng”. Nghĩa là lớp chưa làm gì, và main cũng chưa tạo hay đăng ký nó.
class ConsoleLoggerObserver : public ITemperatureObserver {
public:
    virtual ~ConsoleLoggerObserver() {}
    void onTemperatureExceeded(const TemperatureExceededEvent& event) override {
        (void)event;
    }
};
#endif


# Nhận xét và hạn chế (có thể đưa vào phần đánh giá)
1. Ghi lại toàn bộ tệp mỗi lần lưu: với ít dữ liệu thì không sao, nhưng khi có hàng nghìn lô hàng sẽ chậm. Cải tiến: dùng cơ sở dữ liệu hoặc chỉ ghi phần thay đổi.
2. Ký tự | trong dữ liệu: tên sản phẩm, người gửi, người nhận do người dùng nhập, nếu chứa dấu | sẽ làm hỏng định dạng tệp. Chưa có bước xử lý.
3. Không lưu độ chính xác đầy đủ: nhiệt độ ghi 2 chữ số thập phân nên đọc lại có thể lệch rất nhỏ so với giá trị gốc.
4. Tên container cố định: khi nạp lại, model luôn là "Standard-Model", vì tệp không lưu trường này. Hiện không ảnh hưởng vì không nơi nào dùng đến.
5. findById trả con trỏ nội bộ: tiện nhưng nếu inMemoryStore thêm phần tử mới (làm vector cấp phát lại bộ nhớ) thì con trỏ cũ có thể không còn hợp lệ. Trong chương trình hiện tại con trỏ được dùng xong rồi mới save, nên chưa gây lỗi.
6. MockSensor có rng được khởi tạo ngay trong danh sách khởi tạo constructor nhưng khai báo sau currentTemp. Thứ tự khởi tạo thực tế theo thứ tự khai báo, không ảnh hưởng ở đây vì các thành viên độc lập nhau.