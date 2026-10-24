# Application: Nhận yêu cầu từ giao diện khi người dùng nhập, gọi các đối tượng Domain làm việc rồi trả kết quả về. Application layer đóng vai trò cầu nối, chỉ phụ thuộc vào Domain, không biết gì về tệp, cảm biến hay giao diện 
# Các khái niệm được dùng: DTO, Application Service, Publisher (Observe Pattern), Dependency Injection. 
#ifndef APPLICATION_DTOS_H
#define APPLICATION_DTOS_H

#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>
#include "Domain.cpp"
using namespace std;
# 1. Class CreateShipmentDTO (Data Transfer Object): là đối tượng chỉ dùng để chở data giữa các layer, không chứa nghiệp vụ. 
* Cần dùng DTO vì giao diện không cần biết cách dựng Shipment, ColdContainer, TemperatureRange, mà chỉ cần gói thông tin vào một đối tượng đơn giản 
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
* Liên kết: ConsoleMenu::handleCreateShipment (Presentation) tạo DTO này từ dữ liệu người dùng nhập, rồi truyền cho ShipmentService::createShipment. (khi người dùng nhập thì data được truyền vào đây) 

# 2. Class ShipmentDTO: dùng để trả thông tin lô hàng từ Service ra giao diện (ngược lại CreateShipmentSTO) 
* DTO này làm phẳng data: thay vì để giao diện đi qua shipment.getContainer().getTempRange().getMin(), Service lấy sẵn data từ đây và đưa minTemp ra thẳng 
* Giao diện nhận được ShipmentDTO trong này, chứ không phải Shipment (Domain) --> không thể vô tình sửa trong Domain (tách biệt rõ Domain với giao diện)
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
* Liên kết: ShipmentService::getShipmentHistory tạo danh sách DTO này.ShipmentViewFormatter (Presentation) dùng để in bảng, AuditReportExporter (Extension) dùng để xuất CSV/Excel, AlertFeedLogger dùng để tra tên sản phẩm theo mã lô hàng.
(Mọi thứ dùng qua ShipmentDTO chứ không phải chọc thẳng vào Shipment (Domain) --> tách biệt rõ ràng hơn)

# 3. Class TelemetryProcessor: là bên phát sự kiện trong Observer Pattern
* TelemetryProcessor đóng vai trò Publisher trong mẫu thiết kế Observer. Lớp này quản lý danh sách các đối tượng cài đặt giao diện ITemperatureObserver và phát sự kiện TemperatureExceededEvent tới tất cả khi có vi phạm nhiệt độ. Nhờ đó, ShipmentService không cần biết ai nhận cảnh báo hay họ xử lý thế nào, giúp hệ thống giảm phụ thuộc giữa các thành phần và dễ mở rộng thêm các kiểu cảnh báo mới mà không phải sửa mã nguồn hiện có. 
// Publisher quản lý danh sách Observers và phát sự kiện
class TelemetryProcessor { //la TelemetryProcessor trong yeu cau cua thay 
    private: 
        vector<ITemperatureObserver*> observers; 
    public: 
* Hàm đăng kí thêm người nghe mới vào danh sách (thêm sự kiện) 
        void subscribe(ITemperatureObserver* obs) {
            observers.push_back(obs);
        }
* Hàm duyệt danh sách và gọi gọi onTemperatureExceeded(event) của từng người nghe.
        void publish(const TemperatureExceededEvent& event){
            for(auto obs : observers){
                obs->onTemperatureExceeded(event); // gọi hàm xử lý của observer
            }
        }
}; 
* Danh sách chứa ITemperatureObserver* (interface của Domain), nên mỗi phần tử có thể là 1 lớp khác nhau, nhưng đều được gọi bằng cùng 1 dòng obs- 
typedef TelemetryProcessor EventPublisher; // Đặt tên khác cho class này thui 

