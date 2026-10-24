# Giải thích domain.cpp 

- Domain là tầng lõi của hệ thống giám sát chuỗi cung ứng lạnh (Cold Chain). Tầng này mô hình hóa các khái niệm nghiệp vụ (lô hàng, thùng lạnh, ngưỡng nhiệt độ, vi phạm) và các quy tắc đi kèm. Domain layer độc lập với giao diện, lưu trữ và chạy cảm biến. Trong khi các layer còn lại đều gọi đến các class trong Domain, Domain layer không phụ thuộc vào các layer khác
- Tầng Domain áp dụng các khái niệm của lập trình hướng đối tượng và Domain-Driven Design (DDD), gồm: Value Object, Entity, Aggregate Root, Domain Event, Repository Interface và Observer Pattern.


# 1. enum class ShipmentStatus: tạo 1 kiểu dữ liệu mới biểu diễn vòng đời của 1 lô hàng qua 5 trạng thái cố định 
- Dùng enum giúp tránh lỗi gõ sai, giúp mã an toàn và rõ nghĩa hơn 
enum class ShipmentStatus{ 
    CREATED,      // vừa tạo 
    IN_TRANSIT,  // đang vận chuyển 
    ALERTED,     // có vi phạm nhiệt độ ở mức cảnh báo 
    DELIVERED,   // đã giao hàng thành công 
    COMPROMISED  //vi phạm quá nhiều lần/hư hỏng 
};
* Hai hàm chuyển đổi trạng thái: 
* Khai báo hàm inline để có thể đặt trong tệp header mà không gây lỗi định nghĩa trùng 
* Đổi trạng thái thành chuỗi kí tự. Application layer dùng để tạo DTO hiển thị, Infrastructure layer dùng khi ghi tệp 
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
* Đổi chuỗi kí tự thành trạng thái, dùng khi đọc dữ liệu từ tệp 
inline ShipmentStatus statusFromString(const string& s){
    if (s == "IN_TRANSIT")  return ShipmentStatus::IN_TRANSIT;
    if (s == "ALERTED")     return ShipmentStatus::ALERTED;
    if (s == "DELIVERED")   return ShipmentStatus::DELIVERED;
    if (s == "COMPROMISED") return ShipmentStatus::COMPROMISED;
    return ShipmentStatus::CREATED;
}
# 2. Value Object: TemperatureRange: biểu diễn khoảng nhiệt độ an toàn của hàng hóa (chỉ mô phỏng giá trị, không có định danh riêng ) 
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
* Hàm isWithinRange chứa quy tắc nghiệp vụ của hệ thống: nhiệt độ hợp lệ khi minTemp <= temp <= maxTemp
        bool isWithinRange(float temp) const {
            return (temp >= minTemp) && (temp <= maxTemp);
        }
};
* Liên kết: ColdContainer chứa 1 đối tượng này. ShipmentService::createShipment (Application) tạo nó từ dữ liệu người dùng nhập. TemperatureExceededEvent lấy getMin(), getMax() từ nó.
# 3. Value Object: Location : biểu diễn tọa độ (vĩ độ, kinh độ) của xe vận chuyển 
* Thay vì truyền tham số float rời rạc, hệ thống truyền 1 đối tượng Location, giúp mã gọn và tránh nhầm lẫn thứ tự 
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
* Liên kết: là thành phần của ViolationLog, TelemetryLog và TemperatureExceededEvent. MockSensor::readCurrentLocation() (Infrastructure) tạo ra đối tượng này, FileShipmentRepository đọc và ghi nó qua parseLocation.

