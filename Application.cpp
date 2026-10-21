#ifndef APPLICATION_DTOS_H
#define APPLICATION_DTOS_H

#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>
#include "Domain.cpp"
using namespace std;

// DTO dùng để nhận dữ liệu khi tạo mới lô hàng từ UI/Client
class CreateShipmentDTO {
private:
    string productName;
    string sender;
    string receiver;
    int containerId;
    float minTemp;
    float maxTemp;

public:

    CreateShipmentDTO() {
        minTemp = 0.0f;
        maxTemp = 0.0f;
    }

    CreateShipmentDTO(string prod, string snd, string rcv,
                      int cntId, float minT, float maxT) {
        productName = prod;
        sender = snd;
        receiver = rcv;
        containerId = cntId;
        minTemp = minT;
        maxTemp = maxT;
    }

    string getProductName() const { return productName; }
    string getSender() const { return sender; }
    string getReceiver() const { return receiver; }
    int getContainerId() const { return containerId; }
    float getMinTemp() const { return minTemp; }
    float getMaxTemp() const { return maxTemp; }
};

// DTO dùng để trả dữ liệu thông tin lô hàng ra UI
class ShipmentDTO {
private:
    int shipmentId;
    string productName;
    string status;
    float lastRecordedTemp;
    int violationCount;
    Location lastLocation; 

public:
    ShipmentDTO() {
        lastRecordedTemp = 0.0f;
        violationCount = 0;
    }

    ShipmentDTO(int id, string prod, string st, float lastTemp, int violations, Location loc = Location()) {
        shipmentId = id;
        productName = prod;
        status = st;
        lastRecordedTemp = lastTemp;
        violationCount = violations;
        lastLocation = loc; 
    }

    int getShipmentId() const { return shipmentId; }
    string getProductName() const { return productName; }
    string getStatus() const { return status; }
    float getLastRecordedTemp() const { return lastRecordedTemp; }
    int getViolationCount() const { return violationCount; }
    Location getLastLocation() const {return lastLocation;}
};

// Publisher quản lý danh sách Observers và phát sự kiện
class TelemetryProcessor { //la TelemetryProcessor trong yeu cau cua thay 
    private: 
        vector<ITemperatureObserver*> observers; 
    public: 
        void subscribe(ITemperatureObserver* obs) {
            observers.push_back(obs);
        }
        void publish(const TemperatureExceededEvent& event){
            for(auto obs : observers){
                obs->onTemperatureExceeded(event); // gọi hàm xử lý của observer
            }
        }
};
typedef TelemetryProcessor EventPublisher; 

class ShipmentService {
private:
    IShipmentRepository* shipmentRepo; // Dependency Injection Repository
    EventPublisher* eventPublisher;     // Dependency Injection Publisher

    // Hàm phụ trợ sinh mã tự động (Helper)
    int generateUniqueId() {
        static int counter = 1000;
        return ++counter;
    }

public:
    // Constructor tiếp nhận các thành phần qua con trỏ
    ShipmentService(IShipmentRepository* repo, EventPublisher* publisher)
        : shipmentRepo(repo), eventPublisher(publisher) {}

    // 1. Nghiệp vụ: Tạo lô hàng mới từ DTO
    int createShipment(const CreateShipmentDTO& dto) {
        if (dto.getMinTemp() > dto.getMaxTemp()) {
            throw invalid_argument("Nhiet do toi thieu khong duoc lon hon nhiet do toi da.");
        }
        if (dto.getContainerId() <= 0) {
            throw invalid_argument("Container ID phai la so duong.");
        }
        if (dto.getProductName().empty() || dto.getSender().empty() || dto.getReceiver().empty()) {
            throw invalid_argument("Ten san pham, nguoi gui va nguoi nhan khong duoc de trong.");
        }
        // Tạo Value Object & Entity con thuộc Domain
        TemperatureRange tempRange(dto.getMinTemp(), dto.getMaxTemp());
        ColdContainer container(dto.getContainerId(), "Standard-Model", tempRange);

        // Sinh mã ID và khởi tạo Aggregate Root
        int newId = generateUniqueId();
        Shipment shipment(newId, dto.getProductName(), dto.getSender(), dto.getReceiver(), container);

        // Bắt đầu trạng thái vận chuyển
        shipment.startTransit();

        // Lưu thông qua Repository Pattern
        if (shipmentRepo != nullptr) {
            shipmentRepo->save(shipment);
        }

        return newId;
    }

