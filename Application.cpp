#ifndef APPLICATION_DTOS_H
#define APPLICATION_DTOS_H

#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>
#include <ctime> 
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
        containerId = 0; 
        minTemp = 0.0f;
        maxTemp = 0.0f;
    }
    CreateShipmentDTO(string prod, string snd, string rcv, int cntId, float minT, float maxT) {
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
    int shipmentId = 0;
    string productName;
    string status;
    float lastRecordedTemp;
    int violationCount;
    Location lastLocation; 
    float minTemp;
    float maxTemp; 
    vector<TelemetryLog> telemetryHistory; 

public:
    ShipmentDTO() {
        lastRecordedTemp = 0.0f;
        violationCount = 0;
        minTemp = 0.0f; 
        maxTemp = 0.0f; 
    }

    ShipmentDTO(int id, string prod, string st, float lastTemp, int violations, Location loc = Location(), float minT= 0.0f, float maxT = 0.0f, vector<TelemetryLog> telemetry = {}) {
        shipmentId = id;
        productName = prod;
        status = st;
        lastRecordedTemp = lastTemp;
        violationCount = violations;
        lastLocation = loc; 
        minTemp = minT; 
        maxTemp = maxT; 
        telemetryHistory = telemetry; 
    }

    int getShipmentId() const { return shipmentId; }
    string getProductName() const { return productName; }
    string getStatus() const { return status; }
    float getLastRecordedTemp() const { return lastRecordedTemp; }
    int getViolationCount() const { return violationCount; }
    Location getLastLocation() const {return lastLocation;}
    float getMinTemp() const {return minTemp;}
    float getMaxTemp() const {return maxTemp;}
    const vector<TelemetryLog>& getTelemetryHistory() const{return telemetryHistory;}
};

// Publisher quản lý danh sách Observers và phát sự kiện
class TelemetryProcessor { 
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
    IShipmentRepository* shipmentRepo; 
    EventPublisher* eventPublisher;    

    int generateUniqueId(){
        int maxId = 1000; 
        if(shipmentRepo != nullptr){
            for(const auto& s : shipmentRepo->findAll()){
                if(s.getShipmentId() > maxId) maxId = s.getShipmentId();
            }
        }
        return maxId + 1; 
    }
    static string currentTimeString(){
        time_t now = time(nullptr);
        tm* ltm = localtime(&now);
        char buf[32];
        strftime(buf, sizeof(buf), "%H:%M:%S %d/%m/%Y", ltm);
        return string(buf);
    }
public:
    // Constructor tiếp nhận các thành phần qua con trỏ
    ShipmentService(IShipmentRepository* repo, EventPublisher* publisher){
            shipmentRepo = repo; 
            eventPublisher = publisher; 
        }
    // Đánh dấu đã giao hàng <HÀM NÀY CHƯA ĐƯỢC DÙNG> 
    void markDelivered(int shipmentId){
        if(shipmentRepo == nullptr) return; 
        Shipment* shipment = shipmentRepo->findById(shipmentId);
        if(shipment) {
            shipment->markDelivered(); // chuyen sang Delivered 
            shipmentRepo->save(*shipment); 
        }
    }
    //Tạo lô hàng mới từ DTO
    int createShipment(const CreateShipmentDTO& dto) {
        float minT = dto.getMinTemp(); 
        float maxT = dto.getMaxTemp(); 
        if(minT > maxT) swap(minT, maxT); // neu nhap nguoc thi tu dao lai 
        // chỉ swap 2 biến sao chép, trong DTO vẫn chưa swap --> khoảng an toàn vẫn bị đảo ngược
        // Khắc phục: khởi tạo tempRange từ minT, maxT 
        TemperatureRange tempRange(minT, maxT);
        ColdContainer container(dto.getContainerId(), "Standard-Model", tempRange);

        int newId = generateUniqueId();
        Shipment shipment(newId, dto.getProductName(), dto.getSender(), dto.getReceiver(), container);

        shipment.startTransit();

        float startTemp = (minT + maxT) /2.0f; 
        shipment.addTelemetry(TelemetryLog(startTemp, Location(21.0f, 105.08f), currentTimeString(), "Khoi tao lo hang"));

        if (shipmentRepo != nullptr){
            shipmentRepo->save(shipment);
        }
        return newId;
    }

    //Tiếp nhận - xử lý dữ liệu cảm biến gửi về thời gian thực
    bool processSensorData(const int& shipmentId, float temp, Location loc, const string& time,const string& scenarioName = "UNKNOWN") {
        if (shipmentRepo == nullptr) return false;

        Shipment* shipment = shipmentRepo->findById(shipmentId);
        if (!shipment) {
            throw runtime_error("Khong tim thay lo hang voi ID: " + to_string(shipmentId));
        }

        bool isExceeded = shipment->processSensorData(temp, loc, time, scenarioName);

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
                eventPublisher->publish(event);
            }
        }
        shipmentRepo->save(*shipment);
        return isExceeded; 
    }

    vector<ShipmentDTO> getShipmentHistory() {
        vector<ShipmentDTO> dtoList;
        if (shipmentRepo == nullptr) return dtoList;

        auto shipments = shipmentRepo->findAll();

        for (const auto& shipment : shipments) {
            int violationCount = static_cast<int>(shipment.getViolations().size());
            float lastTemp = 0.0f;
            Location lastLoc; 

            const auto& tele = shipment.getTelemetryHistory(); 
            if(!tele.empty()){
                lastTemp = tele.back().getTemperature(); 
                lastLoc = tele.back().getLocation(); 
            }
            TemperatureRange range = shipment.getContainer().getTempRange();

            ShipmentDTO dto(
                shipment.getShipmentId(),
                shipment.getProductName(),
                statusToString(shipment.getStatus()),
                lastTemp,
                violationCount,
                lastLoc,
                range.getMin(), 
                range.getMax(), 
                tele
            );

            dtoList.push_back(dto);
        }
        return dtoList;
    }
    void finalSimulation(int shipmentId){
        if(shipmentRepo == nullptr) return; 
        Shipment* shipment = shipmentRepo->findById(shipmentId);
        if(shipment != nullptr){
            shipment->finalStatus(); // goi ham tu dong doi trang thai
            shipmentRepo->save(*shipment); // luu vao bo dem 
        }
    }
};
#endif
