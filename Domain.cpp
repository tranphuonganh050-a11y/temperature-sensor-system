#ifndef DOMAIN_H
#define DOMAIN_H

#include <iostream>
#include <vector>
#include <string>
using namespace std;
enum class ShipmentStatus{ // tạo kiểu dữ liệu mới tên shipmentStatus 
    CREATED,
    IN_TRANSIT,
    ALERTED,
    DELIVERED, 
    COMPROMISED
};

inline string statusToString(ShipmentStatus s){
    switch(s){
        case ShipmentStatus::CREATED:     return "CREATED";
        case ShipmentStatus::IN_TRANSIT:  return "IN_TRANSIT";
        case ShipmentStatus::ALERTED:     return "ALERTED";
        case ShipmentStatus::DELIVERED:   return "DELIVERED";
        case ShipmentStatus::COMPROMISED: return "COMPROMISED";
    }
    return "UNKNOWN";
}
inline ShipmentStatus statusFromString(const string& s){
    if (s == "IN_TRANSIT")  return ShipmentStatus::IN_TRANSIT;
    if (s == "ALERTED")     return ShipmentStatus::ALERTED;
    if (s == "DELIVERED")   return ShipmentStatus::DELIVERED;
    if (s == "COMPROMISED") return ShipmentStatus::COMPROMISED;
    return ShipmentStatus::CREATED;
}
// Value Object: Phạm  vi nhiệt độ an toàn 
class TemperatureRange{
    private: 
        float minTemp;
        float maxTemp; 
    
    public:
        TemperatureRange(float min = 0.0f, float max = 0.0f){
            minTemp = min;
            maxTemp = max;
        }
        
        float getMin() const {return minTemp;}
        float getMax() const {return maxTemp;}
        bool isWithinRange(float temp) const {
            return (temp >= minTemp) && (temp <= maxTemp);
        }
};

//Value object: Tọa độ xe vận chuyển 
class Location{
    private: 
        float latitude; //vi do 
        float longitude; //kinh do 
    public: 
        Location(float lat = 0.0f, float lng = 0.0f){
            latitude = lat; 
            longitude = lng; 
        }
        float getLatitude() const {return latitude;}
        float getLongitude() const {return longitude;}

};

// Su kien vi pham nhiet do 
class TemperatureExceededEvent{
    private: 
        int shipmentId; 
        float exceededTemp; 
        float safeMin; 
        float safeMax; 
        Location location; 
        string timestamp; 
        string scenarioName; 
    public: 
        TemperatureExceededEvent(int id, float exceeded, float min, float max, const Location& loc, const string& time, const string& scenario = "UNKNOWN"){
            shipmentId = id; 
            exceededTemp = exceeded; 
            safeMin = min; 
            safeMax = max; 
            location = loc; 
            timestamp = time; 
            scenarioName = scenario; 
        }
        int getShipmentId() const {return shipmentId;}
        float getExceededTemp() const {return exceededTemp;}
        float getSafeMin() const {return safeMin;}
        float getSafeMax() const {return safeMax;}
        Location getLocation() const {return location;}
        string getTimestamp() const {return timestamp;}
        string getScenarioName() const {return scenarioName;}
};

// Log vi phạm: ghi lại các sự kiện vi phạm nhiệt độ 
class ViolationLog{ //ghi dấu vết vi phạm 
    private: 
        float recordedTemp; 
        Location location;
        string timestamp; 
    public: 
        ViolationLog(float temp, const Location& loc, const string& time){
            recordedTemp = temp;
            location = loc; 
            timestamp = time; 
        }
        float getRecordedTemp() const {return recordedTemp;} 
        Location getLocation() const {return location;}
        string getTimestamp() const {return timestamp;}
};
// Entity: Thùng chứa hàng - được định danh qua ID 
class ColdContainer{
    private: 
        int containerId; 
        string model; 
        TemperatureRange tempRange; // tempRange kieu TemperatureRange
    public: 
        ColdContainer(int id = 0, const string& mod = "", const TemperatureRange& range = TemperatureRange()){
            containerId = id;
            model = mod; 
            tempRange = range; 
        }
        int getContainerId() const {return containerId;}
        string getModel() const {return model;} 
        TemperatureRange getTempRange() const {return tempRange;}
        