    // 2. Nghiệp vụ: Tiếp nhận & xử lý dữ liệu cảm biến gửi về thời gian thực
    void processSensorData(const int& shipmentId, float temp, Location loc, const string& time,const string& scenarioName = "UNKNOWN") {
        if (shipmentRepo == nullptr) return;

        // B1: Lấy lô hàng từ DB theo ID
        Shipment* shipment = shipmentRepo->findById(shipmentId);
        if (!shipment) {
            throw runtime_error("Khong tim thay lo hang voi ID: " + to_string(shipmentId));
        }

        // B2: Gọi domain logic xử lý dữ liệu cảm biến
        bool isExceeded = shipment->processSensorData(temp, loc, time);

        // B3: Kiểm tra nếu có vi phạm mới bùng phát -> Phát Domain Event ra ngoài
        if (isExceeded) {
            TemperatureRange range = shipment->getContainer().getTempRange();

            TemperatureExceededEvent event(
                shipmentId,
                temp,
                range.getMin(),
                range.getMax(),
                loc,
                time,
                scenarioName
            );

            if (eventPublisher != nullptr) {
                eventPublisher->publish(event); // Bắn sự kiện tới Observers (Alert, UI...)
            }
        }

        // B4: Cập nhật lại dữ liệu lô hàng vào Database
        shipmentRepo->save(*shipment);
    }

    // Tra cứu khoảng nhiệt độ an toàn theo đúng lô hàng đang được mô phỏng.
    bool getTemperatureRange(int shipmentId, float& minTemp, float& maxTemp) {
        if (shipmentRepo == nullptr) return false;
        Shipment* shipment = shipmentRepo->findById(shipmentId);
        if (shipment == nullptr) return false;
        const TemperatureRange range = shipment->getContainer().getTempRange();
        minTemp = range.getMin();
        maxTemp = range.getMax();
        return true;
    }

    // 3. Nghiệp vụ: Lấy lịch sử tất cả lô hàng để hiển thị ra UI (Chuyển đổi Entity sang DTO)
    vector<ShipmentDTO> getShipmentHistory() {
        vector<ShipmentDTO> dtoList;
        if (shipmentRepo == nullptr) return dtoList;

        // Lấy danh sách Entities từ Repository
        auto shipments = shipmentRepo->findAll();

        // Map từ Entity Domain sang DTO UI
        for (const auto& shipment : shipments) {
            string statusStr = "UNKNOWN";
            switch (shipment.getStatus()) {
                case ShipmentStatus::CREATED: statusStr = "CREATED"; break;
                case ShipmentStatus::IN_TRANSIT: statusStr = "IN_TRANSIT"; break;
                case ShipmentStatus::ALERTED: statusStr = "ALERTED"; break;
                case ShipmentStatus::DELIVERED: statusStr = "DELIVERED"; break;
                case ShipmentStatus::COMPROMISED: statusStr = "COMPROMISED"; break;
            }

            int violationCount = static_cast<int>(shipment.getViolations().size());
            // Lấy dữ liệu của lần đo mới nhất, không chỉ lần vi phạm gần nhất.
            float lastTemp = shipment.hasReading() ? shipment.getLastRecordedTemp() : 0.0f;
            Location lastLoc = shipment.hasReading() ? shipment.getLastLocation() : Location();

            ShipmentDTO dto(
                shipment.getShipmentId(),
                shipment.getProductName(),
                statusStr,
                lastTemp,
                violationCount,
                lastLoc
            );

            dtoList.push_back(dto);
        }

        return dtoList;
    }
};

#endif
