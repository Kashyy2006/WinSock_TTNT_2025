// client/login.js
document.getElementById('connection-form').addEventListener('submit', function(event) {
    event.preventDefault(); // Ngăn không cho form reload lại trang

    const ip = document.getElementById('ip-address').value.trim();
    const port = document.getElementById('port').value.trim();

    // Kiểm tra cơ bản (bạn có thể thêm regex để kiểm tra IP kỹ hơn)
    if (!ip || !port) {
        alert("Vui lòng nhập đầy đủ IP và Port.");
        return;
    }

    // Tạo URL đầy đủ
    const serverUrl = `http://${ip}:${port}`;

    // Lưu vào localStorage để dùng ở các trang khác
    localStorage.setItem('serverUrl', serverUrl);

    // Chuyển hướng đến trang dashboard chính
    window.location.href = 'index.html';
});