        bool checkTemperature(float currentTemp) const {
            return tempRange.isWithinRange(currentTemp); //kiem tra trong nguong 
        }  
};
// Log ghi nhật mọi điểm dữ liệu cảm biến (vẽ biểu đồ) 
class TelemetryLog{
    private: 
        float temperature;
        Location location; 
        string timestamp; 
        string scenarioName;
    public: 
        TelemetryLog(float temp, const Location& loc, const string& time, const string& scenario = "unknown"){
            temperature = temp;
            location = loc; 
            timestamp = time; 
            scenarioName = scenario; 
        }

        float getTemperature() const{ return temperature;}
        Location getLocation() const{return location;}
        string getTimestamp() const{return timestamp;}
        string getScenarioName() const{return scenarioName;}
};

// Entity: Lô hàng ( chứa coldcontainer và violationLog) 
class Shipment{
    private: 
        int shipmentId;
        string productName; 
        string sender; 
        string receiver; 
        ShipmentStatus status; 
        ColdContainer container; 
        vector<ViolationLog> violation; 
        vector<TelemetryLog> telemetryHistory; 
    public:
        Shipment(int id=0, const string& name="", const string& send="", const string& rcv="", const ColdContainer& con=ColdContainer(), ShipmentStatus st=ShipmentStatus::CREATED){
            shipmentId = id; 
            productName = name; 
            sender = send;
            receiver = rcv;
            container = con; 
            status = st; 
        }
        int getShipmentId() const {return shipmentId;}
        string getProductName() const {return productName;}
        string getSender() const {return sender;}
        string getReceiver() const {return receiver;}
        ShipmentStatus getStatus() const {return status;}
        const vector<ViolationLog>& getViolations() const {return violation;}
        const ColdContainer& getContainer() const {return container;}
        const vector<TelemetryLog>& getTelemetryHistory() const{return telemetryHistory;}
        
        //cập nhật trạng thái khi lô hàng được tạo 
        void startTransit(){
            if (status == ShipmentStatus::CREATED){
                status = ShipmentStatus::IN_TRANSIT; 
            }
        }
        // đánh dấu trạng thái khi lô hàng đang được giao hoặc đã cảnh báo  
        void markDelivered(){
            if( status == ShipmentStatus::IN_TRANSIT || status == ShipmentStatus::ALERTED){
                status = ShipmentStatus::DELIVERED; 
            }
        }
         // cập nhật trạng thái 
        void finalStatus(){
            int vCount = static_cast<int>(violation.size());
            if (vCount == 0){
                status = ShipmentStatus::DELIVERED; // Khong vi pham lan nao 
            }
            else if (vCount <= 3){
                status = ShipmentStatus::ALERTED; // vi pham tu 1 den 3 lan --> Canh bao
            }
            else {
                status = ShipmentStatus::COMPROMISED; // vi pham trn 3 lan --> Hong nang/Huy hang 
            }
        }
        // Xử lý khi phát sinh event 
        bool processSensorData(float temp, const Location& loc, const string& timestamp, const string& scenarioName = "UNKNOWN"){
        
            telemetryHistory.push_back(TelemetryLog(temp, loc, timestamp, scenarioName));

        
            bool isExceeded = !container.checkTemperature(temp);
            if(isExceeded){ //Neu vuot nguong thì đổi status và được thêm vào violation
                status = ShipmentStatus::ALERTED; 
                violation.push_back(ViolationLog(temp,  loc, timestamp));
                return true;
            }
            return false;
        }
        void addViolation(const ViolationLog& v){
            violation.push_back(v);
        }
        void addTelemetry(const TelemetryLog& t){
            telemetryHistory.push_back(t);
        }

};
// interface Repository: Lưu trữ và truy xuất dữ liệu lô hàng
class IShipmentRepository{
    public: 
        virtual ~IShipmentRepository() {}
        virtual void save(const Shipment& shipment) = 0;
        virtual Shipment* findById(const int& shipmentId) = 0; 
        virtual vector<Shipment> findAll() = 0; 
};

class ITemperatureObserver{
    public: 
        virtual ~ITemperatureObserver() {}
        virtual void onTemperatureExceeded(const TemperatureExceededEvent& event) = 0;
};

class AlertService: public ITemperatureObserver{
    public: 
        void onTemperatureExceeded(const TemperatureExceededEvent& event) override{
        }
};
class ShipmentAggregator: public ITemperatureObserver{
    public: 
        void onTemperatureExceeded(const TemperatureExceededEvent& event) override{
        }
};
#endif
