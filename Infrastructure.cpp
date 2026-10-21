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
    vector<Shipment> inMemoryStore; // Tam thoi luu tru 
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
    void save(const Shipment& shipment)
    {
        bool found = false;
        size_t alreadyLoggedViolations = 0;
        for(auto& s: inMemoryStore){
            if(s.getShipmentId() == shipment.getShipmentId()){
                alreadyLoggedViolations = s.getViolations().size();
                s = shipment;
                found = true;
                break;
            }
        }
        if (!found) inMemoryStore.push_back(shipment);
        ofstream outFile(filePath, std::ios::app);
        if (!outFile.is_open())
        {
            cout << "LOI! Khong the mo file ghi du lieu: " << filePath << endl;
            return;
        }
        outFile << "SHIPMENT|"                                              // Ghi dong du lieu
                << shipment.getShipmentId() << "|"
                << shipment.getProductName() << "|"
                << shipment.getSender() << "|"
                << shipment.getReceiver() << "|"
                << statusToString(shipment.getStatus()) << "\n";
        // Ghi cac vi pham phat sinh moi ke tu lan save truoc, tranh lap lai log cu.
        // File nay la audit log; repository chua nap lai trang thai tu file khi khoi dong.
        const auto& violations = shipment.getViolations();
        for (size_t i = alreadyLoggedViolations; i < violations.size(); ++i) {
            const auto& v = violations[i];
            outFile << "VIOLATION|"
                    << shipment.getShipmentId() << "|"
                    << v.getTimestamp() << "|"
                    << v.getRecordedTemp() << "|"
                    << v.getLocation().getLatitude() << "," << v.getLocation().getLongitude() << "\n";
        }
        outFile.close();
    }
    Shipment* findById(const int& shipmentId) override{
        for(auto& shipment : inMemoryStore){
            if(shipment.getShipmentId() == shipmentId) {
                return &shipment;
            }
        }return nullptr;
    }
    vector<Shipment> findAll() override{
        return inMemoryStore;
    }
    void displayHistoryFromFile() //Do du lieu tu file
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
enum class SimulationScenario{ 
    NORMAL,
    COOLING_FAILURE,
    DOOR_OPEN
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
        {}
        float readCurrentTemp(int step, SimulationScenario& scenario, float minTemp = -10.0f, float maxTemp = 0.0f){
            uniform_real_distribution<float> noise(-0.5f, 0.5f);  //lam nhieu so
            if(step <= 2){
                scenario = SimulationScenario::NORMAL;
            }
            else if(step <= 4){
                scenario = SimulationScenario::COOLING_FAILURE;
            }
            else{
                scenario = SimulationScenario::DOOR_OPEN; 
            }
            switch(scenario){
                case SimulationScenario::NORMAL:  //Truong hop binh thuong
                    currentTemp = (minTemp + maxTemp)/2.0f + noise(rng); // cong them so nhieu
                    break;
                case SimulationScenario::COOLING_FAILURE:
                    currentTemp += 1.0f + noise(rng); //Truong hop hong he thong lam lanh
                    break;
                case SimulationScenario::DOOR_OPEN:
                    currentTemp += 6.0f + noise(rng); //Truong hop mo cua xe
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
        float readHumidity() const{  //Do am (Tu 45%-85%)
            uniform_real_distribution<float> dist(45.0f, 85.0f);
            return dist(rng); 
        }
        Location readCurrentLocation() const{
            uniform_real_distribution<float> latDist(20.9f, 21.1f); // Kinh do tai Ha Noi
            uniform_real_distribution<float> lngDist(105.7f, 105.9f); // Vi do tai Ha Noi
            return Location(latDist(rng), lngDist(rng));
        }
        string getCurrentTime() const{  //Thoi gian thuc
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