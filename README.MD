# H2O2 Concentration Meter

## 1. Giới thiệu
Đồ án nghiên cứu và chế tạo thiết bị đo nồng độ Hydrogen Peroxide (H2O2) trong dung dịch bằng phương pháp đo quang học, sử dụng cảm biến TCS3200 và vi điều khiển ATmega2560.

## 2. Thành phần hệ thống
- Vi điều khiển: ATmega 2560 PRO EMBEDD
- Cảm biến quang học: TCS3200
- Nguồn sáng: LED vàng 1W
- Hiển thị: LCD
- Thuật toán xử lý: Linear Regression và SVR-RBF

## 3. Cấu trúc mã nguồn
- `SVR_RBF/`: Huấn luyện, đánh giá và dự đoán nồng độ bằng Python.
- `Linear_Regression/`: Chương trình điều khiển vi điều khiển.
- `Results/`: Kết quả đánh giá và dự đoán mô hình.

## 4. Cách sử dụng
1. Cài đặt các thư viện trong `requirements.txt`.
2. Chuẩn bị dữ liệu CSV theo định dạng mà chương trình yêu cầu.
3. Chạy chương trình Python để huấn luyện hoặc dự đoán.
4. Sử dụng file header mô hình SVR-RBF trong chương trình AVR.

## 5. Mục tiêu
Xây dựng thiết bị đo nhỏ gọn, chi phí thấp, có khả năng ước lượng nồng độ H2O2 và hiệu chỉnh kết quả bằng học máy.