# 4. Domain Event TemperatureExceededEvent: biểu diễn sự kiện đã xảy ra vượt ngưỡng nhiệt độ. Class này lưu mã lô hàng, nhiệt độ vi phạm, ngường an toàn, vị trí, thời gian và tên kịch bản 
* Domain event giúp ghi lại 1 sự kiện đã xảy ra trong nghiệp vụ, sự kiện này chỉ đọc (get), không có hàm sửa vì quá khứ không thể thay đổi 
class TemperatureExceededEvent{
    private: 
        int shipmentId; 
        float exceededTemp; 
        float safeMin; 
        float safeMax; 
        Location location;  // chứa đối tượng Location 
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
* Liên kết: ShipmentService::processSensorData tạo domain event khi Shipment báo có vi phạm, sau đó TelemetryProcessor::publish gửi nó tới các Observer. AlertFeedLogger (Extension) lưu lại để hiển thị bảng “Ghi nhận vi phạm”.

# 5. Class ViolationLog và TelemetryLog: hai lớp này là bản ghi dữ liệu (log), chỉ lưu và cung cấp get, không chứa logic.
* Trong đó: 
- ViolationLog lưu nhiệt độ, vị trí, thời gian chỉ khi có vi phạm, mục đích đếm và truy vết vi phạm 
- TelemetryLog lưu nhiệt độ, vị trí, thời gian và tên kịch bản cho mọi lần đo, mục đích xuất báo cáo và vẽ biểu đồ 
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
// Log ghi nhật mọi điểm dữ liệu cảm biến
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
* Liên kết: Số lượng ViolationLog trong Shipment chính là căn cứ để quyết định trạng thái cuối. Tầng Infrastructure ghi chúng thành các dòng VIOLATION|... và TELEMETRY|... trong tệp, còn Extension.cpp đọc TelemetryLog (qua DTO) để dựng bảng dữ liệu biến động nhiệt độ.

# 6. Entity ColdContainer (thực thể): biểu diễn thùng chứa hàng lạnh trên xe. Thùng lạnh được nhận diện qua containerId. 
class ColdContainer{
    private: 
        int containerId; 
        string model; 
        TemperatureRange tempRange; // khoảng nhiệt  TemperatureRange
    public: 
        ColdContainer(int id = 0, const string& mod = "", const TemperatureRange& range = TemperatureRange()){
            containerId = id;
            model = mod; 
            tempRange = range; 
        }
        int getContainerId() const {return containerId;}
        string getModel() const {return model;} 
        TemperatureRange getTempRange() const {return tempRange;}

* Hàm checkTemperature thay vì tự so sánh, nó gọi tempRange.isWithinRange(currentTemp) để kiểm tra. Mỗi class chỉ làm đúng việc của mình 
        bool checkTemperature(float currentTemp) const {
            return tempRange.isWithinRange(currentTemp);
        }  
};
* Liên kết: là thành phần của Shipment. ShipmentService::createShipment tạo nó, FileShipmentRepository::loadFromFile dựng lại nó từ dữ liệu trong tệp.

# 7. Aggregate Root Shipment: là lớp trung tâm của hệ thống, đại diện cho 1 lô hàng. Class gom các đối tượng liên quan (ColdContainer, ViolationLog, TelemetryLog) và chịu tránh nhiệm giữ các quy tắc nhiệm vụ nhất quán 
* Aggregate Root bên ngoài chỉ làm việc với Shipment, không thao tác trực tiếp lên log hay thùng lạnh. 
* một Shipment “có một” ColdContainer và “có nhiều” log.
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
        // Dùng vector tạo danh sách động vì không biết số lần ghi vào danh sách 
    public:
        Shipment(int id=0, const string& name="", const string& send="", const string& rcv="", const ColdContainer& con=ColdContainer(), ShipmentStatus st=ShipmentStatus::CREATED){
            shipmentId = id; 
            productName = name; 
            sender = send;
            receiver = rcv;
            container = con; 
            status = st; 
        }
* Hàm getViolations() và getTelemetryHistory() trả về tham chiếu const, nên bên ngoài chỉ đọc được, không sửa được.
        int getShipmentId() const {return shipmentId;}
        string getProductName() const {return productName;}
        string getSender() const {return sender;}
        string getReceiver() const {return receiver;}
        ShipmentStatus getStatus() const {return status;}
        const vector<ViolationLog>& getViolations() const {return violation;}
        const ColdContainer& getContainer() const {return container;}
        const vector<TelemetryLog>& getTelemetryHistory() const{return telemetryHistory;}
        
* Hàm chuyển CREATED sang IN_TRANSIT (chỉ khi đang CREATED) 
        void startTransit(){
            if (status == ShipmentStatus::CREATED){
                status = ShipmentStatus::IN_TRANSIT; 
            }
        }
* Hàm chuyển DELIVERED nếu đang IN_TRANSIT hoặc ALERTED	
        void markDelivered(){
            if( status == ShipmentStatus::IN_TRANSIT || status == ShipmentStatus::ALERTED){
                status = ShipmentStatus::DELIVERED; 
            }
        }
* Hàm chốt trạng thái cuối cùng: 0 vi phạm là DELIVERED, 1 đến 3 là ALERTED, trên 3 là COMPROMISED
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
* Hàm ghi một TelemetryLog, kiểm tra nhiệt độ qua container.checkTemperature. Nếu vượt ngưỡng thì chuyển trạng thái sang ALERTED, thêm ViolationLog và trả về true, ngược lại trả về false
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
* Thêm 1 log vi phạm 
        void addViolation(const ViolationLog& v){
            violation.push_back(v);
        }
* Thêm 1 log đo 
        void addTelemetry(const TelemetryLog& t){
            telemetryHistory.push_back(t);
        }
};
* Các luật như “trên 3 lần vi phạm thì hủy hàng” nằm ngay trong Shipment::finalStatus() thay vì ở Service hay Menu. Nhờ vậy khi nghiệp vụ thay đổi chỉ cần sửa một nơi.