# 4. Class ShipmentService (Application Service): Đây là lớp quan trọng nhất trong Application Layer, đại diện cho các ca sử dụng của hệ thống 
class ShipmentService {
private:
    IShipmentRepository* shipmentRepo; // kho lưu trữ (interface)
    EventPublisher* eventPublisher;     // Bên phát sự kiện 

* Hàm giúp đánh số thứ tự lô hàng theo thứ tự tăng dần, bắt đầu là 1001 
    int generateUniqueId(){
        int maxId = 1000; 
        if(shipmentRepo != nullptr){
            for(const auto& s : shipmentRepo->findAll()){
                if(s.getShipmentId() > maxId) maxId = s.getShipmentId();
            }
        }
        return maxId + 1; 
    }
* Hàm lấy thời gian thực. Khai báo static vì không cần dữ liệu của đối tượng 
    static string currentTimeString(){
        time_t now = time(nullptr);
        tm* ltm = localtime(&now);
        char buf[32];
        strftime(buf, sizeof(buf), "%H:%M:%S %d/%m/%Y", ltm);
        return string(buf);
    }
* 2 hàm này private nên chỉ phục vụ nội bộ, bên ngoài không cần gọi 

public:
    // Constructor tiếp nhận các thành phần qua con trỏ
    ShipmentService(IShipmentRepository* repo, EventPublisher* publisher)
        : shipmentRepo(repo), eventPublisher(publisher) {}

*  Hàm đánh dấu đã giao 
    void markDelivered(int shipmentId){
        if(shipmentRepo == nullptr) return; 
        Shipment* shipment = shipmentRepo->findById(shipmentId);
        if(shipment) {
            shipment->markDelivered(); // chuyen sang Delivered 
            shipmentRepo->save(*shipment); 
        }
    }
* Nghiệp vụ 1: Tạo lô hàng mới 
    * ĐỌc ngưỡng từ DTO: 
    int createShipment(const CreateShipmentDTO& dto) {
        float minT = dto.getMinTemp(); 
        float maxT = dto.getMaxTemp(); 
        if(minT > maxT) swap(minT, maxT); // nếu nhập ngược nhiệt độ thì đảo lại 

    * Dựng TemperatureRange, ColdContainer cho lô hàng mới 
        TemperatureRange tempRange(dto.getMinTemp(), dto.getMaxTemp());
        ColdContainer container(dto.getContainerId(), "Standard-Model", tempRange);

    * Sinh mã ID và tạo Shipment mới 
        int newId = generateUniqueId();
        Shipment shipment(newId, dto.getProductName(), dto.getSender(), dto.getReceiver(), container);

    * Bắt đầu trạng thái vận chuyển
        shipment.startTransit();

    * Tạo nhiệt độ và tọa độ đầu tiên khi bắt đầu khởi tạo lô hàng mới 
        float startTemp = (minT + maxT) /2.0f; 
        shipment.addTelemetry(TelemetryLog(startTemp, Location(21.0f, 105.08f), currentTimeString(), "Khoi tao lo hang"));

    * Lưu thông tin lô hàng (lịch sử) và trả về mã lô hàng (trên giao diện)
        if (shipmentRepo != nullptr) {
            shipmentRepo->save(shipment);
        }

        return newId;
    }
* * * Service chỉ lắp ráp các đối tượng Domain theo đúng thứ tự, còn việc đổi trạng thái do Shipment tự quyết định.

* Nghiệp vụ 2: Tiếp nhận & xử lý dữ liệu cảm biến gửi về theo thời gian thực
    * Hàm xử lý dữ liệu cảm biến (theo id lô hàng, nhiệt độ đo được, tọa đô, thời gian thực)
    void processSensorData(const int& shipmentId, float temp, Location loc, const string& time,const string& scenarioName = "UNKNOWN") {
        if (shipmentRepo == nullptr) return;

        // Bước 1: Tìm lô hàng theo mã, không thấy thì ném runtime_error
        Shipment* shipment = shipmentRepo->findById(shipmentId);
        if (!shipment) {
            throw runtime_error("Khong tim thay lo hang voi ID: " + to_string(shipmentId));
        }

        // B2: Ghi vào telemetry và kiểm tra nhiệt độ trả về của lô hàng có vi phạm không 
        bool isExceeded = shipment->processSensorData(temp, loc, time, scenarioName);

        // B3: Nếu vi phạm nhiệt độ, dựng TemperatureExceededEvent rồi phát cảnh báo đi
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
* Việc phán đoán "có vượt ngưỡng không" nằm ở Domain, Service chỉ nhận kết quả true/false rồi quyết định có phát sự kiện không 

* Nghiệp vụ 3: Lấy lịch sử tất cả lô hàng để hiển thị ra UI (Chuyển đổi Entity sang DTO)
* Hàm này chuyển Entity sang DTO: trả về 1 danh sách thông tin tất cả lô hàng, ở dạng sẵn sàng để hiển thị 
    vector<ShipmentDTO> getShipmentHistory() {
        // Tạo danh sách rỗng để chứa kết quả. Nếu chưa có kho lưu trữ (nullptr nghĩa là con trỏ chưa trỏ đến đâu), trả luôn danh sách rỗng để tránh lỗi.
        vector<ShipmentDTO> dtoList;
        if (shipmentRepo == nullptr) return dtoList;

        // Lấy tất cả hồ sơ gốc: Gọi findAll() của interface IShipmentRepository. Thực tế phía sau là FileShipmentRepository::findAll(), nhưng Service không cần biết. auto nghĩa là để trình biên dịch tự suy ra kiểu (ở đây là vector<Shipment>).

        auto shipments = shipmentRepo->findAll();

        // Duyệt từng lô hàng: Lấy lần lượt từng Shipment ra xử lý. const auto& nghĩa là “chỉ xem, không sửa, không sao chép”, vừa nhanh vừa an toàn.
        for (const auto& shipment : shipments) {
        // Đếm số lần vi phạm bằng cách lấy kích thước danh sách ViolationLog. static_cast<int> đổi kiểu số cho đồng nhất.
            int violationCount = static_cast<int>(shipment.getViolations().size());
            float lastTemp = 0.0f;
            Location lastLoc; 

            const auto& tele = shipment.getTelemetryHistory(); 
            if(!tele.empty()){
                lastTemp = tele.back().getTemperature(); 
                lastLoc = tele.back().getLocation(); 
            }
            //Lấy lần đo cuối cùng. tele.back() là phần tử cuối của danh sách. Có kiểm tra !tele.empty() để tránh lỗi khi lô hàng chưa có lần đo nào.


            //Đi qua chuỗi Shipment → ColdContainer → TemperatureRange để lấy ngưỡng an toàn. Đây là việc phải “đào sâu” mà giao diện không cần làm.
            TemperatureRange range = shipment.getContainer().getTempRange();

            // Đóng gói thành DTO: Gom tất cả vào một ShipmentDTO, rồi bỏ vào danh sách kết quả. Chú ý statusToString đổi trạng thái từ enum (ví dụ ShipmentStatus::ALERTED) thành chữ "ALERTED" để giao diện in ra được ngay.
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
        // trả ra danh sách 
        return dtoList;
    }
* Hàm này tìm lô hàng, gọi shipment->finalStatus() ((Domain chốt trạng thái cuối theo số lần vi phạm), rồi lưu lại.)
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



# Nhận xét và hạn chế (có thể đưa vào phần đánh giá của báo cáo)
1. markDelivered chưa được dùng: hàm đã có trong Service nhưng menu chưa có chức năng gọi nó.
2. TelemetryProcessor giữ con trỏ thô: nó không sở hữu các observer, nên các observer phải còn sống suốt thời gian chạy (trong main thì thỏa mãn). 
3. Lớp cũng chưa có hàm hủy đăng ký.
4. ShipmentDTO còn chứa đối tượng Domain (Location, TelemetryLog), nên chưa tách hoàn toàn giao diện khỏi Domain. Với quy mô đề tài, đây là chấp nhận được.
