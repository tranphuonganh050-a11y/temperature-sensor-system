#ifndef INFRASTRUCTURE_H
#define INFRASTRUCTURE_H

#include "Domain.cpp"
#include <iostream>
#include <random>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

class FileShipmentRepository: public IShipmentRepository {
private:
    string filePath;
    vector<Shipment> inMemoryStore; // Lưu trữ tạm thời để truy xuất nhanh 
    string statusToString(ShipmentStatus status){
        switch (status){
            case ShipmentStatus::CREATED: return "CREATED";
            case ShipmentStatus::IN_TRANSIT: return "IN_TRANSIT";
            case ShipmentStatus::ALERTED: return "ALERTED";
            case ShipmentStatus::DELIVERED: return "DELIVERED";
            case ShipmentStatus::COMPROMISED: return "COMPROMISED";
            default: return "UNKNOWN";
        }
    }

public:
    FileShipmentRepository(const string& path = "LICH_SU_KIEM_TRA.txt"){
        filePath = path;
    }

    // Ghi trực tiếp thông tin lô hàng vào file
    void save(const Shipment& shipment)
    {   
        // Cập nhật vào bộ đệm 
        // kiểm tra xem lô hàng đã tồn tại trong bộ đệm chưa trước khi push 
        bool found = false; 
        for(auto& s: inMemoryStore){
            if(s.getShipmentId() == shipment.getShipmentId()){
                s = shipment; 
                found = true;
                break;
            }
        }
        // nếu chưa tồn tại thì push 
        if (!found) inMemoryStore.push_back(shipment);
        ofstream outFile(filePath, std::ios::app);
        if (!outFile.is_open())
        {
            cout << "LOI! Khong the mo file ghi du lieu: " << filePath << endl;
            return;
        }

        // Ghi dòng dữ liệu lô hàng
        outFile << "SHIPMENT|" 
                << shipment.getShipmentId() << "|"
                << shipment.getProductName() << "|"
                << shipment.getSender() << "|"
                << shipment.getReceiver() << "|"
                << statusToString(shipment.getStatus()) << "\n";

        // Ghi danh sách vi phạm nhiệt độ (nếu có)
        for (const auto& v : shipment.getViolations())
        {
            outFile << "VIOLATION|" 
                    << shipment.getShipmentId() << "|"
                    << v.getTimestamp() << "|"
                    << v.getRecordedTemp() << "|"
                    << v.getLocation().getLatitude() << "," << v.getLocation().getLongitude() << "\n";
        }
        outFile.close();
    }
    //Tìm kiếm lô hàng theo ID từ bộ đệm 
    Shipment* findById(const int& shipmentId) override{
        for(auto& shipment : inMemoryStore){
            if(shipment.getShipmentId() == shipmentId) {
                return &shipment; // Trả về con trỏ đến lô hàng tìm thấy
            }
        }return nullptr; // Không tìm thấy
    }
    //Lấy danh sách tất cả lô hàng từ bộ đệm 
    vector<Shipment> findAll() override{
        return inMemoryStore; // Trả về danh sách tất cả lô hàng
    }

    // Đọc tất cả dữ liệu từ file ra màn hình/danh sách
    void displayHistoryFromFile()
    {
        std::ifstream inFile(filePath);
        if (!inFile.is_open())
        {
            std::cout << "Chua co lich su kiem tra\n";
            return;
        }

        std::cout << "\n==========================================================================" << std::endl;
        std::cout << "|                   LICH SU KIEM TRA TU FILE TEXT                        |" << std::endl;
        std::cout << "==========================================================================" << std::endl;
        std::string line;
        while (std::getline(inFile, line))
        {
            std::cout << line << std::endl;
        }
        inFile.close();
        std::cout << "--------------------------------------------------------------------------\n" << std::endl;
    }
};
//Định nghĩa các kịch bản mô phỏng 
enum class SimulationScenario{ 
    NORMAL,// xe di chuyển bình thường 
    COOLING_FAILURE, // hỏng hệ thống làm lạnh (tăng dần 1 độ mỗi lần đọc)
    DOOR_OPEN // mở cửa xe đột ngột (nhiệt độ tăng vọt)
};
class MockSensor
{
    private:
        string SensorID;
        string containerID;
        float currentTemp; 
        mutable mt19937 rng; //May tao so ngau nhien

    public:
        MockSensor(string id = "Sensor_01", string cId = "Container_01", float initialTemp = 4.0f)
         : SensorID(id), containerID(cId), currentTemp(initialTemp), rng(random_device{}())
        {
            random_device rd;
            rng.seed(rd());
        }
        //Tự động sinh nhiệt độ theo kịch bản mô phỏng
        float readCurrentTemp(int step, SimulationScenario& scenario, float minTemp = -10.0f, float maxTemp = 0.0f){
            uniform_real_distribution<float> noise(-0.5f, 0.5f);  //làm nhiễu số cho tự nhiên 
            // phân chia thứ tự tình huống tự động theo các trường hợp mô phỏng 
            if(step <= 2){
                scenario = SimulationScenario::NORMAL;
            }
            else if(step <= 4){
                scenario = SimulationScenario::COOLING_FAILURE;
            }
            else{
                scenario = SimulationScenario::DOOR_OPEN; 
            }
            //Mô phỏng biến đổi nhiệt độ 
            switch(scenario){
                case SimulationScenario::NORMAL:  //trường họp xe bình thường 
                    currentTemp = (minTemp + maxTemp)/2.0f + noise(rng); // + số  nhiễu 
                    break;
                case SimulationScenario::COOLING_FAILURE:
                    currentTemp += 1.0f + noise(rng); //th hỏng hệ thống lạnh 
                    break;
                case SimulationScenario::DOOR_OPEN:
                    currentTemp += 6.0f + noise(rng); //th mở cửa xe 
                    break;
                default:
                    break;
            }
            return currentTemp;
        }
        string scenarioToString(SimulationScenario scenario){
            switch(scenario){
                case SimulationScenario::NORMAL: return "Xe di chuyen binh thuong !";
                case SimulationScenario::COOLING_FAILURE: return "Hong he thong lam lanh !";
                case SimulationScenario::DOOR_OPEN: return "Mo cua xe dot ngot !";
                default: return "UNKNOWN";
            }
        }
        //Tự động sinh độ ẩm (humidity) (45% - 85%)
        float readHumidity() const{
            uniform_real_distribution<float> dist(45.0f, 85.0f);
            return dist(rng); 
        }
        // Tự động sinh tọa độ 
        Location readCurrentLocation() const{
            uniform_real_distribution<float> latDist(20.9f, 21.1f); // Giả lập Hà Nội
            uniform_real_distribution<float> lngDist(105.7f, 105.9f);
            return Location(latDist(rng), lngDist(rng));
        }
        //Lấy thời gian thực
        string getCurrentTime() const{
            time_t now = time(nullptr);
            tm* ltm = localtime(&now);
            char buf[32];
            strftime(buf, sizeof(buf), "%H:%M:%S %d/%m/%Y", ltm);
            return string(buf);
        }
        string getSensorID()  const {return SensorID;}
        string getContainerID() const {return containerID;}
};
using IoTSensorSimulator = MockSensor; 
class ConsoleLoggerObserver: public ITemperatureObserver{
    public: 
        virtual ~ConsoleLoggerObserver() {}
        void onTemperatureExceeded(const TemperatureExceededEvent& event) override {
            (void)event; 
    }
};
#endif