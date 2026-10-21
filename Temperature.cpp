#include "Infrastructure.cpp"
#include "Extension.cpp"
#include "Presentation.cpp"
#include <iostream>

int main(){
    // Khởi tạo Infrastructure
    FileShipmentRepository repo("LICH_SU_KIEM_TRA.txt");

    TelemetryProcessor telemetryProcessor; 
    // Tao cac observer 
    AlertService alertService; 
    ShipmentAggregator aggregatorService; 
    DashboardNotifier dashboardNotifier; 
    // Đăng ký thêm Feed Logger vào telemetryProcessor sẵn có
    AlertFeedLogger alertFeedLogger;


    telemetryProcessor.subscribe(&alertService);
    telemetryProcessor.subscribe(&aggregatorService);
    telemetryProcessor.subscribe(&dashboardNotifier);
    telemetryProcessor.subscribe(&alertFeedLogger);

    
    ShipmentService shipmentService(&repo, &telemetryProcessor); 

    //Khởi tạo Presentatio và chạy chương trình
    ConsoleMenu menu(shipmentService);
    menu.displayMainMenu(); 

    cout << "\n======== KET XUAT BAO CAO & DASHBOARD GIAM SAT ========\n";

    // Khi cần xem danh sách cảnh báo (Alert Feed):
    alertFeedLogger.displayAlertFeed();

    // Khi cần xem bản đồ trạng thái màu sắc:
    DashboardView dashboard;
    dashboard.renderColorMap(shipmentService.getShipmentHistory());

    if(AuditReportExporter::exportToCSV(shipmentService.getShipmentHistory())){
        cout << "Da xuat bao cao thanh cong !\n"; 
    }
    return 0; 
}