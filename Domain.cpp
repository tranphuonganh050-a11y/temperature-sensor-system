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
        float lastRecordedTemp = 0.0f;
        Location lastLocation;
        string lastTimestamp;
        bool hasSensorReading = false;
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
        float getLastRecordedTemp() const { return lastRecordedTemp; }
        Location getLastLocation() const { return lastLocation; }
        string getLastTimestamp() const { return lastTimestamp; }
        bool hasReading() const { return hasSensorReading; }
        
        //cập nhật trạng thái khi lô hàng được tạo 
        void startTransit(){
            if (status == ShipmentStatus::CREATED){
                status = ShipmentStatus::IN_TRANSIT; 
            }
        }
        // đánh dấu trạng thái khi lô hàng đang được giao hoặc đã cảnh báo  
        void markTransit(){
            if( status == ShipmentStatus::IN_TRANSIT || status == ShipmentStatus::ALERTED){
                status = ShipmentStatus::DELIVERED; 
            }
        }

        // Xử lý khi phát sinh event 
        bool processSensorData(float temp, const Location& loc, const string& timestamp){
            // Lưu lần đo mới nhất dù nhiệt độ an toàn hay vi phạm.
            lastRecordedTemp = temp;
            lastLocation = loc;
            lastTimestamp = timestamp;
            hasSensorReading = true;

            if(!container.checkTemperature(temp)){
                status = ShipmentStatus::ALERTED;
                violation.push_back(ViolationLog(temp, loc, timestamp));
                return true;
            }
            // ALERTED được giữ lại như dấu vết rằng lô hàng từng vi phạm.
            return false;
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
            cout << "WARNING!!!! Lo hang " << event.getShipmentId()
                << " vi pham nhiet do bao quan !" << endl; 
        }
};
class ShipmentAggregator: public ITemperatureObserver{
    public: 
        void onTemperatureExceeded(const TemperatureExceededEvent& event) override{
            cout << "Ghi nhan nhat ky vi pham lo hang " << event.getShipmentId() << endl; 
        }
};
class DashboardNotifier: public ITemperatureObserver{
    public:
        void onTemperatureExceeded(const TemperatureExceededEvent& event) override{
            cout << event.getScenarioName() << endl; 

        }
};
#endif