# 8. Interface IShipmentRepository: định nghĩa "hợp đồng" lưu trữ và truy xuất lô hàng mà không quy định cách lưu. 
* Lớp trừu tượng/interface có các hàm = 0 là hàm thuần ảo, lớp con bắt buộc phải cài đặt 
* Nó đảm bảo khi xóa đối tượng qua con trở lớp cha, destructor của lớp con vẫn được gọi 
* Interface: muốn làm kho lưu trữ lô hàng thì phải có 3 chức năng: save, findById, findAll. Ai làm được 3 việc đó thì có thể cắm vào interface. 
* ShipmentService giữ con trỏ IShipmentRepository* và gọi save, findById, findAll mà không cần biết lớp thật phía sau.
class IShipmentRepository{
    public: 
        virtual ~IShipmentRepository() {}
        virtual void save(const Shipment& shipment) = 0;
        virtual Shipment* findById(const int& shipmentId) = 0; 
        virtual vector<Shipment> findAll() = 0; 
};
* Liên kết: Domain định nghĩa interface, Infrastructure (FileShipmentRepository) cài đặt, và hàm main truyền &repo vào ShipmentService. Muốn chuyển từ lưu tệp sang cơ sở dữ liệu chỉ cần viết lớp mới kế thừa interface, không phải sửa Service.
* Interface quy định tập hợp các chức năng mà lớp cài đặt bắt buộc phải cung cấp. Bên sử dụng chỉ làm việc thông qua các chức năng đó mà không phụ thuộc vào cách chúng được cài đặt, nhờ đó có thể thay đổi cách cài đặt mà không ảnh hưởng đến phần còn lại của hệ thống  

# 9. Observer pattern: ITemperatureObserver: cho phép nhiều thành phần cùng "lắng nghe" sự kiện vượt ngưỡng nhiệt độ mà không phụ thuộc lẫn nhau 
* ITemperatureObserver là giao diện của người quan sát. TelemetryProcessor (Application) đóng vai trò Publisher, giữ danh sách observer qua subscribe() và gọi publish() để thông báo cho từng observer.
class ITemperatureObserver{
    public: 
        virtual ~ITemperatureObserver() {}
        virtual void onTemperatureExceeded(const TemperatureExceededEvent& event) = 0;
};
# Class AlertService và ShipmentAggregator kế thừa từ ITemperatureObserver: dùng từ khóa override để cài đặt hàm onTemperatureExceeded,, giúp trình biên dịch phát hiện lỗi nếu sai chữ ký hàm 
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

# Vấn đề tồn tại: thân hàm của AlertService và ShipmentAggregator đang để trống, đây là phần dự phòng cho các chức năng như gửi cảnh báo hay tổng hợp dữ liệu trong tương lai 
