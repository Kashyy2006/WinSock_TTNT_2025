/* script.js */

// 1. CẤU HÌNH SERVER
// Mặc định lấy từ localStorage, nếu không có thì dùng chuỗi rỗng (để trình duyệt tự hiểu là cùng domain)
// Nếu bạn chạy file html riêng lẻ (không qua server C++), hãy set mặc định là "http://localhost:6969"
let SERVER_URL = localStorage.getItem("server_url") || "";

// Khởi chạy khi trang load xong
document.addEventListener("DOMContentLoaded", () => {
    updateUIInfo();
});

function updateUIInfo() {
    const displayUrl = SERVER_URL === "" ? "Localhost / Relative" : SERVER_URL;
    const infoSpan = document.getElementById("current-server-info");
    if(infoSpan) infoSpan.innerText = displayUrl;
}

// Hàm đổi IP Server
function updateServerConfig() {
    const oldVal = SERVER_URL || "http://localhost:6969";
    let input = prompt("Nhập địa chỉ Server C++ (VD: http://192.168.1.10:6969):", oldVal);
    
    if (input !== null) {
        // Xóa dấu gạch chéo cuối nếu có
        input = input.replace(/\/$/, "");
        
        SERVER_URL = input;
        localStorage.setItem("server_url", SERVER_URL);
        
        updateUIInfo();
        showToast("Đã cập nhật Server IP thành công!", "success");
    }
}

// 2. HÀM GỬI LỆNH (CORE)
async function sendCommand(path, successMsg = "Thành công!") {
    // Nếu path chưa có http thì nối với SERVER_URL
    const fullUrl = path.startsWith("http") ? path : (SERVER_URL + path);

    try {
        const res = await fetch(fullUrl);
        if (res.ok) {
            showToast(successMsg, "success");
        } else {
            showToast(`Lỗi Server: ${res.status} ${res.statusText}`, "error");
        }
    } catch (err) {
        console.error(err);
        showToast("Không thể kết nối đến Server!", "error");
    }
}

// Hàm xác nhận hành động nguy hiểm
function confirmAction(path, msg) {
    if (confirm(msg)) {
        sendCommand(path, "Lệnh đang được thực thi...");
    }
}

// 3. HÀM XỬ LÝ HÌNH ẢNH
function fetchImage(endpoint) {
    const monitorDiv = document.getElementById('monitor-display');
    const imgTag = document.getElementById('result-image');
    const loadingText = document.getElementById('loading-text');
    
    // Tạo URL với timestamp để tránh Cache
    const fullUrl = (SERVER_URL + endpoint) + "?t=" + new Date().getTime();

    // UI đang tải
    if(loadingText) loadingText.style.display = 'block';
    if(loadingText) loadingText.innerText = "Đang tải dữ liệu từ server...";
    if(imgTag) imgTag.style.display = 'none';

    // Tải ảnh ngầm
    const tempImg = new Image();
    
    tempImg.onload = function() {
        if(imgTag) {
            imgTag.src = fullUrl;
            imgTag.style.display = 'block';
        }
        if(loadingText) loadingText.style.display = 'none';
        showToast("Đã nhận hình ảnh!", "success");
    };

    tempImg.onerror = function() {
        if(loadingText) loadingText.innerText = "❌ Lỗi: Không tải được ảnh.";
        showToast("Lỗi tải ảnh (Check server logs)", "error");
    };

    tempImg.src = fullUrl;
}

// 4. HỆ THỐNG THÔNG BÁO (TOAST)
function showToast(msg, type = "info") {
    const container = document.getElementById('toast-container');
    if(!container) return;

    const toast = document.createElement('div');
    toast.className = `toast-msg ${type}`;
    toast.innerText = msg;

    container.appendChild(toast);

    // Animation hiện ra
    requestAnimationFrame(() => {
        toast.classList.add('show');
    });

    // Tự động tắt sau 3 giây
    setTimeout(() => {
        toast.classList.remove('show');
        // Đợi animation biến mất xong mới xóa khỏi DOM
        setTimeout(() => {
            if(container.contains(toast)) {
                container.removeChild(toast);
            }
        }, 500);
    }, 3000);
}